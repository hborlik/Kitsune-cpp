//
// Created by Yang Bo on 2020/5/9.
//

#include "../include/neuralnet.h"


Dense::Dense(int inSize, int outSize, double (*activationFunc)(double), double (*activationDerivativeFunc)(double),
             double lr) {
    n_in = inSize;
    n_out = outSize;
    activation = activationFunc;
    activationDerivative = activationDerivativeFunc;
    learning_rate = lr;
    inputValue = new double[n_in];
    outputValue = new double[n_out];
    bias = new double[n_out];
    W = new double *[n_in];// n_in行, n_out列
    for (int i = 0; i < n_in; ++i)W[i] = new double[n_out];

    double val = 1.0 / n_out;
    // 均匀分布初始化权值
    for (int i = 0; i < n_in; ++i) {
        for (int j = 0; j < n_out; ++j)W[i][j] = rand_uniform(-val, val);
    }
    for (int i = 0; i < n_out; ++i)bias[i] = 0;
}

Dense::~Dense() {
    delete[] bias;
    delete[] inputValue;
    delete[] outputValue;
    for (int i = 0; i < n_in; ++i)delete[] W[i];
    delete[] W;
}

void Dense::feedForward(const double *input, double *output, bool saveValue) {
    for (int i = 0; i < n_out; ++i) {
        output[i] = bias[i];
        for (int j = 0; j < n_in; ++j) {
            output[i] += W[j][i] * input[j];
        }
        output[i] = activation(output[i]);
    }
    if (saveValue) {
        std::memcpy(inputValue, input, sizeof(double) * n_in);
        std::memcpy(outputValue, output, sizeof(double) * n_out);
    }
}

// SGD
void Dense::BackPropagation(double *g) {
    for (int i = 0; i < n_out; ++i)outputValue[i] = g[i] * activationDerivative(outputValue[i]);

    // 计算传播到上一层的误差
    for (int i = 0; i < n_in; ++i) {
        g[i] = 0;
        for (int j = 0; j < n_out; ++j) {
            g[i] += W[i][j] * outputValue[j];
        }
    }

    // 更新阈值, 顺便将learning_rate乘上保存, 更新权重的时候不用再计算了
    for (int i = 0; i < n_out; ++i) {
        outputValue[i] *= learning_rate;
        bias[i] += outputValue[i];
    }

    // 更新权值
    for (int i = 0; i < n_in; ++i) {
        for (int j = 0; j < n_out; ++j) {
            W[i][j] += inputValue[i] * outputValue[j];
        }
    }
}


// 构造函数, 参数是显层,隐层的个数
AE::AE(int v_sz, int h_sz, double _learning_rate) {
    visible_size = v_sz;
    hidden_size = h_sz;

    // 初始化两层神经网络, 都是用sigmoid激活函数
    encoder = new Dense(visible_size, hidden_size, sigmoid, sigmoidDerivative, _learning_rate);
    decoder = new Dense(hidden_size, visible_size, sigmoid, sigmoidDerivative, _learning_rate);

    // 初始化临时变量的数组
    tmp_x = new double[visible_size];
    tmp_z = new double[visible_size];
    tmp_y = new double[hidden_size];
    // tmp_g是传播梯度用的缓冲区, 所以大小为每一层的最大值
    tmp_g = new double[std::max(hidden_size, visible_size)];

    // 初始化归一化需要的数组
    max_v = new double[visible_size];
    min_v = new double[visible_size];
    for (int i = 0; i < visible_size; ++i) {
        min_v[i] = 1e20;
        max_v[i] = -1e20;
    }
}

// 析构函数
AE::~AE() {
    delete encoder;
    delete decoder;
    delete[] tmp_x;
    delete[] tmp_y;
    delete[] tmp_z;
    delete[] max_v;
    delete[] min_v;
}


// 重建, 返回重建的值
double AE::reconstruct(const double *x) {
    normalize(x); // 首先归一化, 保存在tmp_x里

    encoder->feedForward(tmp_x, tmp_y); // 编码,保存在tmp_y里面

    decoder->feedForward(tmp_y, tmp_z); // 解码,保存在tmp_z里面

    return RMSE(tmp_x, tmp_z, visible_size);
}

// 训练
double AE::train(const double *x) {
    normalize(x); // 0-1正则化, 保存在tmp_x里面
    // 正向跑一遍, 把saveValue参数设为true,准备反向传播误差
    encoder->feedForward(tmp_x, tmp_y, true);
    decoder->feedForward(tmp_y, tmp_z, true);

    // 误差保存在 tmp_g里面
    for (int i = 0; i < visible_size; ++i)tmp_g[i] = tmp_x[i] - tmp_z[i];
    // 反向传播误差
    decoder->BackPropagation(tmp_g);
    encoder->BackPropagation(tmp_g);

    return RMSE(tmp_x, tmp_z, visible_size);
}

// 0-1归一化, 结果保存在tmp_x里
void AE::normalize(const double *x) {
    for (int i = 0; i < visible_size; ++i) {
        min_v[i] = std::min(x[i], min_v[i]);
        max_v[i] = std::max(x[i], max_v[i]);
        tmp_x[i] = (x[i] - min_v[i]) / (max_v[i] - min_v[i] + 1e-13);
    }
}
