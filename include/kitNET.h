/**
 * @brief Created by Yang Bo on 2020/5/11.
 * 
 * modified: 02/06/2023: comments translated to english
 */
#ifndef KITSUNE_CPP_KITNET_H
#define KITSUNE_CPP_KITNET_H

#include <vector>
#include "neuralnet.h"
#include "cluster.h"


/**
*  Parameters required when KitNET creates the integration layer and the output layer
*/
struct KitNETParam {
    int max_size; // Ensemble layer encoder maximum size

    // The number of samples still needed to train the feature map
    int fm_train_num = 0;

    // Autoencoder display-hidden layer ratio of integrated layer
    double ensemble_vh_rate;

    // Output layer autoencoder display and hidden layer ratio
    double output_vh_rate;

    // Integration layer learning rate
    double ensemble_learning_rate;

    // output layer learning rate
    double output_learning_rate;

    // Helper class for clustering
    Cluster *cluster = nullptr;

    ~KitNETParam() {
        if (cluster != nullptr) delete cluster;
    }
};


/**
 *  KitNET class, mainly through clustering, the self-encoder implements the KitNET algorithm
 *
 */

class KitNET {
private:
    // Feature map, which holds the autoencoder to which each element of the feature instance vector is mapped.
    std::vector<std::vector<int> > *featureMap = nullptr;

    // Integrated layer autoencoder
    AE **ensembleLayer = nullptr;

    // Autoencoder for the output layer
    AE *outputLayer = nullptr;

    // A vector of inputs to the integration layer
    double **ensembleInput = nullptr;

    // The input vector of the output layer
    double *outputInput = nullptr;

    // Initialize the parameters required by the autoencoder
    KitNETParam *kitNetParam = nullptr;

    // Initialize KitNET, initialize according to feature parameters, etc.
    void init();

public:

    // There are two types of constructors, one is to directly provide the feature map. The other is to train the feature map according to the parameters, and only after training

     // Constructor 1, the parameters are:
     // 1. The array pointer of the feature map
     // 2. Mapped array pointer integration layer autoencoder display layer/hidden layer ratio 3. Output layer autoencoder display layer/hidden layer ratio, (both default 0.75)
     // 4. The learning rate of the integration layer 5. The learning rate of the output layer (both default 0.1 )
    KitNET(std::vector<std::vector<int> > *fm, double ensemble_vh_rate = 0.75, double output_vh_rate = 0.75,
           double ensemble_learning_rate = 0.1, double output_learning_rate = 0.1) {
        featureMap = fm;
        kitNetParam = new KitNETParam;
        kitNetParam->ensemble_learning_rate = ensemble_learning_rate;
        kitNetParam->ensemble_vh_rate = ensemble_vh_rate;
        kitNetParam->output_vh_rate = output_vh_rate;
        kitNetParam->output_learning_rate = output_learning_rate;
        init(); // Create an autoencoder directly
    }


    // Constructor 2, the parameters are:
     // 1. Enter the size of the instance
     // 2. The maximum size of each autoencoder in the integration layer,
     // 3. The number of instances needed to train the feature map.
     // 4. Integrated layer autoencoder display layer/hidden layer ratio 5. Output layer autoencoder display layer/hidden layer ratio, (both default 0.75)
     // 6. The learning rate of the integration layer 7. The learning rate of the output layer (both default 0.1 )
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


    ~KitNET();

    // Training, returns the reconstruction error. If it is training the FM module, returns 0
    double train(const double *x);

    // Anterior propagation, returns the reconstruction error of the current data
    double execute(const double *x);


};


#endif //KITSUNE_CPP_KITNET_H
