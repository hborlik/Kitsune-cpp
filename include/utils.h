//
// Created by Yang Bo on 2020/5/9.
//

#ifndef KITSUNE_CPP_UTILS_H
#define KITSUNE_CPP_UTILS_H

/**
 *  一些辅助的工具类或者函数
 */

#include <cstdio>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>


// 将pcap文件转为tsv,并且返回tsv文件的指针
FILE *pcap2tcv(const char *);


/*
 *  TvsReader , 负责读取tsv,csv格式的类, 使用文件名初始化, 对于每行可以通过列的id读取
 */
class TsvReader {
private:
    std::FILE *fp;
    char *buffer;
    std::vector<int> id; // 当前buffer里面第i列的位置(从0开始)
    char delimitor;  // 当前分隔符

    // 常量, 缓冲区大小
    static const int BufferSize = 3000;
public:
    // 构造器, 参数为文件的名字和分隔符, 默认是tsv文件(分隔符为'\t')
    TsvReader(const char *filename, char d = '\t') {
        fp = nullptr;
        fp = std::fopen(filename, "r");
        if (fp == nullptr) {
            std::fprintf(stderr, "\nTsvReader: File name is invalid!\n");
            throw -1;
        }
        delimitor = d;
        buffer = new char[BufferSize];
    }

    //构造器, 参数为文件指针和分隔符, 默认是tsv文件(分隔符为'\t')
    TsvReader(FILE *_fp, char d = '\t') {
        fp = _fp;
        if (fp == nullptr) {
            std::fprintf(stderr, "\nTsvReader: File pointer is invalid!\n");
            throw -1;
        }
        delimitor = d;
        buffer = new char[BufferSize];
    }

    // 读取并预处理下一行, 返回当前行的列数. 如果读到了最后一行, 返回0
    int nextLine();

    // 将第col列变成string返回
    std::string getString(int col);

    // 将col列变成int返回
    inline int getInt(int col) {
        int ans = 0;
        int now = id[col];
        while (buffer[now] >= '0' && buffer[now] <= '9')
            ans = (ans << 3) + (ans << 1) + buffer[now++] - '0';
        return ans;
    }

    // 将第col列变成double返回
    inline double getDouble(int col) {
        return strtod(buffer + id[col], nullptr);
    }

    // 判断第col列有没有值
    inline bool hasValue(int col) {
        int now = id[col];
        return buffer[now] != delimitor && buffer[now] != '\r' && buffer[now] != '\n' && buffer[now] != '\0';
    }

    // 析构函数, 释放空间
    ~TsvReader() {
        if (fp != nullptr) std::fclose(fp);
        delete[] buffer;
    }

};

/**
 *  生成csv/tsv文件的类
 */
class TsvWriter {
private:
    // 维护的文件指针
    FILE *fp = nullptr;
    // 分隔符
    char delimitor;
public:
    // 构造器,参数分别是 文件名, 分隔符
    TsvWriter(const char *filename, char d = '\t') {
        // 初始化文件指针
        fp = std::fopen(filename, "w");
        if (fp == nullptr) {
            std::fprintf(stderr, "\nTsvWriter: file open Error!\n");
            throw -1;
        }

        delimitor = d;
    }

    void write(const double *p, int n) {
        fprintf(fp, "%.10f", p[0]);
        for (int i = 1; i < n; ++i)fprintf(fp, "%c%.16f", delimitor, p[i]);
        fprintf(fp, "\n");
    }

    ~TsvWriter() { std::fclose(fp); }
};


/**
 *  一系列常见的激活函数
 */

inline double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

inline double ReLU(double x) {
    return x < 0 ? 0 : x;
}

inline double pReLU(double x) {
    const static double alpha = 0.01;
    return x < 0 ? alpha * x : x;
}

inline double ELU(double x) {
    const static double alpha = 0.01;
    return x < 0 ? alpha * (std::exp(x) - 1) : x;
}


/**
 * 一些回归评价指标
 */

//均方误差
inline double MSE(const double *a, const double *b, int n) {
    if (n <= 0)return 0;
    double sum = 0;
    for (int i = 0; i < n; ++i) {
        double tmp = a[i] - b[i];
        sum += tmp * tmp;
    }
    return sum / n;
}

// 均方根误差
inline double RMSE(const double *a, const double *b, int n) {
    if (n <= 0)return 0;
    double sum = 0;
    for (int i = 0; i < n; ++i) {
        double tmp = a[i] - b[i];
        sum += tmp * tmp;
    }
    return std::sqrt(sum / n);
}

// 平均绝对误差
inline double MAE(const double *a, const double *b, int n) {
    if (n <= 0)return 0;
    double sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += std::fabs(a[i] - b[i]);
    }
    return sum / n;
}

/**
 *  生成随机数据的函数
 */

// 均匀分布
inline double rand_uniform(double _min, double _max) {
    std::srand(std::time(NULL));
    return rand() / (RAND_MAX + 0.1) * (_max - _min) + _min;
}

#endif //KITSUNE_CPP_UTILS_H
