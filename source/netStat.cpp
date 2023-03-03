/**
 * @brief Created by Yang Bo on 2020/5/9.
 * 
 * modified: 02/06/2023: comments translated to english
 */

#include "../include/netStat.h"


// insert new element
void QueueFixed::insert(double x) {
    array[now_index++] = x;
    if (now_index >= QueueCapacity)now_index = 0;
    ++now_size;
}

// Expand the queue and return the number of array elements
int QueueFixed::unroll(double *ans) {
    if (now_size >= QueueCapacity) {// If it is full, now_index is the first one pointed to
        for (int i = 0, j = now_index; i < QueueCapacity; ++i) {
            ans[i] = array[j];
            ++j;
            if (j >= QueueCapacity)
                j = 0;
        }
        return QueueCapacity;
    } else {
        for (int i = 0; i < now_index; ++i)
            ans[i] = array[i];
        return now_size;
    }
}

// Returns the last element entered into the queue
double QueueFixed::getLast() {
    if (now_index == 0)return array[QueueCapacity - 1];
    return array[now_index - 1];
}

// Predict the next value using Lagrangian interpolation
double Extrapolator::predict(double t) {
    // expand the argument into an array
    int sz = tQ.unroll(tArr);
    if (sz < 2) { // less than 2, unpredictable
        if (sz == 0)return 0;
        else return vQ.getLast();
    }

    double diff_sum = 0;
    for (int i = 1; i < sz; ++i)diff_sum += tArr[i] - tArr[i - 1];
    // If the time difference between the current time and the previous one is greater than ten times the average time difference,
    // it cannot be predicted, and the last result in the queue will be returned directly.
    if (diff_sum / (sz - 1) * 10 < (t - tQ.getLast()))
        return vQ.getLast();

    // expand the function value
    vQ.unroll(vArr);

    // Do Lagrangian interpolation
    // Lagrange interpolation formula, L(tp) = sum_{i=1}^{n+1} { y_i * l_i(tp) }
    // where l_i(tp) = \frac{ \prod_{j=1 and j!=i }(tp-x_j) } { \prod_{j=1 and j!=i}(x_i-x_j) }
    double ans = 0;
    for (int i = 0; i < sz; ++i) {
        double y = vArr[i];
        for (int j = 0; j < sz; ++j) {
            if (i == j)continue;
            y *= (t - tArr[j]) / (tArr[i] - tArr[j] + 1e-20);
        }
        ans += y;
    }
    return ans;
}

// constructor of incStat
IncStat::IncStat(const std::string &id, std::vector<double> *_lambdas, double init_time, bool isTypediff) {
    ID = id;
    lambdas = _lambdas;
    isTypeDiff = isTypediff;
    lastTimestamp = init_time;
    mean_valid = var_valid = std_valid = false;
    auto size = lambdas->size();
    // Allocate memory
    CF1 = new double[size];
    CF2 = new double[size];
    w = new double[size];
    cur_std = new double[size];
    cur_mean = new double[size];
    cur_var = new double[size];
    // initialization
    for (int i = 0; i < size; ++i) CF1[i] = 0;
    for (int i = 0; i < size; ++i) CF2[i] = 0;
    for (int i = 0; i < size; ++i) w[i] = 1e-20;//prevent division by 0
}

// incStat's destructor
IncStat::~IncStat() {
    // Release the memory of the relationship class between the two streams maintained
    for (auto v : covs) {
        // 这个实例会有多个类的指针指向, 所以维护一个refNum, 当减为0的时候,就delete掉
        if ((--v->refNum) == 0)
            delete v;
    }
    delete[] CF1;
    delete[] CF2;
    delete[] w;
    delete[] cur_std;
    delete[] cur_mean;
    delete[] cur_var;
}

