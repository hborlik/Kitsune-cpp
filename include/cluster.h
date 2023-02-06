/**
 * @brief Created by Yang Bo on 2020/5/11.
 * 
 * modified: 02/06/2023: comments translated to english
 */
#ifndef KITSUNE_CPP_CLUSTER_H
#define KITSUNE_CPP_CLUSTER_H

#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

/**
 *  An auxiliary class, responsible for maintaining the association information between features,
 * and performing hierarchical clustering.
 */
class Cluster {
private:
    // the size of the instance vector
    int n;

    // The number of instances that have been processed
    int num;

    // covariance matrix
    double **C = nullptr;

    // linear sum of each value
    double *sum = nullptr;

    // linear sum of each value minus the mean
    double *sum1 = nullptr;

    // sum of squares of each value minus the mean
    double *sum2 = nullptr;

    // Temporary variable during calculation
    double *tmp = nullptr;

public:
    // Constructor
    Cluster(int size);

    // destructor
    ~Cluster();

    // Add a vector update
    void update(const double *x);

    // Generate mapping information
    std::vector<std::vector<int>> *getFeatureMap(int maxSize);
};



/**
 *  Auxiliary structures needed for hierarchical clustering
 */

// Sparse representation of the distance matrix, directly using triplets
struct DisNode {
    int id1, id2;
    double distance;

    DisNode(int _i1, int _i2, double _d) : id1(_i1), id2(_i2), distance(_d) {}

    // Overload less than sign
    bool operator<(const DisNode &other) const noexcept {
        return distance < other.distance;
    }
};


// Hierarchical clustering dendrogram node class, which is a binary tree
struct ClusterNode {
    int id; // node number
    int size; //subtree size

    // parent node
    ClusterNode *fa = nullptr;
    // left and right subtrees
    ClusterNode *lson = nullptr;
    ClusterNode *rson = nullptr;

    ClusterNode(int _id, int _size) : id(_id), size(_size) {}

    ~ClusterNode() {
        if (lson != nullptr)delete lson;
        if (rson != nullptr)delete rson;
    }

    // Get the root node of the current node
    ClusterNode *getRoot() {
        ClusterNode *now = this;
        while (now->fa != nullptr)now = now->fa;
        return now;
    }

    // Split the dendrogram by the specified maxsize (and save it in the result.
    void getSon(std::vector<std::vector<int>> *result, int max_size);

    // Save all leaf nodes in the subtree to result.
    void pushLeaf(std::vector<int> &result);
};

#endif //KITSUNE_CPP_CLUSTER_H
