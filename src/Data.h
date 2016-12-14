#pragma once

#include "skel.h"
#include "Packet.h"

class Data {
    char* cur_byte = NULL;
    set<Packet> packets;
public:
    Data(char udp_data[]);
    char* getCurrentDataPointer();
    size_t getDataArrayCopy(char bufToWrite[]);
    void addPacket(char udp_data[]);
    void setCurrentDataPointer(char* p);
    bool empty();
};

addr_id makeAddrID(const struct sockaddr_in *sap) {
    return (sap->sin_addr.s_addr << 8*sizeof(u_short) + sap->sin_port);
}