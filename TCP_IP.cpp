#include "TCP_IP.h"


TCP_Interface::TCP_Interface() : handlerConnection(NULL) {
    this->init();
};

TCP_Interface::TCP_Interface(SOCKET &socket) : handlerConnection(socket) {
    this->init();
}

void TCP_Interface::init() {
    sendThread = thread(&TCP_Interface::sendHandler, this);
    sendThread.detach();
    recvThread = thread(&TCP_Interface::recvHandler, this);
    recvThread.detach();
}

void TCP_Interface::read(char *data, const int length) {
    if (!this->ISCONENCETD) { return; }
    memset(data, 0, length);
    int tmp = recv(this->handlerConnection, data, length, NULL);
    if (tmp < 0) {
        this->ISCONENCETD = false;
    } else { this->ISCONENCETD = true; }
}

void TCP_Interface::write(char *data, const int length) {
    if (!this->ISCONENCETD) { return; }
    int tmp = send(this->handlerConnection, data, length, NULL);
    if (tmp < 0) {
        this->ISCONENCETD = false;
    } else { this->ISCONENCETD = true; }
}

void TCP_Interface::sendPacket(packet_t pt) {
    write((char *) &pt.type, sizeof(char));
    write((char *) &pt.size, sizeof(uint32_t));
    if (pt.size != 0) {
        write((char *) pt.data, pt.size);
    }
}

void TCP_Interface::recvPacket(packet_t *pt) {
    read((char *) &pt->type, sizeof(char));
    read((char *) &pt->size, sizeof(uint32_t));
    if (pt->size != 0) {
        pt->data = new uint8_t[pt->size];
        read((char *) pt->data, pt->size);
    }
}

void TCP_Interface::addInSendBuffer(packet_t &packet) {
    mtx.lock();
    sendBuffer.push_back(packet);
    mtx.unlock();
}

packet_t TCP_Interface::getFromRecvBuffer() {
    while (sendBuffer.size() <= 0) {}
    mtx.unlock();
    packet_t tmp = recvBuffer[0];
    recvBuffer.erase(recvBuffer.begin());
    mtx.unlock();
    return tmp;
}

void TCP_Interface::sendHandler() {
    while (!stopped) {
        if (sendBuffer.size() > 0) {
            mtx.lock();
            packet_t tmp = sendBuffer[0];
            cout << tmp.size << " " << tmp.type << endl;
            sendBuffer.erase(sendBuffer.begin());
            while (!this->ISCONENCETD && !stopped) {
                this->sendPacket(tmp);
            }
            cout << 12341231 << endl;
            mtx.unlock();
        }
    }
}

void TCP_Interface::recvHandler() {
    while (!stopped) {
        packet_t tmp;

        mtx.lock();
        this->recvPacket(&tmp);
        if (this->ISCONENCETD) {
            recvBuffer.push_back(tmp);
            cout << 0000000000000 << endl;
        }
        mtx.unlock();


    }
}

bool TCP_Interface::isConected() {
    return this->ISCONENCETD;
}

TCP_Interface::~TCP_Interface() {
    closesocket(this->handlerConnection);
}


TCP_Client::TCP_Client(string ip, const int port) {
    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) !=
        0) {
        cout << "ERROR " << endl;
        exit(1);
    }

    SOCKADDR_IN addr;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    this->handlerConnection = socket(addr.sin_family, SOCK_STREAM, NULL);
    if (connect(this->handlerConnection, (SOCKADDR *) &addr, sizeof(addr)) != 0) {
        cout << "ERROR Conected" << endl;
    } else { cout << "conetcted complid" << endl; }
}

TCP_Client::~TCP_Client() { closesocket(this->handlerConnection); }


TCP_Server::TCP_Server(const int port, const int maxconnect) {
    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) !=
        0) {
        cout << "ERROR " << endl;
        exit(1);
    }

    SOCKADDR_IN addr;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    this->sListen = socket(AF_INET, SOCK_STREAM, NULL);
    if (this->sListen == 0) {
        exit(0);
    }
    if ((bind(this->sListen, (SOCKADDR *) &addr,
              sizeof(addr))) < 0) { cout << "error" << endl; }
    if (listen(this->sListen,
               maxconnect) < 0) {
        cout << "error" << endl;
    }
}

TCP_Interface *TCP_Server::acceptConn() {
    SOCKET newConnection = accept(this->sListen, NULL, NULL);
    if (newConnection < 0) {
        return NULL;
    }
    return new TCP_Interface(newConnection);
}

TCP_Server::~TCP_Server() {
    closesocket(this->sListen);
}
