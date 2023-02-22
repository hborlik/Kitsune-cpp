//
// Created by Yang Bo on 2020/5/15.
//

#include "../include/featureExtractor.h"
#include "../include/neuralnet.h"
#include "../include/kitNET.h"
#include "test.h"

// Simple example of testing Kitsune

void kitsuneExample() {
    const char *filename = "D:\\Dataset\\KITSUNE\\Mirai\\Mirai_pcap.pcap.tsv";
    const int FM_train_num = 5000; // 
    const int AD_train_num = 50000; // 
    const int KitNET_train_num = AD_train_num + FM_train_num; // The number needed to train KitNET
    const int max_AE = 10; // Autoencoder maximum size

    auto fe = new FE(filename, PacketTSV);  // Initialize the feature extraction module

    int sz = fe->getVectorSize(); // Get the number required for feature extraction

    auto kitNET = new KitNET(sz, max_AE, FM_train_num); // Initialize the kitNET module

    auto *x = new double[sz]; // Initialize the buffer to store the input feature vectors

    FILE *fp = fopen("RMSE.txt", "w");
    int now_packet = 0;
    while (fe->nextVector(x)) {
        ++now_packet;
        if (now_packet <= KitNET_train_num)
            fprintf(fp, "%.15f\n", kitNET->train(x));
        else
            fprintf(fp, "%.15f\n", kitNET->execute(x));

        if (now_packet % 1000 == 0)printf("%d\n", now_packet);
    }

    printf("total packets is %d\n", now_packet);
    fclose(fp);
    delete[] x;
    delete fe;
    delete kitNET;
}