// stream inserts new stats.
void IncStat::insert(double v, double t) {
    // If isTypeDiff is set, use the time difference as statistics
    if (isTypeDiff) {
        double dif = t - lastTimestamp;
        v = dif > 0 ? dif : 0;
    }

    // Decay first
    processDecay(t);

    // update with v
    for (size_t i = 0; i < lambdas->size(); ++i) CF1[i] += v;
    for (size_t i = 0; i < lambdas->size(); ++i) CF2[i] += v * v;
    for (size_t i = 0; i < lambdas->size(); ++i) ++w[i];

    // The mean, variance, and standard deviation will not be calculated yet. 
    // calculate later
    mean_valid = var_valid = std_valid = false;

}

// Execute decay, the parameter is the current timestamp
void IncStat::processDecay(double timestamp) {
    double diff = timestamp - lastTimestamp;
    if (diff > 0) {
        for (size_t i = 0; i < lambdas->size(); ++i) {
            // Calculate the decay factor
            double factor = std::pow(2.0, -lambdas->at(i) * diff);
            CF1[i] *= factor;
            CF2[i] *= factor;
            w[i] *= factor;
        }
        lastTimestamp = timestamp;
    }
}

void IncStat::calMean() {
    if (!mean_valid) { // recalculate when needed
        mean_valid = true;
        for (size_t i = 0; i < lambdas->size(); ++i)
            cur_mean[i] = CF1[i] / w[i];
    }
}

void IncStat::calVar() {
    if (!var_valid) {
        var_valid = true;
        calMean(); // The calculation requires the mean value, first update the mean value
        for (size_t i = 0; i < lambdas->size(); ++i)
            cur_var[i] = fabs(CF2[i] / w[i] - cur_mean[i] * cur_mean[i]);
    }
}

void IncStat::calStd() {
    if (!std_valid) {
        std_valid = true;
        calVar(); // Calculation requires variance, calculate it first
        for (size_t i = 0; i < lambdas->size(); ++i)
            cur_std[i] = std::sqrt(cur_var[i]);
    }
}

// Get all one-dimensional statistical information, (weight, mean, variance)
int IncStat::getAll1DStats(double *result) {
    calMean();
    calVar();
    int offset = 0;
    for (size_t i = 0; i < lambdas->size(); ++i)result[offset++] = (w[i]);
    for (size_t i = 0; i < lambdas->size(); ++i)result[offset++] = (cur_mean[i]);
    for (size_t i = 0; i < lambdas->size(); ++i)result[offset++] = (cur_var[i]);
    return offset;
}


// Update statistics such as covariance of the two streams.
// It can only be called after one of the two streams has been updated, and then 
// That is, after one of the stream insert methods is updated, this method is called immediately to update the relevant statistical data
/**
 * @brief Update statistics. the parameters are the ID of the updated stream, v and t used for updating the updated stream
 * 
 * @param ID ID of the updated stream
 * @param v 
 * @param t 
 */
void IncStatCov::updateCov(const std::string &ID, double v, double t) {
    // Decay first
    processDecay(t);

    // update the mean of the two streams
    incS1->calMean();
    incS2->calMean();

    if (ID == incS1->ID) { // If it's the first update to stream
        // Update the information maintained by the first flow extrapolation method
        ex1.insert(t, v);
        // Get the updated value of the second stream prediction
        double v_other = ex1.predict(t);
        for (size_t i = 0; i < lambdas->size(); ++i) {
            CF3[i] += (v - incS1->cur_mean[i]) * (v_other - incS2->cur_mean[i]);
        }
    } else {// The updated value from the second stream
        // Update the information maintained by the second flow extrapolation method
        ex2.insert(t, v);
        // Get the predicted value of the first stream
        double v_other = ex2.predict(t);
        // Update the numerator part of the covariance (CF3)
        for (size_t i = 0; i < lambdas->size(); ++i) {
            CF3[i] += (v_other - incS1->cur_mean[i]) * (v - incS2->cur_mean[i]);
        }
    }
    // Update weights
    for (size_t i = 0; i < lambdas->size(); ++i) ++w3[i];
}

