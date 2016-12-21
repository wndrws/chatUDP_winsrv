#pragma once

#include "skel.h"
#include "Packet.h"

class Data {
    char* cur_byte = NULL;
    set<Packet> packets;
    int clientID = 0;
public:
    Data() {};
    Data(char udp_data[], int id);
    char* getCurrentDataPointer();
    size_t getDataArrayCopy(char bufToWrite[]);
    void addPacket(char udp_data[]);
    void setCurrentDataPointer(char* p);
    bool empty();
    int getClientID();
};