//
// Created by Yang Bo on 2020/5/9.
//

#include "../include/netStat.h"


// 插入新元素
void QueueFixed::insert(double x) {
    array[now_index++] = x;
    if (now_index >= QueueCapacity)now_index = 0;
    ++now_size;
}

// 将队列展开, 返回数组元素个数
int QueueFixed::unroll(double *ans) {
    if (now_size >= QueueCapacity) {// 如果已经满了,now_index就是指向的第一个
        for (int i = 0, j = now_index; i < QueueCapacity; ++i) {
            ans[i] = array[j];
            ++j;
            if (j >= QueueCapacity)j = 0;
        }
        return QueueCapacity;
    } else {
        for (int i = 0; i < now_index; ++i)ans[i] = array[i];
        return now_size;
    }
}

// 返回最后进入队列的元素
double QueueFixed::getLast() {
    if (now_index == 0)return array[QueueCapacity - 1];
    return array[now_index - 1];
}

// 使用拉格朗日插值预测下一个值
double Extrapolator::predict(double t) {
    // 将自变量展开变成数组
    int sz = tQ.unroll(tArr);
    if (sz < 2) { // 小于2个, 无法预测
        if (sz == 0)return 0;
        else return vQ.getLast();
    }

    double diff_sum = 0;
    for (int i = 1; i < sz; ++i)diff_sum += tArr[i] - tArr[i - 1];
    //如果当前的时间与上一个的时间差大于平均时间差的十倍, 就无法预测,直接返回队列中最后一个结果了.
    if (diff_sum / (sz - 1) * 10 < (t - tQ.getLast()))
        return vQ.getLast();

    // 将函数值展开
    vQ.unroll(vArr);

    // 进行拉格朗日插值
    // 拉格朗日插值公式, L(tp) = sum_{i=1}^{n+1} { y_i * l_i(tp)  }
    // 其中 l_i(tp) = \frac{ \prod_{j=1 and j!=i }(tp-x_j) } { \prod_{j=1 and j!=i}(x_i-x_j) }
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

// incStat的构造函数
IncStat::IncStat(const std::string &id, std::vector<double> *_lambdas, double init_time, bool isTypediff) {
    ID = id;
    lambdas = _lambdas;
    isTypeDiff = isTypediff;
    lastTimestamp = init_time;
    mean_valid = var_valid = std_valid = false;
    auto size = lambdas->size();
    // 分配内存
    CF1 = new double[size];
    CF2 = new double[size];
    w = new double[size];
    cur_std = new double[size];
    cur_mean = new double[size];
    cur_var = new double[size];
    // 初始化
    for (int i = 0; i < size; ++i)CF1[i] = 0;
    for (int i = 0; i < size; ++i)CF2[i] = 0;
    for (int i = 0; i < size; ++i)w[i] = 1e-20;//防止除以0
}

// incStat的析构函数
IncStat::~IncStat() {
    for (auto v:covs) {// 主要是释放维护的两个流之间关系类的内存
        // 这个实例会有多个类的指针指向, 所以维护一个refNum, 当减为0的时候,就delete掉
        if ((--v->refNum) == 0)delete v;
    }
    delete[] CF1;
    delete[] CF2;
    delete[] w;
    delete[] cur_std;
    delete[] cur_mean;
    delete[] cur_var;
}

// 流插入新的统计数据.
void IncStat::insert(double v, double t) {
    // 如果设定了isTypeDiff, 就用时间的差值做统计信息
    if (isTypeDiff) {
        double dif = t - lastTimestamp;
        v = dif > 0 ? dif : 0;
    }

    // 先执行衰减
    processDecay(t);

    // 用v来更新
    for (size_t i = 0; i < lambdas->size(); ++i)CF1[i] += v;
    for (size_t i = 0; i < lambdas->size(); ++i)CF2[i] += v * v;
    for (size_t i = 0; i < lambdas->size(); ++i)++w[i];

    // 均值,方差,标准差先不计算, 等用时再算
    mean_valid = var_valid = std_valid = false;

}

// 执行衰减,参数为当前时间戳
void IncStat::processDecay(double timestamp) {
    double diff = timestamp - lastTimestamp;
    if (diff > 0) {
        for (size_t i = 0; i < lambdas->size(); ++i) {
            // 计算衰减因子
            double factor = std::pow(2.0, -lambdas->at(i) * diff);
            CF1[i] *= factor;
            CF2[i] *= factor;
            w[i] *= factor;
        }
        lastTimestamp = timestamp;
    }
}

// 计算均值
void IncStat::calMean() {
    if (!mean_valid) { // 需要时再计算
        mean_valid = true;
        for (size_t i = 0; i < lambdas->size(); ++i)
            cur_mean[i] = CF1[i] / w[i];
    }
}

// 计算方差
void IncStat::calVar() {
    if (!var_valid) {
        var_valid = true;
        calMean(); // 计算需要均值, 先更新均值
        for (size_t i = 0; i < lambdas->size(); ++i)
            cur_var[i] = fabs(CF2[i] / w[i] - cur_mean[i] * cur_mean[i]);
    }
}

// 计算标准差
void IncStat::calStd() {
    if (!std_valid) {
        std_valid = true;
        calVar(); // 计算需要方差, 先计算出来
        for (size_t i = 0; i < lambdas->size(); ++i)
            cur_std[i] = std::sqrt(cur_var[i]);
    }
}

// 获取全部的一维统计信息, (权值,均值,方差)
int IncStat::getAll1DStats(double *result) {
    calMean();
    calVar();
    int offset = 0;
    for (size_t i = 0; i < lambdas->size(); ++i)result[offset++] = (w[i]);
    for (size_t i = 0; i < lambdas->size(); ++i)result[offset++] = (cur_mean[i]);
    for (size_t i = 0; i < lambdas->size(); ++i)result[offset++] = (cur_var[i]);
    return offset;
}


//更新这两个流的协方差等统计信息.
//只能是两个流其中一个流更新完调用的, 然后参数就是更新完的那个流的ID, 更新完的那个流更新用的v和t
//也就是其中一个流insert方法更新之后, 就紧接着调用这个方法, 更新相关的统计数据
void IncStatCov::updateCov(const std::string &ID, double v, double t) {
    // 首先进行衰减
    processDecay(t);

    // 更新两个流的均值
    incS1->calMean();
    incS2->calMean();

    if (ID == incS1->ID) { // 如果是第一个流传来的更新
        // 更新第一个流外推法维护的信息
        ex1.insert(t, v);
        // 获取第二个流预测的更新值
        double v_other = ex1.predict(t);
        for (size_t i = 0; i < lambdas->size(); ++i) {
            CF3[i] += (v - incS1->cur_mean[i]) * (v_other - incS2->cur_mean[i]);
        }
    } else {// 第二个流传来的更新值
        // 更新第二个流外推法维护的信息
        ex2.insert(t, v);
        // 获取第一个流的预测值
        double v_other = ex2.predict(t);
        // 更新协方差的分子部分(CF3)
        for (size_t i = 0; i < lambdas->size(); ++i) {
            CF3[i] += (v_other - incS1->cur_mean[i]) * (v - incS2->cur_mean[i]);
        }
    }
    // 更新权值
    for (size_t i = 0; i < lambdas->size(); ++i) ++w3[i];
}

// 执行衰减函数
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

// 计算两个流的radius( 方差和的平方根 )
int IncStatCov::getRadius(double *result) {
    incS1->calVar();
    incS2->calVar();
    for (size_t i = 0; i < lambdas->size(); ++i) {
        result[i] = (std::sqrt(incS1->cur_var[i] + incS2->cur_var[i]));
    }
    return lambdas->size();
}

// 计算两个流的均值平方和的平方根
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

// 计算两个流的协方差
int IncStatCov::getCov(double *result) {
    for (size_t i = 0; i < lambdas->size(); ++i)
        result[i] = (CF3[i] / w3[i]);
    return lambdas->size();
}

// 计算两个流的相关系数
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

//获取所有的二维统计信息 [ radius,magnitude,cov,pcc ], 返回往数组里增加的个数
int IncStatCov::getAll2DStats(double *result) {
    int offset = getRadius(result);
    offset += getMagnitude(result + offset);
    offset += getCov(result + offset);
    return offset + getPcc(result + offset);
}


// 更新指定流的一维,二维信息, 将一维的[ weight,mean,std]和二维的[radius,magnitude,cov,pcc]返回
// 参数分别是: 流的ID, 时间戳, 统计数据, 返回结果的引用, 最后一个如果设置true,就用时间戳作为统计数据

int IncStatDB::updateGet1DStats(const std::string &ID, double t, double v, double *result, bool isTypeDiff) {
    auto it = stats.find(ID);
    if (it == stats.end()) { // 如果没找到, 就生成一个新的流
        auto *incStat = new IncStat(ID, lambdas, t, isTypeDiff);
        auto ret = stats.insert(std::make_pair(ID, incStat));
        it = ret.first;
    }
    // it->second现在指向的这个流的统计信息
    it->second->insert(v, t);
    return it->second->getAll1DStats(result);
}


// 更新指定流的二维信息,将[radius,magnitude,cov,pcc]添加到结果里面
// 参数分别是: 第一个流的ID,第二个流的ID, 第一个流的统计信息,时间戳, 结果数组的指针,
// 返回往结果数组里增加的数据的个数
int IncStatDB::updateGet2DStats(const std::string &ID1, const std::string &ID2, double t1, double v1,
                                double *result, bool isTypediff) {
    // 获取两个流, 没有找到就生成新的
    auto it1 = stats.find(ID1);
    if (it1 == stats.end()) { // 如果没找到, 就生成一个新的流
        auto *incStat1 = new IncStat(ID1, lambdas, t1, isTypediff);
        auto ret1 = stats.insert(std::make_pair(ID1, incStat1));
        it1 = ret1.first;
    }
    auto it2 = stats.find(ID2);
    if (it2 == stats.end()) { // 如果没找到, 就生成一个新的流
        auto *incStat2 = new IncStat(ID2, lambdas, t1, isTypediff);
        auto ret2 = stats.insert(std::make_pair(ID2, incStat2));
        it2 = ret2.first;
    }

    // 获取两个流之间的关系, 同时更新其他所有与ID1相关的流关系
    IncStatCov *incStatCov = nullptr;
    for (auto v:it1->second->covs) {
        v->updateCov(ID1, v1, t1);
        // 更新的同时寻找有没有与ID2有关系的流
        if (incStatCov == nullptr && (v->incS1->ID == ID2 || v->incS2->ID == ID2))
            incStatCov = v;
    }

    // 如果没找到, 就生成一个新的流之间的关系
    if (incStatCov == nullptr) {
        incStatCov = new IncStatCov(it1->second, it2->second, lambdas, t1);
        incStatCov->refNum = 2;
        // 两个流都保存一下这个引用, 析构的时候会判断这个引用数量, 为0的时候才会delete掉
        it1->second->covs.push_back(incStatCov);
        it2->second->covs.push_back(incStatCov);
        incStatCov->updateCov(ID1, v1, t1);
    }

    // 获得两个流之间的统计信息
    return incStatCov->getAll2DStats(result);
}


// 构造器, 参数是lambdas
NetStat::NetStat(const std::vector<double> &l) {
    // 初始化四个维护的流信息, 将时间窗口列表的指针传过去.
    lambdas = std::vector<double>(l);
    HT_jit = new IncStatDB(&lambdas);
    HT_Hp = new IncStatDB(&lambdas);
    HT_MI = new IncStatDB(&lambdas);
    HT_H = new IncStatDB(&lambdas);
}

// 无参构造器, 使用默认的lambdas
NetStat::NetStat() {
    lambdas = std::vector<double>({5, 3, 1, 0.1, 0.01});
    HT_jit = new IncStatDB(&lambdas);
    HT_Hp = new IncStatDB(&lambdas);
    HT_MI = new IncStatDB(&lambdas);
    HT_H = new IncStatDB(&lambdas);
}

// 主要的调用函数, 传进去一个包信息, 返回对应的统计向量
// 参数分别是: 源MAC,目的MCA,源IP, IP协议类型, 目的IP, 目的IP协议类型, 数据包大小, 数据包时间戳
int NetStat::updateAndGetStats(const std::string &srcMAC, const std::string &dstMAC,
                               const std::string &srcIP, const std::string &srcProtocol,
                               const std::string &dstIP, const std::string &dstProtocol,
                               double datagramSize, double timestamp, double *result) {

    int offset = 0; // 数组的偏移(当前已经放的个数)

    // MAC.IP: 统计源主机MAC和IP的关系与带宽
    offset += HT_MI->updateGet1DStats(srcMAC + srcIP, timestamp, datagramSize, result);

    // Host-Host BW: 源IP主机的发送流的统计(一维关系), 源IP主机和目的IP主机的发送行为二维关系
    offset += HT_H->updateGet1D2DStats(srcIP, dstIP, timestamp, datagramSize, result + offset);

    // Host-Host Jitter: 主机与主机之间的抖动
    offset += HT_jit->updateGet1DStats(srcIP + dstIP, timestamp, 0, result + offset, true);

    // Host-Host BW: 源IP端口的发送流的统计(一维关系) 源IP端口和目的IP端口的发送行为关系(二维关系)
    // 如果不是tcp/udp的包, 就让mac地址作为流的键值
    if (srcProtocol == "arp") {
        offset += HT_Hp->updateGet1D2DStats(srcMAC, dstMAC, timestamp, datagramSize, result + offset);
    } else {
        offset += HT_Hp->updateGet1D2DStats(srcIP + srcProtocol, dstIP + dstProtocol, timestamp,
                                            datagramSize, result + offset);
    }
    return offset;
}

