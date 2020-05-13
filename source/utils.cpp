//
// Created by Yang Bo on 2020/5/9.
//

#include "../include/utils.h"


FILE *pcap2tcv(const char *filename) {
    const char *tshark_path = "E:\\wireshark\\tshark.exe";
    const char *field = "-e frame.time_epoch -e frame.len -e eth.src -e eth.dst -e ip.src -e ip.dst -e tcp.srcport -e tcp.dstport -e udp.srcport -e udp.dstport -e icmp.type -e icmp.code -e arp.opcode -e arp.src.hw_mac -e arp.src.proto_ipv4 -e arp.dst.hw_mac -e arp.dst.proto_ipv4 -e ipv6.src -e ipv6.dst";
    static char buf[1000];
    std::sprintf(buf, "%s -r \"%s\" -T fields %s -E header=y -E occurrence=f > \"%s.tsv\"", tshark_path, filename,
                 field, filename);
    printf(buf);

    std::system(buf);
    std::sprintf(buf, "%s.tsv", filename);
    std::printf("\ntshark parsing complete. File saved as: \"%s\"\n", buf);
    return fopen(buf, "r");
}


// 读取并预处理下一行, 如果读到了最后一行, 返回false
int TsvReader::nextLine() {
    // 如果读到文件末尾, 返回0列
    if (std::fgets(buffer, BufferSize, fp) == nullptr) return 0;
    id.resize(1, 0); // 第0列从0开始的
    for (int i = 1; buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\0'; ++i) {
        // 寻找分隔符, 找到下一个列的初始位置
        if (buffer[i] == delimitor) {
            id.push_back(i + 1);
        }
    }
    return id.size();
}

// 将第col列变成string返回
std::string TsvReader::getString(int col) {
    std::string ans;
    ans.clear();
    int now = id[col];
    while (buffer[now] != delimitor && buffer[now] != '\r' && buffer[now] != '\n' && buffer[now] != '\0')
        ans.push_back(buffer[now++]);
    return ans;
}
