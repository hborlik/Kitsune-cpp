//
// Created by Yang Bo on 2020/5/15.
//

#include "../include/featureExtractor.h"
#include "../include/neuralnet.h"
#include "../include/kitNET.h"
#include "test.h"

// 简单的测试Kitsune的例子

void kitsuneExample() {
    const char *filename = "D:\\Dataset\\KITSUNE\\Mirai\\Mirai_pcap.pcap.tsv";
    const int FM_train_num = 5000; // 训练特征映射需要的个数
    const int AD_train_num = 50000; // 训练异常检测模块需要的个数
    const int KitNET_train_num = AD_train_num + FM_train_num;  // 训练KitNET需要的个数
    const int max_AE = 10; // 自编码器最大的规模

    auto fe = new FE(filename, PacketTSV);  // 初始化特征提取模块

    int sz = fe->getVectorSize(); // 获取特征提取需要的个数

    auto kitNET = new KitNET(sz, max_AE, FM_train_num); // 初始化kitNET模块

    auto *x = new double[sz]; // 初始化存储输入特征向量的缓冲区

    FILE *fp = fopen("RMSE.txt", "w");
    int now_packet = 0;
    while (fe->nextVector(x)) {
        ++now_packet;
        if (now_packet <= KitNET_train_num)
            fprintf(fp, "%.15f\n", kitNET->train(x));
        else
            fprintf(fp, "%.15f\n", kitNET->execute(x));

        if (now_packet % 1000 == 0)printf("%d\n", now_packet);
    }

    printf("total packets is %d\n", now_packet);
    fclose(fp);
    delete[] x;
    delete fe;
    delete kitNET;
}