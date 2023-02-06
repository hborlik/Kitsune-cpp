/**
 * @brief Created by Yang Bo on 2020/5/9.
 * 
 * modified: 02/06/2023: comments translated to english
 */
#include "../include/featureExtractor.h"


// netStat uses the default time window constructor, and reads the tsv package feature file by default
FE::FE(const char *filename, FileType ft) {
    fileType = ft;
    netStat = new NetStat();
    if (fileType == PacketTSV || fileType == FeatureTSV) {// delimiter is tab
        tsvReader = new TsvReader(filename, '\t');
    } else if (fileType == PacketCSV || fileType == FeatureCSV) {// Delimiter is ','
        tsvReader = new TsvReader(filename, ',');
    } else if (fileType == PCAP) { // Files that need to be converted to tsv
        tsvReader = new TsvReader(pcap2tcv(filename));
    }
    // The read package feature file needs to use netStat statistics
    if (fileType == PCAP || fileType == PacketCSV || fileType == PacketTSV) {
        // There is a header, so you need to read the header first
        tsvReader->nextLine();
    }
}

// The constructor of the specified time window, the package feature file of the tsv read by default
FE::FE(const char *filename, const std::vector<double> &lambdas, FileType ft) {
    fileType = ft;
    netStat = new NetStat(lambdas);
    if (fileType == PacketTSV || fileType == FeatureTSV) {// delimiter is tab
        tsvReader = new TsvReader(filename, '\t');
    } else if (fileType == PacketCSV || fileType == FeatureCSV) {// Delimiter is ','
        tsvReader = new TsvReader(filename, ',');
    } else if (fileType == PCAP) { // Files that need to be converted to tsv
        tsvReader = new TsvReader(pcap2tcv(filename));
    }
    // The read package feature file needs to use netStat statistics
    if (fileType == PCAP || fileType == PacketCSV || fileType == PacketTSV) {
        // There is a header, so you need to read the header first
        tsvReader->nextLine();
    }
}


// Read the characteristics of a line of packets from the reader, pass it to netstat to obtain the vector of the next group of instances,
// If successful, return the number of vectors, otherwise return 0
int FE::nextVector(double *result) {
    int cols = tsvReader->nextLine();
    if (cols == 0)return 0;
    if (fileType == FeatureTSV || fileType == FeatureCSV) { // If you read the vector information directly, read the double directly
        int num = getVectorSize();
        if (cols < num)return 0;
        for (int i = 0; i < num; ++i)result[i] = tsvReader->getDouble(i);
        return num;
    } else { // Incremental statistics with netStat
        std::string srcIP, dstIP;
        if (tsvReader->hasValue(4)) {// Ipv4
            srcIP = tsvReader->getString(4);
            dstIP = tsvReader->getString(5);
        } else { // Ipv6
            srcIP = tsvReader->getString(17);
            dstIP = tsvReader->getString(18);
        }
        std::string srcport, dstport;
        if (tsvReader->hasValue(6)) {//tcp
            srcport = tsvReader->getString(6);
            dstport = tsvReader->getString(7);
        } else if (tsvReader->hasValue(8)) { // udp
            srcport = tsvReader->getString(8);
            dstport = tsvReader->getString(9);
        } else { // It is neither tcp nor udp, it may be a layer 1 or layer 2 packet such as arp or icmp
            if (tsvReader->hasValue(10)) { // icmp
                srcport = dstport = "icmp";
            } else if (tsvReader->hasValue(12)) { // arp
                srcport = dstport = "arp";
                // Use the source ip and destination ip in the arp packet as ip information
                srcIP = tsvReader->getString(14);
                dstIP = tsvReader->getString(16);
            } else { // For other protocols, use source and destination MAC assignments
                srcIP = tsvReader->getString(2);
                dstIP = tsvReader->getString(3);
            }
        }
        return netStat->updateAndGetStats(tsvReader->getString(2), tsvReader->getString(3),
                                          srcIP, srcport, dstIP, dstport, tsvReader->getDouble(1),
                                          tsvReader->getDouble(0), result);
    }
}
