/**
 * @brief Created by Yang Bo on 2020/5/9.
 * 
 * modified: 02/06/2023: comments translated to english
 */

#include "../include/neuralnet.h"


Dense::Dense(int inSize, int outSize, double (*activationFunc)(double), double (*activationDerivativeFunc)(double),
             double lr) {
    n_in = inSize;
    n_out = outSize;
    activation = activationFunc;
    activationDerivative = activationDerivativeFunc;
    learning_rate = lr;
    inputValue = new double[n_in];
    outputValue = new double[n_out];
    bias = new double[n_out];
    W = new double *[n_in];// n_in rows, n_out columns
    for (int i = 0; i < n_in; ++i)W[i] = new double[n_out];

    double val = 1.0 / n_out;
    // Evenly distributed initialization weights
    for (int i = 0; i < n_in; ++i) {
        for (int j = 0; j < n_out; ++j)W[i][j] = rand_uniform(-val, val);
    }
    for (int i = 0; i < n_out; ++i)bias[i] = 0;
}

Dense::~Dense() {
    delete[] bias;
    delete[] inputValue;
    delete[] outputValue;
    for (int i = 0; i < n_in; ++i) delete[] W[i];
    delete[] W;
}

void Dense::feedForward(const double *input, double *output, bool saveValue) {
    for (int i = 0; i < n_out; ++i) {
        output[i] = bias[i];
        for (int j = 0; j < n_in; ++j) {
            output[i] += W[j][i] * input[j];
        }
        output[i] = activation(output[i]);
    }
    if (saveValue) {
        std::memcpy(inputValue, input, sizeof(double) * n_in);
        std::memcpy(outputValue, output, sizeof(double) * n_out);
    }
}

// SGD
void Dense::BackPropagation(double *g) {
    for (int i = 0; i < n_out; ++i)outputValue[i] = g[i] * activationDerivative(outputValue[i]);

    // Calculate the error propagated to the previous layer
    for (int i = 0; i < n_in; ++i) {
        g[i] = 0;
        for (int j = 0; j < n_out; ++j) {
            g[i] += W[i][j] * outputValue[j];
        }
    }

    // Update the threshold, and save by multiplying the learning_rate by the way, no need to calculate when updating the weight
    for (int i = 0; i < n_out; ++i) {
        outputValue[i] *= learning_rate;
        bias[i] += outputValue[i];
    }

    // Update weights
    for (int i = 0; i < n_in; ++i) {
        for (int j = 0; j < n_out; ++j) {
            W[i][j] += inputValue[i] * outputValue[j];
        }
    }
}


// Constructor, the parameter is the number of visible layer and hidden layer
AE::AE(int v_sz, int h_sz, double _learning_rate) {
    visible_size = v_sz;
    hidden_size = h_sz;

    // Initialize the two-layer neural network, using the sigmoid activation function
    encoder = new Dense(visible_size, hidden_size, sigmoid, sigmoidDerivative, _learning_rate);
    decoder = new Dense(hidden_size, visible_size, sigmoid, sigmoidDerivative, _learning_rate);

    // Initialize an array of temporary variables
    tmp_x = new double[visible_size];
    tmp_z = new double[visible_size];
    tmp_y = new double[hidden_size];
    // tmp_g is the buffer used to propagate the gradient, so the size is the maximum value of each layer
    tmp_g = new double[std::max(hidden_size, visible_size)];

    // Initialize the array needed for normalization
    max_v = new double[visible_size];
    min_v = new double[visible_size];
    for (int i = 0; i < visible_size; ++i) {
        min_v[i] = 1e20;
        max_v[i] = -1e20;
    }
}

AE::~AE() {
    delete encoder;
    delete decoder;
    delete[] tmp_x;
    delete[] tmp_y;
    delete[] tmp_z;
    delete[] max_v;
    delete[] min_v;
}


// rebuild, returns the reconstructed value
double AE::reconstruct(const double *x) {
    normalize(x); // First normalize and save in tmp_x

    encoder->feedForward(tmp_x, tmp_y); // Encoding, stored in tmp_y

    decoder->feedForward(tmp_y, tmp_z); // Decode and save in tmp_z

    return RMSE(tmp_x, tmp_z, visible_size);
}

// train
double AE::train(const double *x) {
    normalize(x); // 0-1 regularization, stored in tmp_x
    // Run forward again, set the saveValue parameter to true, and prepare for back propagation error
    encoder->feedForward(tmp_x, tmp_y, true);
    decoder->feedForward(tmp_y, tmp_z, true);

    // The error is stored in tmp_g
    for (int i = 0; i < visible_size; ++i)tmp_g[i] = tmp_x[i] - tmp_z[i];
    // back propagation error
    decoder->BackPropagation(tmp_g);
    encoder->BackPropagation(tmp_g);

    return RMSE(tmp_x, tmp_z, visible_size);
}

// 0-1 normalization, the result is saved in tmp_x
void AE::normalize(const double *x) {
    for (int i = 0; i < visible_size; ++i) {
        min_v[i] = std::min(x[i], min_v[i]);
        max_v[i] = std::max(x[i], max_v[i]);
        tmp_x[i] = (x[i] - min_v[i]) / (max_v[i] - min_v[i] + 1e-13);
    }
}
