//
// Created by Yang Bo on 2020/5/9.
//

#include "../include/Autoencoder.h"

// 构造函数, 参数是显层,隐层的个数
AE::AE(int v_sz, int h_sz, double _learning_rate) {
    visible_size = v_sz;
    hidden_size = h_sz;
    learning_rate = _learning_rate;
    // 所以w是 hidden_size行, visible_size 列
    w = new double *[hidden_size];
    // 使用 [-1/v_sz, 1/v_sz] 均匀分布, 初始化权值
    double val = 1.0 / v_sz;
    for (int i = 0; i < hidden_size; ++i) {
        w[i] = new double[visible_size];
        for (int j = 0; j < hidden_size; ++j)w[i][j] = rand_uniform(-val, val);
    }
    // 初始化两层的阈值都为0
    vbias = new double[visible_size];
    hbias = new double[hidden_size];
    for (int i = 0; i < visible_size; ++i)vbias[i] = 0;
    for (int i = 0; i < hidden_size; ++i)hbias[i] = 0;

    // 初始化临时变量的数组
    tmp_x = new double[visible_size];
    tmp_g = new double[visible_size];
    tmp_z = new double[visible_size];
    tmp_y = new double[hidden_size];

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
    delete[] hbias;
    delete[] vbias;
    // 将每一行的都释放掉
    for (int i = 0; i < hidden_size; ++i)delete[] w[i];
    delete[] w;
    delete[] tmp_x;
    delete[] tmp_y;
    delete[] tmp_z;
    delete[] tmp_g;
    delete[] max_v;
    delete[] min_v;
}


// 执行编码过程, 将tmp_x变换到隐层,临时保存在tmp_y里
void AE::encode() {
    for (int i = 0; i < hidden_size; ++i) {
        tmp_y[i] = hbias[i];
        for (int j = 0; j < visible_size; ++j)
            tmp_y[i] += w[i][j] * tmp_x[j];
        // 使用激活函数进行变换, 这里使用的sigmoid
        tmp_y[i] = sigmoid(tmp_y[i]);
    }
}

// 执行解码过程, 将隐藏tmp_y变换到输出层, 临时保存在tmp_z里
void AE::decode() {
    for (int i = 0; i < visible_size; ++i) {
        tmp_z[i] = vbias[i];
        for (int j = 0; j < hidden_size; ++j)
            tmp_z[i] += w[j][i] * tmp_y[j];
        tmp_z[i] = sigmoid(tmp_z[i]);
    }
}

// 重建, 返回重建的值
double AE::reconstruct(const double *x) {
    normalize(x); // 首先归一化
    encode();
    decode();
    return RMSE(tmp_x, tmp_z, visible_size);
}

// 训练
double AE::train(const double *x) {
    normalize(x);
    // 正向传播跑一遍
    encode();
    decode();

    // 输出层阈值
    for (int i = 0; i < visible_size; ++i) {
        vbias[i] += learning_rate * (tmp_x[i] - tmp_z[i]);
    }
    // 隐层阈值
    for (int i = 0; i < hidden_size; ++i) {
        tmp_g[i] = 0;
        for (int j = 0; j < visible_size; ++j) {
            tmp_g[i] += w[i][j] * (tmp_x[j] - tmp_z[j]);
        }
        tmp_g[i] *= tmp_y[i] * (1 - tmp_y[i]);
        hbias[i] += learning_rate * tmp_g[i];
    }
    // 连接权值
    for (int i = 0; i < hidden_size; ++i) {
        for (int j = 0; j < visible_size; ++j) {
            w[i][j] += learning_rate * (tmp_g[i] * tmp_x[j] + tmp_y[i] * (tmp_x[j] - tmp_z[j]));
        }
    }

    return RMSE(tmp_z, tmp_x, visible_size);
}

// 0-1归一化, 结果保存在tmp_x里
void AE::normalize(const double *x) {
    for (int i = 0; i < visible_size; ++i) {
        min_v[i] = std::min(x[i], min_v[i]);
        max_v[i] = std::max(x[i], max_v[i]);
        tmp_x[i] = (x[i] - min_v[i]) / (max_v[i] - min_v[i] + 1e-13);
    }
}

// 构造函数, 从文件里读取已经训练的模型
AE::AE(const char *filename) {
    FILE *fp = std::fopen(filename, "rb");
    if (fp == nullptr) {
        std::fprintf(stderr, "\nAE: file open Error!\n");
        throw -1;
    }
    std::fread(&visible_size, sizeof(visible_size), 1, fp);
    std::fread(&hidden_size, sizeof(hidden_size), 1, fp);
    std::fread(&learning_rate, sizeof(learning_rate), 1, fp);
    if (visible_size <= 0 || hidden_size <= 0) {
        std::fprintf(stderr, "\nAE: read parameter error!\n");
        throw -1;
    }
    // w是 hidden_size行, visible_size 列
    w = new double *[hidden_size];
    for (int i = 0; i < hidden_size; ++i) {
        w[i] = new double[visible_size];
        std::fread(w[i], sizeof(w[i][0]), visible_size, fp);
    }
    vbias = new double[visible_size];
    hbias = new double[hidden_size];
    std::fread(vbias, sizeof(vbias[0]), visible_size, fp);
    std::fread(hbias, sizeof(hbias[0]), hidden_size, fp);

    // 初始化临时变量的数组(这个不需要读取)
    tmp_x = new double[visible_size];
    tmp_g = new double[visible_size];
    tmp_z = new double[visible_size];
    tmp_y = new double[hidden_size];

    // 初始化归一化需要的数组
    max_v = new double[visible_size];
    min_v = new double[visible_size];
    std::fread(max_v, sizeof(max_v[0]), visible_size, fp);
    std::fread(min_v, sizeof(min_v[0]), visible_size, fp);
    std::fclose(fp);
}


// 将当前训练的模型保存在file里面, 保存的格式需要跟读取的格式一样
void AE::saveToFile(const char *filename) {
    FILE *fp = std::fopen(filename, "wb");
    if (fp == nullptr) {
        std::fprintf(stderr, "AE: file open Error!\n");
        throw -1;
    }
    std::fwrite(&visible_size, sizeof(visible_size), 1, fp);
    std::fwrite(&hidden_size, sizeof(hidden_size), 1, fp);
    std::fwrite(&learning_rate, sizeof(learning_rate), 1, fp);
    for (int i = 0; i < hidden_size; ++i) {
        std::fwrite(w[i], sizeof(w[i][0]), visible_size, fp);
    }
    std::fwrite(vbias, sizeof(vbias[0]), visible_size, fp);
    std::fwrite(hbias, sizeof(hbias[0]), hidden_size, fp);
    std::fwrite(max_v, sizeof(max_v[0]), visible_size, fp);
    std::fwrite(min_v, sizeof(min_v[0]), visible_size, fp);
    std::fclose(fp);
}