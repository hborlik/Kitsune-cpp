/**
 * @brief Created by Yang Bo on 2020/5/9.
 * 
 * modified: 02/06/2023: comments translated to english
 */

#ifndef KITSUNE_CPP_NEURALNET_H
#define KITSUNE_CPP_NEURALNET_H

#include <cstdio>
#include <cstring>
#include "utils.h"


/**
 *  Simple fully connected network layer
 */
class Dense {
private:
    int n_in;    // input size

    int n_out;    // output size

    double **W = nullptr; // connection weight

    double *bias = nullptr; // threshold

    double (*activation)(double); // function pointer to the activation function

    double (*activationDerivative)(double); // function pointer to the derivative of the activation function (parameter is the function value of the activation function)

    double learning_rate; // learning rate

    double *inputValue = nullptr; //Temporary variable to hold the input value

    double *outputValue = nullptr; // Temporary variable to hold the output value

public:
    // Constructor, the parameters are the number of input neurons, the number of output neurons, activation function, derivative of activation function, learning rate (default 0.1)
    Dense(int inSize, int outSize, double (*activationFunc)(double), double (*activationDerivativeFunc)(double),
          double lr = 0.1);

    ~Dense();

    // For forward propagation, the third parameter indicates whether to save the temporary variable of the input and output values. (Only false when forward propagation, must be true when bp is required after propagation)
    void feedForward(const double *input, double *output, bool saveValue = false);

    // Backpropagate the error, and save the error propagated to the previous layer to g. The capacity of g needs to be max(n_in,n_out)
    void BackPropagation(double *g);
};


/**
 *  Autoencoder class, by maintaining two fully connected layers (encoder and decoder)
 */
class AE {
private:
    int visible_size; // The size of the visible layer

    int hidden_size; // hidden layer size

    Dense *encoder = nullptr, *decoder = nullptr; // Two-layer neural network, encoder and decoder

    double *min_v = nullptr, *max_v = nullptr; // 0-1 normalization needs to maintain the maximum and minimum values

    double *tmp_x, *tmp_y, *tmp_z, *tmp_g; // Temporary variables

    // 0-1 normalization, the result is saved in tmp_x
    void normalize(const double *x);

    void normalize(const double *x) const;

public:
    // Constructor, the parameter is the number of visible layer, hidden layer, learning rate, default 0.01
    AE(int v_sz, int h_sz, double _learning_rate = 0.01);
    ~AE();

    // reconstruction, returns the root mean error of the reconstruction
    double reconstruct(const double *x) const;

    // training, returns the root mean error of the reconstruction
    double train(const double *x);

};


#endif //KITSUNE_CPP_NEURALNET_H
