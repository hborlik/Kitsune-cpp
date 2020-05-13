//
// Created by Yang Bo on 2020/5/11.
//

#ifndef KITSUNE_CPP_CLUSTER_H
#define KITSUNE_CPP_CLUSTER_H

#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

/**
 *  一个辅助的类, 负责维护特征之间的关联信息, 进行层次聚类
 */
class Cluster {
private:
    // 实例向量的规模
    int n;

    // 已经处理的实例的个数
    int num;

    // 协方差矩阵
    double **C = nullptr;

    // 每个值的线性和
    double *sum = nullptr;

    // 每个值减去均值的 线性和
    double *sum1 = nullptr;

    // 每个值减去均值的 平方和
    double *sum2 = nullptr;

    // 计算时的临时变量
    double *tmp = nullptr;

public:
    // 构造函数
    Cluster(int size);

    // 析构函数
    ~Cluster();

    // 增加一个向量更新
    void update(const double *x);

    // 生成映射信息
    std::vector<std::vector<int> > *getFeatureMap(int maxSize);
};



/**
 *  层次聚类需要的辅助结构体
 */

// 距离矩阵的稀疏表示,直接用三元组
struct DisNode {
    int id1, id2;
    double distance;

    DisNode(int _i1, int _i2, double _d) : id1(_i1), id2(_i2), distance(_d) {}

    // 重载小于号
    bool operator<(const DisNode &other) const {
        return distance < other.distance;
    }
};


// 层次聚类树状图节点类, 是个二叉树
struct ClusterNode {
    int id; // 节点编号
    int size; //子树大小

    // 父节点
    ClusterNode *fa = nullptr;
    // 左右子树
    ClusterNode *lson = nullptr;
    ClusterNode *rson = nullptr;

    // 构造函数
    ClusterNode(int _id, int _size) : id(_id), size(_size) {}

    // 析构函数
    ~ClusterNode() {
        if (lson != nullptr)delete lson;
        if (rson != nullptr)delete rson;
    }

    // 获取当前节点的根节点
    ClusterNode *getRoot() {
        ClusterNode *now = this;
        while (now->fa != nullptr)now = now->fa;
        return now;
    }

    // 通过指定的maxsize, 将树状图分割(并且保存到result里面.
    void getSon(std::vector<std::vector<int>> *result, int max_size);

    // 将子树里面所有的叶节点保存到result里面.
    void pushLeaf(std::vector<int> &result);
};

#endif //KITSUNE_CPP_CLUSTER_H
