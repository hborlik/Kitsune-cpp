/**
 * @brief Created by Yang Bo on 2020/5/9.
 * 
 * modified: 02/06/2023: comments translated to english
 */
#ifndef KITSUNE_CPP_NETSTAT_H
#define KITSUNE_CPP_NETSTAT_H


/**
  * Implemented Kitsune's feature statistics algorithm
  * This file contains classes: NetStat, IncStat, IncStatCov, IncStatDB
  * There are two tool classes: QueueFixed (fixed size queue), Extractor (calculation of Lagrange extrapolation method)
  */

#include <vector>
#include <string>
#include <map>
#include <cmath>


/**
 *  A fixed-size queue that only inserts and does not delete
 */
class QueueFixed {
public:
    // Queue Capacity, Static Elements
    static const int QueueCapacity = 3;

private:
    double array[QueueFixed::QueueCapacity];
    // The index where the current insertion is placed
    int now_index;
    // current number of elements
    int now_size;

public:

    QueueFixed() {
        now_index = now_size = 0;
    }

    // insert new element
    void insert(double x);

    // Expand the queue into an array, copy it into arr, and return the number of elements in the array
    int unroll(double *arr);

    // Returns the last element entered into the queue
    double getLast();

};

class Extrapolator {
private:
    // Maintain two queues, one of which is the independent variable t and the other is the function value v
    QueueFixed tQ, vQ;

    // Save expanded value
    double tArr[QueueFixed::QueueCapacity];
    double vArr[QueueFixed::QueueCapacity];

public:
    // insert new element
    void insert(double t, double v) {
        tQ.insert(t);
        vQ.insert(v);
    }

    // Predict the next value using Lagrangian interpolation
    double predict(double t);
};


class IncStatCov; // Because of cross-references, declare this class first


/**
 *  IncStat is the incremental data statistics of a specific stream
 */
class IncStat {
private:
    // The decay factor of the stream, which is the pointer to the time window list
    std::vector<double> *lambdas;

    // Statistical linear sum, square sum, and weight list
    // the i-th value corresponds to the statistical information of the i-th time window
    double *CF1 = nullptr, *CF2 = nullptr, *w = nullptr;

    // last timestamp
    double lastTimestamp;

    // Is the current mean, variance, and standard deviation valid (when false, needs to be recalculated)
    bool mean_valid, var_valid, std_valid;

    // Is the type different? 
    // If true, use the new timestamp as data (also for calculating timestamp statistics)
    bool isTypeDiff;

public:
    // The ID of the current stream index
    std::string ID;

    // The current mean, variance, standard deviation list
    // the i-th value corresponds to the statistical information of the i-th time window
    double *cur_mean = nullptr, *cur_var = nullptr, *cur_std = nullptr;

    // the collection of streams associated with the current stream
    std::vector<IncStatCov *> covs;

    // Constructor, the parameters are the ID of the current stream, lambda, the initialization timestamp, 
    // whether to use the timestamp as statistics
    IncStat(const std::string &_ID, std::vector<double> *_lambdas, double init_time = 0, bool isTypediff = false);

    // Destructor, to destroy the pointer content of the edge information
    ~IncStat();

    // A function to insert new data, the parameters are vstatistics, ttimestamp
    void insert(double v, double t = 0);

    // Execute decay, the parameter is the current timestamp
    void processDecay(double timestamp);

    // Calculate mean
    void calMean();

    // Calculate the variance
    void calVar();

    // Calculate standard deviation
    void calStd();

    // Get all the one-dimensional statistical information
    // (weight, mean, variance), and append the result to the result, and return the number of increased data
    int getAll1DStats(double *result);
};


/**
 * IncStatCov 维护两个流之间的关系(连边),
 * 里面存放着两个流的指针和他们两个之间的统计信息
 */
class IncStatCov {
private:
    // 维护的时间窗口列表的指针
    std::vector<double> *lambdas;
    // 每个值减去均值的乘积和 , sum (A-uA)(B-uB), 协方差的分子部分
    double *CF3 = nullptr;
    // 当前权值
    double *w3 = nullptr;
    // 上次时间戳
    double lastTimestamp;
    // 两个拉格朗日外推法的类
    Extrapolator ex1, ex2;

public:
    // 两个流的指针:
    IncStat *incS1, *incS2;
    // 被引用的数量, 如果为0就销毁
    int refNum;

    // 构造函数, 参数分别是两个流的指针,lambdas指针,初始时间戳
    IncStatCov(IncStat *inc1, IncStat *inc2, std::vector<double> *l, double init_time) {
        lambdas = l;
        incS1 = inc1;
        incS2 = inc2;
        lastTimestamp = init_time;

        CF3 = new double[lambdas->size()];
        for (size_t i = 0; i < lambdas->size(); ++i)CF3[i] = 0;
        w3 = new double[lambdas->size()];
        // 防止除以0
        for (size_t i = 0; i < lambdas->size(); ++i)w3[i] = 1e-20;
    }

    // 析构函数, (指向的类的实例会由map析构的时候调用的), 只需要delete掉自己new的即可
    ~IncStatCov() {
        if (CF3 != nullptr) delete[] CF3;
        if (w3 != nullptr) delete[] w3;
    }

