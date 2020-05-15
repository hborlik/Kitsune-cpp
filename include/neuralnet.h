//
// Created by Yang Bo on 2020/5/9.
//

#ifndef KITSUNE_CPP_NEURALNET_H
#define KITSUNE_CPP_NEURALNET_H

#include <cstdio>
#include <cstring>
#include "utils.h"


/**
 *  简单的全连接网络层
 */
class Dense {
private:
    int n_in;    // 输入规模

    int n_out;    // 输出的规模

    double **W = nullptr; // 连接权重

    double *bias = nullptr; // 阈值

    double (*activation)(double); // 激活函数的函数指针

    double (*activationDerivative)(double); // 激活函数的导数的函数指针 (参数是激活函数的函数值)

    double learning_rate; // 学习率

    double *inputValue = nullptr; //保存输入值的临时变量

    double *outputValue = nullptr; // 保存的输出值的临时变量

public:
    // 构造器, 参数是输入的神经元个数, 输出的神经元个数, 激活函数, 激活函数的导数, 学习率(默认0.1)
    Dense(int inSize, int outSize, double (*activationFunc)(double), double (*activationDerivativeFunc)(double),
          double lr = 0.1);

    ~Dense();

    // 前向传播, 第三个参数表示 是否保存输入输出值的临时变量. (只前向传播的时候就false, 传播完需要bp的时候一定要true)
    void feedForward(const double *input, double *output, bool saveValue = false);

    // 反向传播误差,并把传播给上一层的误差保存到g. g的容量需要为max(n_in,n_out)
    void BackPropagation(double *g);
};


/**
 *  自编码器类, 通过维护两个全连接层(编码器和解码器)
 */
class AE {
private:
    int visible_size; // 可见层的大小

    int hidden_size; // 隐层大小

    Dense *encoder = nullptr, *decoder = nullptr; // 两层神经网络,编码器和解码器

    double *min_v = nullptr, *max_v = nullptr; // 0-1归一化需要维护的最大值最小值

    double *tmp_x, *tmp_y, *tmp_z, *tmp_g; // 临时变量

    // 0-1归一化, 结果保存在tmp_x里
    void normalize(const double *x);

public:
    // 构造函数, 参数是显层,隐层的个数, 学习率, 默认0.01
    AE(int v_sz, int h_sz, double _learning_rate = 0.01);

    // 析构函数
    ~AE();

    // 重建, 返回重建的 均根误差
    double reconstruct(const double *x);

    // 训练, 返回重建的 均根误差
    double train(const double *x);

};


#endif //KITSUNE_CPP_NEURALNET_H
