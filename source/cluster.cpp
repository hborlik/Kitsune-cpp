/**
 * @brief Created by Yang Bo on 2020/5/11.
 * 
 * modified: 02/06/2023: comments translated to english
 */
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

    // Initialize variables
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
    // Get the square root of the sum of the squares of each value minus the mean for each feature
    // (calculates the denominator of the correlation coefficient)
    // save in tmp.
    for (int i = 0; i < n; ++i) {
        if (sum2[i] <= 0)tmp[i] = 0;
        else tmp[i] = std::sqrt(sum2[i]);
    }
    std::vector<DisNode> dis;
    dis.clear();
    // Generate a distance matrix (triple representation), only get half of the matrix
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            double l = tmp[i] * tmp[j];
            if (l <= 0)l = 1e-20;
            l = 1 - C[i][j] / l;
            if (l < 0)l = 0;
            dis.emplace_back(i, j, l);
        }
    }
    // Sort the matrix according to the distance, and find the two clusters with the smallest distance to merge each time
    std::sort(dis.begin(), dis.end());

    std::vector<ClusterNode*> leaf; // leaf node
    leaf.clear();
    for (int i = 0; i < n; ++i)leaf.push_back(new ClusterNode(i, 1));
    int now_id = n;
    for (auto edge: dis) {
        ClusterNode *root1 = leaf[edge.id1]->getRoot();
        ClusterNode *root2 = leaf[edge.id2]->getRoot();
        // When the two are not in the same cluster, they are merged to generate a new node
        if (root1 != root2) {
            auto *root = new ClusterNode(now_id++, root1->size + root2->size);
            root1->fa = root;
            root2->fa = root;
            root->lson = root1;
            root->rson = root2;
        }
    }
    // After merging, all must be on one tree, that is, a dendrogram is obtained
    ClusterNode *root = leaf.back()->getRoot();
    // Cut the dendrogram into multiple clusters, and each cluster does not exceed maxSize;
    auto *ans = new std::vector<std::vector<int> >();
    root->getSon(ans, maxSize);
    // Free up the memory of the dendrogram
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
    if (size <= max_size) { 
        // If the current number of subtrees meets the requirements, put them all in one cluster directly
        std::vector<int> ans;
        ans.clear();
        pushLeaf(ans);
        result->push_back(ans);
    }else{
        // Otherwise, it is divided into two clusters, and the recursive judgment
        // left subtree
        lson->getSon(result,max_size);
        // right subtree
        rson->getSon(result,max_size);
    }
}

void ClusterNode::pushLeaf(std::vector<int> &result) {
    if (lson == nullptr && rson == nullptr) {
        result.push_back(id);
    } else {
        if (lson != nullptr)lson->pushLeaf(result);
        if (rson != nullptr)rson->pushLeaf(result);
    }
}
