//
// Created by Yang Bo on 2020/5/15.
//

#include "../include/neuralnet.h"
#include "test.h"
#include <cmath>


using namespace std;

// 测试Dense 全连接网络的类

// 线性激活函数
inline double active1(double x) { return x; }

// 对应的导数
inline double active1D(double x) { return 1; }

// 需要拟合的函数
inline double func(double x) {
    return 7.0 * sin(0.75 * x) + 0.5 * x;
}

void testDense() {
    int hidden_neuron_num = 20; //隐层的神经元的个数
    int epoch = 2000; // 迭代次数
    double max_X = 10.5;
    double min_X = -10.5;
    int train_num = 1000; //训练的数据个数

    // 构建一个两层的全连接网络, 拟合一个三角函数
    auto layer1 = new Dense(1, hidden_neuron_num, sigmoid, sigmoidDerivative,0.002);
    auto layer2 = new Dense(hidden_neuron_num, 1, active1, active1D,0.002);

    auto *train_x = new double[train_num];
    auto *train_y = new double[train_num];
    // 生成测试数据
    for (int i = 0; i < train_num; ++i) {
        train_x[i] = rand_uniform(min_X, max_X);
        // 在y值加入噪声
        train_y[i] = func(train_x[i]) + rand_uniform(-0.5, 0.5);
    }
    double tmp[20];
    FILE *lossF = fopen("loss.txt", "w");

    // 训练
    for (int e = 0; e < epoch; ++e) {
        if(e%100==99)printf("epoch: %d/%d\n", e+1, epoch);
        double predict;
        for (int i = 0; i < train_num; ++i) {
            double input = (train_x[i] - min_X) / (max_X - min_X); // 0-1归一化
            layer1->feedForward(&input, tmp, true);
            layer2->feedForward(tmp, &predict, true);
            double loss = (predict - train_y[i]) * (predict - train_y[i]); // mse
            fprintf(lossF, "%.15f\n", loss);

            // 反向传播误差
            tmp[0] = train_y[i] - predict;
            layer2->BackPropagation(tmp);
            layer1->BackPropagation(tmp);

        }
    }
    fclose(lossF);

    // 将数据保存, 可视化作图分析
    FILE *trainF = fopen("train.txt", "w");
    for (int i = 0; i < train_num; ++i) {
        fprintf(trainF, "%.15f,%.15f\n", train_x[i], train_y[i]);
    }
    fclose(trainF);

    // 测试
    printf("testing...\n");
    auto test_x = new double[300];
    double now = min_X;
    int test_num = 0;
    while (now <= max_X) { // 生成步长为0.1的测试数据
        test_x[test_num++] = now;
        now += 0.1;
    }
    // 测试的时候将数据保存
    FILE *testF = fopen("test.txt", "w");
    double predict;
    for (int i = 0; i < test_num; ++i) {
        double input = (test_x[i] - min_X) / (max_X - min_X); // 0-1 归一化
        layer1->feedForward(&input, tmp);
        layer2->feedForward(tmp, &predict);
        fprintf(testF, "%.15f,%.15f\n", test_x[i], predict);
    }
    fclose(testF);

    delete layer1;
    delete layer2;
    delete[] train_x;
    delete[] train_y;
    delete[] test_x;
}