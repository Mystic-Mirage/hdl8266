#include "vector"
#include "Arduino.h"
#include "hdl.h"

using namespace std;

word checksum(vector<byte> data) {
    word cs = 0;
    for (byte d : data) {
        cs ^= d << 8;
        for (int j = 0; j < 8; j++) {
            if (cs & 0x8000) {
                cs = (cs << 1) ^ 0x1021;
            } else {
              cs <<= 1;
            }
        }
    }
    return cs;
}

Packet::Packet(byte net, byte dev, word type, word code, byte tnet, byte tdev, vector<byte> content={}) {
  this->net = net;
  this->dev = dev;
  this->type = type;
  this->code = code;
  this->tnet = tnet;
  this->tdev = tdev;
  this->content = content;
}

Packet::Packet(char data[]) {
  if (strncmp(data+4, header, 10) != 0) {
    Serial.println("header?");
    success = false;
    return;
  }
  
  if (makeWord(data[14], data[15]) != 0xaaaa) {
    Serial.println("starter?");
    success = false;
    return;
  }

  size_t len = data[16] - 11;
  vector<byte> p(data+16, data+25+len);

  word cs = makeWord(data[25+len], data[26+len]);
  if (checksum(p) != cs) {
    Serial.println("checksum?");
    success = false;
    return;
  }

  net = data[17];
  dev = data[18];
  type = makeWord(data[19], data[20]);
  code = makeWord(data[21], data[22]);
  tnet = data[23];
  tdev = data[24];

  content.insert(content.end(), p.begin()+9, p.end());
}

byte Packet::dump(char buf[]) {
  int i;

  for (i = 0; i < 4; i++) {
    buf[i] = ip[i];
  }

  for (i = 0; i < 10; i++) {
    buf[i+4] = header[i];
  }

  for (i = 14; i < 16; i++) {
    buf[i] = 0xaa;
  }

  size_t size = content.size();
  vector<byte> p = {(byte)(size+11), net, dev, highByte(type), lowByte(type), highByte(code), lowByte(code), tnet, tdev};

  p.insert(p.end(), content.begin(), content.end());

  word cs = checksum(p);
  p.push_back(highByte(cs));
  p.push_back(lowByte(cs));

  copy(p.begin(), p.end(), buf+16);
  return size + 27;
}

void Packet::send() {
  size_t size = dump(outgoingPacket);
  digitalWrite(ledPin, LOW);
  udp.beginPacket(broadcast, port);
  udp.write(outgoingPacket, size);
  udp.endPacket();
  digitalWrite(ledPin, HIGH);
}

void sendPacket(word code, byte tnet, byte tdev, vector<byte> content={}) {
  Packet p(network_id, device_id, device_type, code, tnet, tdev, content);
  p.send();
}

void receivePacket() {
  if (udp.parsePacket()) {
    size_t size = udp.read(incomingPacket, 255);
    if (size) {
      digitalWrite(ledPin, LOW);
      Packet p(incomingPacket);
      if (p.success and (p.tnet == network_id or p.tnet == 0xff or p.tnet == 0x00) and (p.tdev == device_id or p.tdev == 0xff or p.tdev == 0x00)) {
        switch (p.code) {
          case 0x0031:
            if (p.content[0] == 1) {
//              Serial.println("switch!");
              relayState = p.content[1] == 100 ? HIGH : LOW;
              digitalWrite(relayPin, relayState);
              sendPacket(0x0032, p.net, p.dev, {1, 0xf8, p.content[1], 8, relayState});
            } else {
              sendPacket(0x0032, p.net, p.dev, {p.content[0], 0xf5, 0, 8, relayState});
            }
            break;
          case 0x0033:
//            Serial.println("status!");
            sendPacket(0x0034, p.net, p.dev, {8, relayState == HIGH ? 100 : 0, 0, 0, 0, 0, 0, 0, 0});
            break;
          case 0xf00e:
            if (p.content[0] == 1) {
//              Serial.println("ch_remark!");
              vector<byte> a = {p.content[0]};
              a.insert(a.end(), ch_remark, ch_remark+20);
              sendPacket(0xf00f, p.net, p.dev, a);
            }
            break;
          case 0x000e:
//            Serial.println("remark!");
            sendPacket(0x000f, p.net, p.dev, vector<byte>(remark, remark+20));
            break;
        }
      }
      digitalWrite(ledPin, HIGH);
    }
  }
}

