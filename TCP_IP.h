#pragma once

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

using namespace std;

typedef enum : uint8_t {
    packetClose,
    packetData,
    packetBroadcast,
    packetLogin,
    packetLoginStatus,
    packetRegist,
    packetRegistStatus,
    packetSearchUsers,
    packetListUsers,
    packetMessage
} packet_t_t;

typedef struct {
    packet_t_t type;
    uint32_t size;
    uint8_t *data;
} packet_t;

class TCP_Interface {
protected:
    SOCKET handlerConnection;
private:
    bool ISCONENCETD = true;
    vector<packet_t> sendBuffer;
    vector<packet_t> recvBuffer;
    thread sendThread;
    thread recvThread;
    bool stopped = false;
    mutex mtx;
public:
    TCP_Interface();

    TCP_Interface(SOCKET &socket);

    void init();

    void read(char *data, const int length);

    void write(char *data, const int length);

    void sendPacket(packet_t);

    void recvPacket(packet_t *);

    void addInSendBuffer(packet_t&);

    packet_t getFromRecvBuffer();

    void sendHandler();

    void recvHandler();

    bool isConected();

    ~TCP_Interface();
};

class TCP_Client : public TCP_Interface {
private:

public:
    TCP_Client(string ip, const int port);

    ~TCP_Client();
};

class TCP_Server {
private:
    SOCKET sListen;

public:
    TCP_Server(const int port, const int maxconnect);

    TCP_Interface *acceptConn();

    ~TCP_Server();
};
