#include <iostream>
#include <ctime>
#include <cstdlib>
#include "include/kitNET.h"
#include "include/featureExtractor.h"
#include "test/test.h"

using namespace std;


void aE() {
    const char *filename = "D:\\Dataset\\KITSUNE\\Mirai\\Mirai_pcap.pcap.tsv";
    const char *fea2 = "D:\\Dataset\\KITSUNE\\Mirai\\feature_my.csv";
    const char *fea1 = "D:\\Dataset\\KITSUNE\\Mirai\\feature_Kitsune.csv";
    const int FM_train_num = 5000; // The number of training feature maps required
    const int AD_train_num = 50000; // The number of training anomaly detection modules required
    const int KitNET_train_num = AD_train_num + FM_train_num;  // The number needed to train KitNET
    const int max_AE = 10; // Autoencoder maximum size

    auto fe = new FE(fea2, FeatureCSV);  // Initialize the feature extraction module

    int sz = fe->getVectorSize(); // Obtain the number of features required for feature extraction

    auto femap1 = new vector<vector<int> >{{42},
                                           {49},
                                           {71, 21, 28, 85, 99, 78, 92},
                                           {34, 20, 27},
                                           {35},
                                           {41, 48},
                                           {81, 67, 74, 82, 68, 75, 88, 95, 89, 96},
                                           {12, 43, 9,  36},
                                           {6,  29, 0,  15, 3,  22},
                                           {79, 65, 72, 86, 93},
                                           {59, 62, 56, 50, 53},
                                           {11, 38, 14, 45, 39, 46},
                                           {13, 44, 10, 37},
                                           {87, 94, 80, 66, 73},
                                           {7,  30, 1,  16, 4,  23},
                                           {90, 97, 83, 69, 76, 40, 47, 33, 19, 26},
                                           {8,  31, 5,  24, 2,  17, 32, 18, 25},
                                           {61, 58, 52, 55, 64, 63, 60, 57, 51, 54},
                                           {91, 98, 84, 70, 77}};
    auto femap2 = new vector<vector<int> >{{99, 98, 97, 96, 95},
                                           {54, 53, 52, 51, 50},
                                           {18, 3,  19, 4},
                                           {69, 68, 67, 66, 65},
                                           {17, 2,  16, 1,  15, 0},
                                           {64, 59, 58, 57, 56, 55, 63, 62, 61, 60},
                                           {94, 93},
                                           {92, 91, 90, 82, 81, 80, 77, 76, 75},
                                           {84, 83, 79, 78},
                                           {44, 43},
                                           {34, 33, 29, 14, 28, 13},
                                           {89, 88, 87, 86, 85, 39, 38, 37, 36, 35},
                                           {74, 73, 72, 71, 70},
                                           {22, 7,  21, 6,  20, 5},
                                           {23, 8,  24, 9},
                                           {32, 31, 30},
                                           {42, 41, 40, 26, 11, 25, 10, 27, 12},
                                           {49, 48, 47, 45, 46}};
    auto kitNET = new KitNET(femap2);
    //auto kitNET = new KitNET(sz, max_AE, FM_train_num, 0.75, 0.75, 0.1, 0.1);

    auto *x = new double[sz]; // Initialize the buffer to store the input feature vectors

    FILE *fp = fopen("RMSE.txt", "w");
    int now_packet = 0;
    while (fe->nextVector(x)) {
        ++now_packet;
//        if (now_packet <= FM_train_num){
//            fprintf(fp,"0\n");
//            continue;
//        }
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

void testARP() {
    const char *filename = "D:\\Dataset\\KITSUNE\\ARP_MitM\\ARP_MitM_pcap.pcapng.tsv";
    const int FM_train_num = 10000; // The number of training feature maps required
    const int AD_train_num = 300000; // The number of training anomaly detection modules required
    const int KitNET_train_num = AD_train_num + FM_train_num;  // The number needed to train KitNET
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


int main() {
    time_t start_time = time(nullptr);

    kitsuneExample();

    time_t end_time = time(nullptr);

    cout << "elapsed time: " << end_time - start_time << " s" << endl;
    return 0;
}
