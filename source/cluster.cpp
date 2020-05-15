//
// Created by Yang Bo on 2020/5/11.
//

#include "../include/cluster.h"

Cluster::Cluster(int size) {
    n = size;
    sum = new double[n];
    sum1 = new double[n];
    sum2 = new double[n];
    num = 0;
    C = new double *[n];
    for (int i = 0; i < n; ++i)C[i] = new double[n];
    tmp = new double[n];

    // 初始化变量
    for (int i = 0; i < n; ++i)sum[i] = 0;
    for (int i = 0; i < n; ++i)sum1[i] = 0;
    for (int i = 0; i < n; ++i)sum2[i] = 0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)C[i][j] = 0;
}

void Cluster::update(const double *x) {
    ++num;
    for (int i = 0; i < n; ++i) {
        sum[i] += x[i];
        tmp[i] = x[i] - sum[i] / num;
        sum1[i] += tmp[i];
        sum2[i] += tmp[i] * tmp[i];
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            C[i][j] += tmp[i] * tmp[j];
        }
    }
}

std::vector<std::vector<int> > *Cluster::getFeatureMap(int maxSize) {
    // 获取每个特征的 每个值减去均值的平方和 的 平方根 (计算相关系数的分母)
    // 保存在tmp里面.
    for (int i = 0; i < n; ++i) {
        if (sum2[i] <= 0)tmp[i] = 0;
        else tmp[i] = std::sqrt(sum2[i]);
    }
    std::vector<DisNode> dis;
    dis.clear();
    // 生成距离矩阵(三元组表示),只获取一半矩阵即可
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            double l = tmp[i] * tmp[j];
            if (l <= 0)l = 1e-20;
            l = 1 - C[i][j] / l;
            if (l < 0)l = 0;
            dis.emplace_back(i, j, l);
        }
    }
    // 将矩阵按照距离排序, 每次找到距离最小的两个簇合并
    std::sort(dis.begin(), dis.end());

    std::vector<ClusterNode*> leaf; // 叶节点
    leaf.clear();
    for (int i = 0; i < n; ++i)leaf.push_back(new ClusterNode(i, 1));
    int now_id = n;
    for (auto edge: dis) {
        ClusterNode *root1 = leaf[edge.id1]->getRoot();
        ClusterNode *root2 = leaf[edge.id2]->getRoot();
        // 当两个不在一个簇的时候,就合并,生成一个新的节点
        if (root1 != root2) {
            auto *root = new ClusterNode(now_id++, root1->size + root2->size);
            root1->fa = root;
            root2->fa = root;
            root->lson = root1;
            root->rson = root2;
        }
    }
    // 合并之后,所有的一定在一棵树上,也就是获得了树状图
    ClusterNode *root = leaf.back()->getRoot();
    // 将树状图切开,分成多个簇,每个簇不超过maxSize;
    auto *ans = new std::vector<std::vector<int> >();
    root->getSon(ans, maxSize);
    // 释放树状图的内存
    delete root;
    return ans;
}

Cluster::~Cluster() {
    delete[] sum;
    delete[] sum1;
    delete[] sum2;
    for (int i = 0; i < n; ++i)delete[] C[i];
    delete[] C;
    delete[] tmp;
}

void ClusterNode::getSon(std::vector<std::vector<int>> *result, int max_size) {
    if (size <= max_size) { // 如果当前的子树个数满足要求,就直接全部放在一个簇
        std::vector<int> ans;
        ans.clear();
        pushLeaf(ans);
        result->push_back(ans);
    }else{ // 否则就分成两个簇,递归判断
        // 左子树
        lson->getSon(result,max_size);
        // 右子树
        rson->getSon(result,max_size);
    }
}

void ClusterNode::pushLeaf(std::vector<int> &result) {
    if (lson == nullptr && rson == nullptr) {//leaf
        result.push_back(id);
    } else {
        if (lson != nullptr)lson->pushLeaf(result);
        if (rson != nullptr)rson->pushLeaf(result);
    }
}