// Execute the decay function
void IncStatCov::processDecay(double t) {
    double diff = t - lastTimestamp;
    if (diff > 0) {
        for (size_t i = 0; i < lambdas->size(); ++i) {
            double factor = std::pow(2.0, -lambdas->at(i) * diff);
            CF3[i] *= factor;
            w3[i] *= factor;
        }
        lastTimestamp = t;
    }
}

// Computes the radius (square root of sum of variance) of two streams
int IncStatCov::getRadius(double *result) {
    incS1->calVar();
    incS2->calVar();
    for (size_t i = 0; i < lambdas->size(); ++i) {
        result[i] = (std::sqrt(incS1->cur_var[i] + incS2->cur_var[i]));
    }
    return lambdas->size();
}

// Computes the square root of the sum of squares of the means of two streams
int IncStatCov::getMagnitude(double *result) {
    incS1->calMean();
    incS2->calMean();
    for (size_t i = 0; i < lambdas->size(); ++i) {
        double mean1 = incS1->cur_mean[i];
        double mean2 = incS2->cur_mean[i];
        result[i] = (std::sqrt(mean1 * mean1 + mean2 * mean2));
    }
    return lambdas->size();
}

// Calculate the covariance of two streams
int IncStatCov::getCov(double *result) {
    for (size_t i = 0; i < lambdas->size(); ++i)
        result[i] = (CF3[i] / w3[i]);
    return lambdas->size();
}

// Calculates the correlation coefficient of two streams
int IncStatCov::getPcc(double *result) {
    incS1->calStd();
    incS2->calStd();
    for (size_t i = 0; i < lambdas->size(); ++i) {
        double ss = incS1->cur_std[i] * incS2->cur_std[i];
        if (ss < 1e-20) result[i] = 0;
        else result[i] = (CF3[i] / (w3[i] * ss));
    }
    return lambdas->size();
}

//Get all two-dimensional statistical information [ radius,magnitude,cov,pcc ], return the number added to the array
int IncStatCov::getAll2DStats(double *result) {
    int offset = getRadius(result);
    offset += getMagnitude(result + offset);
    offset += getCov(result + offset);
    return offset + getPcc(result + offset);
}


// Update the one-dimensional and two-dimensional information of the specified stream, and return the one-dimensional [weight, mean, std] and two-dimensional [radius, magnitude, cov, pcc]
// The parameters are: stream ID, timestamp, statistical data, reference to the returned result.  if the last one is set to true, the timestamp will be used as statistical data

int IncStatDB::updateGet1DStats(const std::string &ID, double t, double v, double *result, bool isTypeDiff) {
    auto it = stats.find(ID);
    if (it == stats.end()) { // If not found, generate a new stream
        auto *incStat = new IncStat(ID, lambdas, t, isTypeDiff);
        auto ret = stats.insert(std::make_pair(ID, incStat));
        it = ret.first;
    }
    // The statistics of the stream pointed to by it->second
    it->second->insert(v, t);
    return it->second->getAll1DStats(result);
}


