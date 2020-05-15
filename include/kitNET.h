//
// Created by Yang Bo on 2020/5/11.
//

#ifndef KITSUNE_CPP_KITNET_H
#define KITSUNE_CPP_KITNET_H

#include <vector>
#include "neuralnet.h"
#include "cluster.h"


/**
*  KitNET 创建集成层, 输出层的时候需要的参数
*/
struct KitNETParam {
    int max_size; // 集成层编码器最大的大小

    // 训练特征映射还需要的样本数
    int fm_train_num = 0;

    // 集成层的自编码器显隐层比例
    double ensemble_vh_rate;

    // 输出层自编码器显隐层比例
    double output_vh_rate;

    // 集成层学习率
    double ensemble_learning_rate;

    // 输出层学习率
    double output_learning_rate;

    // 聚类的辅助类
    Cluster *cluster = nullptr;

    // 析构函数
    ~KitNETParam() {
        if (cluster != nullptr)delete cluster;
    }
};


/**
 *  KitNET 类, 主要是通过聚类,自编码器实现了KitNET算法
 *
 */

class KitNET {
private:
    // 特征映射, 保存特征实例向量每个元素映射到的自编码器.
    std::vector<std::vector<int> > *featureMap = nullptr;

    // 集成层的自编码器
    AE **ensembleLayer = nullptr;

    // 输出层的自编码器
    AE *outputLayer = nullptr;

    // 集成层输入的向量
    double **ensembleInput = nullptr;

    // 输出层的输入向量
    double *outputInput = nullptr;

    // 初始化自编码器需要的参数
    KitNETParam *kitNetParam = nullptr;

    // 初始化KitNET, 根据特征参数等初始化.
    void init();

public:

    // 构造器有两种, 一种是直接提供特征映射.  另一种是根据参数来训练特征映射,训练之后才进行

    // 构造器1, 参数分别是:
    // 1. 特征映射的数组指针
    // 2. 映射的数组指针集成层自编码器显层/隐层比例 3. 输出层自编码器显层/隐层比例, (都默认0.75)
    // 4. 集成层的学习率   5. 输出层的学习率  (都默认 0.1 )
    KitNET(std::vector<std::vector<int> > *fm, double ensemble_vh_rate = 0.75, double output_vh_rate = 0.75,
           double ensemble_learning_rate = 0.1, double output_learning_rate = 0.1) {
        featureMap = fm;
        kitNetParam = new KitNETParam;
        kitNetParam->ensemble_learning_rate = ensemble_learning_rate;
        kitNetParam->ensemble_vh_rate = ensemble_vh_rate;
        kitNetParam->output_vh_rate = output_vh_rate;
        kitNetParam->output_learning_rate = output_learning_rate;
        init(); // 直接创建自编码器
    }


    // 构造器2,参数分别是:
    // 1. 输入实例的规模
    // 2. 集成层每个自编码器最大规模,
    // 3. 训练特征映射需要的实例的个数.
    // 4. 集成层自编码器显层/隐层比例 5. 输出层自编码器显层/隐层比例, (都默认0.75)
    // 6. 集成层的学习率   7. 输出层的学习率  (都默认 0.1 )
    KitNET(int n, int maxAE, int fm_train_num, double ensemble_vh_rate = 0.75, double output_vh_rate = 0.75,
           double ensemble_learning_rate = 0.1, double output_learning_rate = 0.1) {
        kitNetParam = new KitNETParam;
        kitNetParam->ensemble_learning_rate = ensemble_learning_rate;
        kitNetParam->ensemble_vh_rate = ensemble_vh_rate;
        kitNetParam->output_vh_rate = output_vh_rate;
        kitNetParam->output_learning_rate = output_learning_rate;
        kitNetParam->max_size = maxAE;
        kitNetParam->fm_train_num = fm_train_num;
        kitNetParam->cluster = new Cluster(n);
    }


    // 析构函数
    ~KitNET();

    // 训练, 返回重建误差. 如果是在训练FM模块,返回0
    double train(const double *x);

    // 前项传播, 返回当前数据的重建误差
    double execute(const double *x);


};


#endif //KITSUNE_CPP_KITNET_H
