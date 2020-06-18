#ifndef HDL_H_
#define HDL_H_

#include "vector"
#include "Arduino.h"

using namespace std;

char incomingPacket[255];
char outgoingPacket[255];

word checksum(vector<byte> data);

class Packet {
  public:
    Packet(byte, byte, word, word, byte, byte, vector<byte>);
    Packet(char[]);
    bool success = true;
    byte net;
    byte dev;
    word type;
    word code;
    byte tnet;
    byte tdev;
    vector<byte> content;
    byte dump(char[]);
    void send();
};

void sendPacket(word code, byte tnet, byte tdev, vector<byte> content={});
void receivePacket();

#endif // HDL_H_
