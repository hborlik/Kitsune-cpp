//
// Created by Yang Bo on 2020/5/11.
//

#include "../include/KitNET.h"

void KitNET::init() {
    if (kitNetParam == nullptr) {
        fprintf(stderr, "KITNET: KitNETParam must not be null\n");
        throw -1;
    }
    // 需要聚类得到映射数组
    if (featureMap == nullptr) {
        if (kitNetParam->cluster == nullptr) {
            fprintf(stderr, "KITNET: the Cluster object must not be null\n");
        }
        // 聚类获得特征映射
        featureMap = kitNetParam->cluster->getFeatureMap(kitNetParam->max_size);
    }

    // 初始化自编码器
    ensembleLayer = new AE *[featureMap->size()];
    for (int i = 0; i < featureMap->size(); ++i) {
        ensembleLayer[i] = new AE(featureMap->at(i).size(),
                                  featureMap->at(i).size() * kitNetParam->ensemble_vh_rate,
                                  kitNetParam->ensemble_learning_rate);
    }
    outputLayer = new AE(featureMap->size(), featureMap->size() * kitNetParam->output_vh_rate,
                         kitNetParam->output_learning_rate);

    // 初始化自编码器输入参数的缓冲区
    ensembleInput = new double *[featureMap->size()];
    for (int i = 0; i < featureMap->size(); ++i) {
        ensembleInput[i] = new double[featureMap->at(i).size()];
    }
    outputInput = new double[featureMap->size()];
    /*
    for (auto &i : *featureMap) {
        fprintf(stderr, "[");
        for (int j : i)fprintf(stderr, "%d,", j);
        fprintf(stderr, "]\n");
    }
    */

    delete kitNetParam;
    kitNetParam = nullptr;
}


// 析构函数
KitNET::~KitNET() {
    delete kitNetParam; // 如果是null的话, delete null 没有影响, 所以直接delete即可
    for (int i = 0; i < featureMap->size(); ++i) {
        delete ensembleLayer[i];
        delete[] ensembleInput[i];
    }
    delete outputLayer;
    delete[] outputInput;
    delete featureMap;
}

double KitNET::train(const double *x) {
    if (featureMap == nullptr) { // 如果特征映射还没初始化
        --kitNetParam->fm_train_num;
        // 更新聚类里面维护的数值
        kitNetParam->cluster->update(x);
        // 如果训练特征映射的个数达到了设定值, 就初始化自编码器
        if (kitNetParam->fm_train_num == 0)init();
        return 0;
    } else {// 训练自编码器
        for (int i = 0; i < featureMap->size(); ++i) {
            // 将对应的特征向量复制到缓冲区
            for (int j = 0; j < featureMap->at(i).size(); ++j) {
                ensembleInput[i][j] = x[featureMap->at(i).at(j)];
            }
            outputInput[i] = ensembleLayer[i]->train(ensembleInput[i]);
        }
        // 训练输出层, 返回重建误差
        return outputLayer->train(outputInput);
    }
}

double KitNET::execute(const double *x) {
    if (featureMap == nullptr) { // 如果特征映射还没初始化
        fprintf(stderr, "KitNET: the feature map is not initialized!!\n");
        throw -1;
    }
    for (int i = 0; i < featureMap->size(); ++i) {
        // 将对应的特征向量复制到缓冲区
        for (int j = 0; j < featureMap->at(i).size(); ++j) {
            ensembleInput[i][j] = x[featureMap->at(i).at(j)];
        }
        // 运行这个小自编码器, 将重建误差保存当做输出层的输入
        outputInput[i] = ensembleLayer[i]->reconstruct(ensembleInput[i]);
    }
    return outputLayer->reconstruct(outputInput);
}
