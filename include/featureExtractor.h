/**
 * @brief Created by Yang Bo on 2020/5/9.
 * 
 * modified: 02/06/2023: comments translated to english
 */
#ifndef KITSUNE_CPP_FEATUREEXTRACTOR_H
#define KITSUNE_CPP_FEATUREEXTRACTOR_H

#include <cstdio>
#include "utils.h"
#include "netStat.h"

/**
 *  class for feature extraction
  * Currently can do:
  * 1. Extract features from pcap and get instance vectors
  * 2. Read the features from the tsv and csv files of the package, and get the instance vector
  * 3. Read the instance vector directly from the tsv and csv files of the instance vector
  * What to do next:
  * 4. Online packet capture, directly obtain the instance vector of each packet
 */


// enumerated type, defined file type
enum FileType {
    PCAP, PacketTSV, PacketCSV, FeatureCSV, FeatureTSV, OnlineNetDevice
};

// Responsible for obtaining feature vectors. 
// It can be read from pcap, tsv files, or directly read a line of vectors directly from file types such as FeatureCSV
class FE {
private:
    TsvReader *tsvReader = nullptr;
    NetStat *netStat = nullptr;
    FileType fileType; // 当前文件的类型
public:
    // netStat uses the default time window constructor, and reads the tsv package feature file by default
    FE(const char *filename, FileType ft = PacketTSV);

    // The constructor of the specified time window, the package feature file of the tsv read by default
    FE(const char *filename, const std::vector<double> &lambdas, FileType ft = PacketTSV);

    // destructor
    ~FE() {
        delete tsvReader;
        if (netStat != nullptr)delete netStat;
    }

    // Get the next instance vector and save it in result
    int nextVector(double *result);

    // Return the size of the instance vector generated each time
    inline int getVectorSize() { return netStat->getVectorSize(); }

};



#endif //KITSUNE_CPP_FEATUREEXTRACTOR_H
