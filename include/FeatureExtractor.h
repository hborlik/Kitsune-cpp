//
// Created by Yang Bo on 2020/5/9.
//

#ifndef KITSUNE_CPP_FEATUREEXTRACTOR_H
#define KITSUNE_CPP_FEATUREEXTRACTOR_H

#include <cstdio>
#include "utils.h"
#include "netStat.h"

/**
 *  特征提取的类
 *  目前可以做到:
 *  1. 从pcap提取特征, 获取实例向量
 *  2. 从包的tsv,csv文件读取特征,获取实例向量
 *  3. 直接从实例向量的tsv,csv文件中读取实例向量
 *  下一步做的:
 *  4. 在线抓包, 直接获取每个包的实例向量
 */


// 枚举类型, 定义的文件类型
enum FileType {
    PCAP, PacketTSV, PacketCSV, FeatureCSV, FeatureTSV, OnlineNetDevice
};

// 负责获取特征向量. 可以从pcap, tsv文件读取, 也可以直接从FeatureCSV等文件类型直接读取一行向量
class FE {
private:
    TsvReader *tsvReader = nullptr;
    NetStat *netStat = nullptr;
    FileType fileType; // 当前文件的类型
public:
    // netStat使用默认的时间窗口的构造函数, 默认读取的tsv的包特征文件
    FE(const char *filename, FileType ft = PacketTSV);

    // 指定时间窗口的构造函数, 默认读取的tsv的包特征文件
    FE(const char *filename, const std::vector<double> &lambdas, FileType ft = PacketTSV);

    // 析构函数
    ~FE() {
        delete tsvReader;
        if (netStat != nullptr)delete netStat;
    }

    // 获取下一个实例向量, 保存在result中
    int nextVector(double *result);

    // 返回每次生成的实例向量的大小
    inline int getVectorSize() { return netStat->getVectorSize(); }

};



#endif //KITSUNE_CPP_FEATUREEXTRACTOR_H
