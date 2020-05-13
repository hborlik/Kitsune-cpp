//
// Created by Yang Bo on 2020/5/9.
//

#include "../include/FeatureExtractor.h"


// netStat使用默认的时间窗口的构造函数, 默认读取的tsv的包特征文件
FE::FE(const char *filename, FileType ft) {
    fileType = ft;
    netStat = new NetStat();
    if (fileType == PacketTSV || fileType == FeatureTSV) {// 分隔符为制表符
        tsvReader = new TsvReader(filename, '\t');
    } else if (fileType == PacketCSV || fileType == FeatureCSV) {// 分隔符为','
        tsvReader = new TsvReader(filename, ',');
    } else if (fileType == PCAP) { // 需要转为tsv类的文件
        tsvReader = new TsvReader(pcap2tcv(filename));
    }
    // 读取的包特征文件, 需要使用netStat统计
    if (fileType == PCAP || fileType == PacketCSV || fileType == PacketTSV) {
        // 有表头, 所以需要先读取了表头
        tsvReader->nextLine();
    }
}

// 指定时间窗口的构造函数, 默认读取的tsv的包特征文件
FE::FE(const char *filename, const std::vector<double> &lambdas, FileType ft) {
    fileType = ft;
    netStat = new NetStat(lambdas);
    if (fileType == PacketTSV || fileType == FeatureTSV) {// 分隔符为制表符
        tsvReader = new TsvReader(filename, '\t');
    } else if (fileType == PacketCSV || fileType == FeatureCSV) {// 分隔符为','
        tsvReader = new TsvReader(filename, ',');
    } else if (fileType == PCAP) { // 需要转为tsv类的文件
        tsvReader = new TsvReader(pcap2tcv(filename));
    }
    // 读取的包特征文件, 需要使用netStat统计
    if (fileType == PCAP || fileType == PacketCSV || fileType == PacketTSV) {
        // 有表头, 所以需要先读取了表头
        tsvReader->nextLine();
    }
}


// 从reader里面读取一行包的特征, 传给netstat获取下一组实例的向量,
// 如果成功就返回向量的个数, 不然就返回0
int FE::nextVector(double *result) {
    int cols = tsvReader->nextLine();
    if (cols == 0)return 0;
    if (fileType == FeatureTSV || fileType == FeatureCSV) { // 如果直接读取向量信息,就直接读double
        int num = getVectorSize();
        if (cols < num)return 0;
        for (int i = 0; i < num; ++i)result[i] = tsvReader->getDouble(i);
        return num;
    } else { // 用netStat增量统计
        std::string srcIP, dstIP;
        if (tsvReader->hasValue(4)) {// Ipv4
            srcIP = tsvReader->getString(4);
            dstIP = tsvReader->getString(5);
        } else { // Ipv6
            srcIP = tsvReader->getString(17);
            dstIP = tsvReader->getString(18);
        }
        std::string srcport, dstport;
        if (tsvReader->hasValue(6)) {//tcp
            srcport = tsvReader->getString(6);
            dstport = tsvReader->getString(7);
        } else if (tsvReader->hasValue(8)) { // udp
            srcport = tsvReader->getString(8);
            dstport = tsvReader->getString(9);
        } else { // 既不是tcp,又不是udp,可能是arp或icmp等1,2层的包了
            if (tsvReader->hasValue(10)) { // icmp
                srcport = dstport = "icmp";
            } else if (tsvReader->hasValue(12)) { // arp
                srcport = dstport = "arp";
                // 用arp包内的源ip与目的ip作为ip信息
                srcIP = tsvReader->getString(14);
                dstIP = tsvReader->getString(16);
            } else { // 其他协议, 用源和目的MAC赋值
                srcIP = tsvReader->getString(2);
                dstIP = tsvReader->getString(3);
            }
        }
        return netStat->updateAndGetStats(tsvReader->getString(2), tsvReader->getString(3),
                                          srcIP, srcport, dstIP, dstport, tsvReader->getDouble(1),
                                          tsvReader->getDouble(0), result);
    }
}