    //更新这两个流的协方差等统计信息.
    //只能是两个流其中一个流更新完调用的, 然后参数就是更新完的那个流的ID, 更新完的那个流更新用的v和t
    //也就是其中一个流insert方法更新之后, 就紧接着调用这个方法, 更新相关的统计数据
    void updateCov(const std::string &ID, double v, double t);

    // 执行衰减函数
    void processDecay(double t);

    // 计算两个流的radius( 方差和的平方根 ), 返回增加的数据的个数
    int getRadius(double *result);

    // 计算两个流的均值平方和的平方根, 返回增加的数据的个数
    int getMagnitude(double *result);

    // 计算两个流的协方差, 返回增加的数据的个数
    int getCov(double *result);

    // 计算两个流的相关系数, 返回增加的数据的个数
    int getPcc(double *result);

    //获取所有的二维统计信息 [ radius,magnitude,cov,pcc ], 返回增加的数据的个数
    int getAll2DStats(double *result);
};


/**
 *  IncStatDB 维护当前统计的一类流的集合
 */
class IncStatDB {
private:
    // 统计的一类流的集合, string为对应的键值, value 为指向对应流的指针
    std::map<std::string, IncStat *> stats;
    // lambdas 维护的时间窗口列表的 指针
    std::vector<double> *lambdas;

public:
    // 构造器, 将时间窗口的指针列表传过来
    IncStatDB(std::vector<double> *l) {
        lambdas = l;
    }

    // 更新指定流的一维信息, 并将统计值[weight,mean,std]追加到result里, 返回增加的数据的个数
    int updateGet1DStats(const std::string &ID, double t, double v, double *result,
                         bool isTypeDiff = false);

    // 更新指定流的二维信息,将[radius,magnitude,cov,pcc]添加到结果里面
    // 参数分别是: 第一个流的ID,第二个流的ID, 第一个流的统计信息,时间戳, 结果的数组的指针, 返回增加的数据的个数
    int updateGet2DStats(const std::string &ID1, const std::string &ID2, double t1, double v1,
                         double *result, bool isTypediff = false);

    // 更新指定流的一维,二维信息, 将一维的[ weight,mean,std]和二维的[radius,magnitude,cov,pcc]返回
    // 参数分别是: 流的ID, 时间戳, 统计数据, 结果数组的指针, 最后一个如果设置true,就用时间戳作为统计数据
    // 返回增加的数据的个数
    int updateGet1D2DStats(const std::string &ID1, const std::string &ID2, double t1,
                           double v1, double *result, bool isTypediff = false) {
        int offset = updateGet1DStats(ID1, t1, v1, result, isTypediff);
        return offset + updateGet2DStats(ID1, ID2, t1, v1, result + offset, isTypediff);
    }

    // 析构函数, 将维护的incStat 的指针的集合指向的值, 全部释放掉
    ~IncStatDB() {
//        std::fprintf(stderr, "the number of incStat is: %d\n", stats.size());
//        int ans = 0;
//        int m = 0;
        for (auto it:stats) {
//            ans += it.second->covs.size();
//            if (it.second->covs.size() > m)m = it.second->covs.size();
            delete it.second;
        }
//        std::fprintf(stderr, "the number of incStatCov is %d\n", ans);
//        std::fprintf(stderr, "the max number of incStatCov is %d\n", m);
    }
};

/**
 * NetStat 类维护着当前的网络统计信息, 包括主机,分组抖动, 网络信道等统计信息
 * 负责将包生成统计的实例向量
 */

class NetStat {
private:
    // 时间窗口
    std::vector<double> lambdas;
    // 统计四类流的信息,
    //1. HT_jit: 主机与主机之间的抖动统计 只统计1维 (3个特征)
    //2. HT_MI: MAC-IP发送流的关系统计  只统计1维 (3个特征)
    //3. HT_H: 维护源主机发送流的一维带宽统计和与目的主机发送流之间的二维统计
    //4. HT_Hp: 维护源主机端口发送流的一维带宽统计和与目的主机端口发送流之间的二维统计 (7个特征), 这个与上面的HT_H不同是键值是ip+port, 考虑每个端口
    IncStatDB *HT_jit = nullptr, *HT_MI = nullptr, *HT_H = nullptr, *HT_Hp = nullptr;

public:
    // 构造器, 参数是lambdas
    NetStat(const std::vector<double> &l);

    // 无参构造器, 使用默认的lambdas
    NetStat();

    // 主要的调用函数, 传进去一个包信息, 返回对应的统计向量
    // 参数分别是: 源MAC,目的MCA,源IP, IP协议类型, 目的IP, 目的IP协议类型, 数据包大小, 数据包时间戳
    int updateAndGetStats(const std::string &srcMAC, const std::string &dstMAC,
                          const std::string &srcIP, const std::string &srcProtocol,
                          const std::string &dstIP, const std::string &dstProtocol,
                          double datagramSize, double timestamp, double *result);

    // 返回生成的统计实例向量的维度, 当前是每个lambda对应20个特征
    int getVectorSize() { return lambdas.size() * 20; }

    // 析构函数, delete掉 new 的四个实例
    ~NetStat() {
        delete HT_H;
        delete HT_Hp;
        delete HT_MI;
        delete HT_jit;
    }
};



#endif //KITSUNE_CPP_NETSTAT_H
