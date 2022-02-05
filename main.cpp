#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "DLList.h"
#include "TCP_IP.h"
#include "sqlite3.h"

typedef struct {
    TCP_Interface *client;
    string nickName = "";
} clientInfo;

typedef struct {
    int id;
    string user;
    string passwd;
} usersdata_t;

#define CLIENT DLLIST_NODE<clientInfo*>*

using namespace std;
CLIENT Client0;

sqlite3 *db = 0;


vector<usersdata_t> DBSelectUsers(string pole, string poleValue) {
    vector<usersdata_t> ret;
    sqlite3_stmt *asd;
    stringstream str;
    str << "SELECT * FROM 'users' WHERE " << pole << " LIKE '%" << poleValue << "%';";
    sqlite3_prepare(db, str.str().c_str(), -1, &asd, nullptr);
    sqlite3_step(asd);
    while (sqlite3_column_text(asd, 0)) {
        usersdata_t tmp;
        tmp.id = sqlite3_column_int(asd, 0);
        tmp.user = string((char *) sqlite3_column_text(asd, 1));
        tmp.passwd = string((char *) sqlite3_column_text(asd, 2));
        ret.push_back(tmp);
        sqlite3_step(asd);
    }
    sqlite3_finalize(asd);
    return ret;
}

void DBInsertUser(string user, string passwd) {
    sqlite3_stmt *asd;
    stringstream str;
    str << "INSERT INTO 'users' ('user', 'passwd') VALUES ('" << user << "', '" << passwd << "');";
    sqlite3_prepare(db, str.str().c_str(), -1, &asd, nullptr);
    sqlite3_step(asd);
    sqlite3_finalize(asd);
}

bool loginCheck(packet_t *packet) {
    string login((char *) packet->data);
    string passwd((char *) (packet->data + login.size() + 1));
    vector<usersdata_t> arr = DBSelectUsers("user", login);

    if (arr.size() == 0) {
        return false;
    }
    return (login == arr[0].user && passwd == arr[0].passwd);
}

bool registCheck(packet_t *packet) {
    string login((char *) packet->data);
    string passwd((char *) (packet->data + login.size() + 1));

    vector<usersdata_t> arr = DBSelectUsers("user", login);

    if (arr.size() > 0) {
        return false;
    } else {
        DBInsertUser(login, passwd);
        return true;
    }
}

vector<usersdata_t> searchUsers(packet_t *packet) {
    string login((char *) packet->data);
    vector<usersdata_t> ret = DBSelectUsers("user", login);
    return ret;
}

void clientThread(TCP_Interface *connection, packet_t *packet, string &nickname) {
    packet_t newPacket;
    printf("%d %d\n", packet->type, packet->size);
    if (packet->type == packetData) {
        newPacket = packet_t{packetBroadcast, packet->size, packet->data};
        CLIENT iteraitNODE = Client0->next;
        while (iteraitNODE != nullptr) {
            if (iteraitNODE->data->client != connection) {
                iteraitNODE->data->client->sendPacket(newPacket);
            }
            iteraitNODE = iteraitNODE->next;
        }
    }
    if (packet->type == packetLogin) {
        uint8_t responce = loginCheck(packet);
        if (responce) {
            nickname = string((char *) packet->data);
        }
        newPacket = packet_t{packetLoginStatus, sizeof(uint8_t), &responce};
        connection->addInSendBuffer(newPacket);
    }
    if (packet->type == packetRegist) {
        uint8_t responce = registCheck(packet);
        newPacket = packet_t{packetRegistStatus, sizeof(uint8_t), &responce};
        connection->addInSendBuffer(newPacket);
    }
    if (packet->type == packetSearchUsers) {
        vector<usersdata_t> listUser = searchUsers(packet);
        string listUserStr;
        for (int i = 0; i < listUser.size(); ++i) {
            listUserStr += listUser[i].user + '\0';
        }
        cout << listUserStr.size() << endl;
        newPacket = packet_t{packetListUsers, (uint32_t) listUserStr.size(), (uint8_t * ) & listUserStr[0]};
        connection->addInSendBuffer(newPacket);
    }
    if (packet->type == packetMessage) {
        string recipient((char *) packet->data);
        string message((char *) (packet->data + recipient.size() + 1));

        newPacket = packet_t{packetMessage, (uint32_t) message.size(), (uint8_t * ) & message[0]};
        CLIENT iteraitNODE = Client0->next;
        while (iteraitNODE != nullptr) {
            if (iteraitNODE->data->nickName == recipient) {
                iteraitNODE->data->client->addInSendBuffer(newPacket);
            }
            iteraitNODE = iteraitNODE->next;
        }
    }
}

void __clientThread(CLIENT NODE) {
    thread::id this_id = this_thread::get_id();
    cout << this_id << " <- new client" << endl;
    packet_t newPacket;
    while (true) {
        newPacket = NODE->data->client->getFromRecvBuffer();
        string login((char *) newPacket.data);
        string passwd((char *) (newPacket.data + login.size() + 1));
        if (newPacket.type == packetClose || !NODE->data->client->isConected()) {
            delete newPacket.data;
            break;
        }
        clientThread(NODE->data->client, &newPacket, NODE->data->nickName);
        delete newPacket.data;
    }

    delete DLLIST_freeNode(NODE);
}


int main() {
    sqlite3_open("C:\\Users\\none\\CLionProjects\\server\\test_db.db", &db);
    Client0 = DLLIST_addNode<clientInfo *>(nullptr, nullptr);
    TCP_Server server(11111, 20);
    while (true) {
        clientInfo *tmp = new clientInfo{server.acceptConn()};
        CLIENT NODE = DLLIST_addNode(Client0, tmp);
        thread newClient(__clientThread, NODE);
        newClient.detach();
    }
    DLLIST_freeChain(Client0);
}