// Update the two-dimensional information of the specified stream, and add [radius, magnitude, cov, pcc] to the result
// The parameters are: ID of the first stream, ID of the second stream, statistical information of the first stream, timestamp, pointer to the result array,
// Return the number of data added to the result array
int IncStatDB::updateGet2DStats(const std::string &ID1, const std::string &ID2, double t1, double v1,
                                double *result, bool isTypediff) {
    // Get two streams, generate a new one if not found
    auto it1 = stats.find(ID1);
    if (it1 == stats.end()) { // If not found, generate a new stream
        auto *incStat1 = new IncStat(ID1, lambdas, t1, isTypediff);
        auto ret1 = stats.insert(std::make_pair(ID1, incStat1));
        it1 = ret1.first;
    }
    auto it2 = stats.find(ID2);
    if (it2 == stats.end()) { // If not found, generate a new stream
        auto *incStat2 = new IncStat(ID2, lambdas, t1, isTypediff);
        auto ret2 = stats.insert(std::make_pair(ID2, incStat2));
        it2 = ret2.first;
    }

    // Get the relationship between two streams, and update all other stream relationships related to ID1 at the same time
    IncStatCov *incStatCov = nullptr;
    for (auto v:it1->second->covs) {
        v->updateCov(ID1, v1, t1);
        // While updating, look for streams related to ID2
        if (incStatCov == nullptr && (v->incS1->ID == ID2 || v->incS2->ID == ID2))
            incStatCov = v;
    }

    // If not found, generate a new relationship between streams
    if (incStatCov == nullptr) {
        incStatCov = new IncStatCov(it1->second, it2->second, lambdas, t1);
        incStatCov->refNum = 2;
        // Save this reference in both streams. When destructing, the number of references will be judged, and it will be deleted only when it is 0.
        it1->second->covs.push_back(incStatCov);
        it2->second->covs.push_back(incStatCov);
        incStatCov->updateCov(ID1, v1, t1);
    }

    // Get statistics between two streams
    return incStatCov->getAll2DStats(result);
}


// Constructor, parameters are lambdas
NetStat::NetStat(const std::vector<double> &l) {
    //Initialize the four maintained flow information, and pass the pointer of the time window list to it.
    lambdas = std::vector<double>(l);
    HT_jit = new IncStatDB(&lambdas);
    HT_Hp = new IncStatDB(&lambdas);
    HT_MI = new IncStatDB(&lambdas);
    HT_H = new IncStatDB(&lambdas);
}

// No-argument constructor, using default lambdas
NetStat::NetStat() {
    lambdas = std::vector<double>({5, 3, 1, 0.1, 0.01});
    HT_jit = new IncStatDB(&lambdas);
    HT_Hp = new IncStatDB(&lambdas);
    HT_MI = new IncStatDB(&lambdas);
    HT_H = new IncStatDB(&lambdas);
}

// The main call function, pass in a package information, and return the corresponding statistical vector
// The parameters are: source MAC, destination MCA, source IP, IP protocol type, destination IP, destination IP protocol type, packet size, packet timestamp
int NetStat::updateAndGetStats(const std::string &srcMAC, const std::string &dstMAC,
                               const std::string &srcIP, const std::string &srcPort,
                               const std::string &dstIP, const std::string &dstPort,
                               double datagramSize, double timestamp, double *result) {

    int offset = 0; // The offset of the array (the number currently placed)

    // MAC.IP: Statistical source host MAC and IP relationship and bandwidth
    offset += HT_MI->updateGet1DStats(srcMAC + srcIP, timestamp, datagramSize, result);

    // Host-Host BW: Statistics of the sending flow of the source IP host (one-dimensional relationship)
    // two-dimensional relationship between the sending behavior of the source IP host and the destination IP host
    offset += HT_H->updateGet1D2DStats(srcIP, dstIP, timestamp, datagramSize, result + offset);

    // Host-Host Jitter: Jitter between hosts and hosts
    offset += HT_jit->updateGet1DStats(srcIP + dstIP, timestamp, 0, result + offset, true);

    // Host-Host BW: Statistics of the sending flow of the source IP port (one-dimensional relationship) The sending behavior relationship between the source IP port and the destination IP port (two-dimensional relationship)
    // If it is not a tcp/udp package, let the mac address be the key value of the stream
    if (srcPort == "arp") {
        offset += HT_Hp->updateGet1D2DStats(srcMAC, dstMAC, timestamp, datagramSize, result + offset);
    } else {
        offset += HT_Hp->updateGet1D2DStats(srcIP + srcPort, dstIP + dstPort, timestamp,
                                            datagramSize, result + offset);
    }
    return offset;
}

