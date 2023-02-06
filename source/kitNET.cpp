/**
 * @brief Created by Yang Bo on 2020/5/11.
 * 
 * modified: 02/06/2023: comments translated to english
 */
#include "../include/kitNET.h"

void KitNET::init() {
    if (kitNetParam == nullptr) {
        fprintf(stderr, "KITNET: KitNETParam must not be null\n");
        throw -1;
    }
    // Need to cluster to get mapped array
    if (featureMap == nullptr) {
        if (kitNetParam->cluster == nullptr) {
            fprintf(stderr, "KITNET: the Cluster object must not be null\n");
        }
        // Clustering to obtain feature maps
        featureMap = kitNetParam->cluster->getFeatureMap(kitNetParam->max_size);
    }

    // Clustering to obtain feature maps to initialize autoencoders
    ensembleLayer = new AE *[featureMap->size()];
    for (int i = 0; i < featureMap->size(); ++i) {
        ensembleLayer[i] = new AE(featureMap->at(i).size(),
                                  std::ceil(featureMap->at(i).size() * kitNetParam->ensemble_vh_rate),
                                  kitNetParam->ensemble_learning_rate);
    }
    outputLayer = new AE(featureMap->size(), std::ceil(featureMap->size() * kitNetParam->output_vh_rate),
                         kitNetParam->output_learning_rate);

    // Initialize a buffer of autoencoder input parameters
    ensembleInput = new double *[featureMap->size()];
    for (int i = 0; i < featureMap->size(); ++i) {
        ensembleInput[i] = new double[featureMap->at(i).size()];
    }
    outputInput = new double[featureMap->size()];

    for (auto &i : *featureMap) {
        fprintf(stderr, "[");
        for (int j : i)fprintf(stderr, "%d,", j);
        fprintf(stderr, "]\n");
    }


    delete kitNetParam;
    kitNetParam = nullptr;
}

KitNET::~KitNET() {
    delete kitNetParam; // If it is null, delete null has no effect, so delete it directly
    for (int i = 0; i < featureMap->size(); ++i) {
        delete ensembleLayer[i];
        delete[] ensembleInput[i];
    }
    delete outputLayer;
    delete[] outputInput;
    delete featureMap;
}

double KitNET::train(const double *x) {
    if (featureMap == nullptr) { // If the feature map has not been initialized
        --kitNetParam->fm_train_num;
        // Update the value maintained in the cluster
        kitNetParam->cluster->update(x);
        // If the number of training feature maps reaches the set value, initialize the autoencoder
        if (kitNetParam->fm_train_num == 0)init();
        return 0;
    } else {// train the autoencoder
        for (int i = 0; i < featureMap->size(); ++i) {
            // Copy the corresponding eigenvectors to the buffer
            for (int j = 0; j < featureMap->at(i).size(); ++j) {
                ensembleInput[i][j] = x[featureMap->at(i).at(j)];
            }
            outputInput[i] = ensembleLayer[i]->train(ensembleInput[i]);
        }
        // Train the output layer, return the reconstruction error
        return outputLayer->train(outputInput);
    }
}

double KitNET::execute(const double *x) {
    if (featureMap == nullptr) { // If the feature map has not been initialized
        fprintf(stderr, "KitNET: the feature map is not initialized!!\n");
        throw -1;
    }
    for (int i = 0; i < featureMap->size(); ++i) {
        // Copy the corresponding eigenvectors to the buffer
        for (int j = 0; j < featureMap->at(i).size(); ++j) {
            ensembleInput[i][j] = x[featureMap->at(i).at(j)];
        }
        // Run this small autoencoder, saving the reconstruction error as input to the output layer
        outputInput[i] = ensembleLayer[i]->reconstruct(ensembleInput[i]);
    }
    return outputLayer->reconstruct(outputInput);
}
