#ifdef __cplusplus
extern "C" {
#endif
char* program_name;
#ifdef __cplusplus
}
#endif

#include "etcp.h"
#include <iostream>
#include <unordered_map>
#include "ClientRec.h"
#include "automutex.h"

SOCKET udpSocket;
bool stop = false;
unordered_map<int, ClientRec> clients;
unordered_map<addr_id, Data> storage;
static CAutoMutex mutex;
bool blockingMode = false;

int findClient(string IDorName);

DWORD WINAPI clientThread(LPVOID clientID) {
    int id = *((int*) clientID);
    addr_id s = clients.at(id).getAddrID();
    unsigned char code = 255;
    int rcvdb, r;
    string name = "<unknown>";
    unordered_map<int, ClientRec>::const_iterator it;
    //u_long mode = 0;

    while(!clients.at(id).isToClose()) {
//        if(mode == 0) {
//            mode = 1;
//            ioctlsocket(s, FIONBIO, &mode); // Make socket non-blocking
//        }
        rcvdb = readn(s, (char*) &code, 1);
        if(rcvdb == 0) {
            clients.at(id).notify(NotificationType::LOGOUT);
            clients.at(id).close();
            cout << "User " + name+"#"+to_string(id) + " disconnected gracefully." << endl;
        } else if(rcvdb < 0) {
            int error = WSAGetLastError();
            if(error == WSAEWOULDBLOCK || error == WSAEINTR) {
                // It's ok, continue doing job after some time
                Sleep(200);
                // TODO Need heartbeats from server as we don't know when client is crashed
            } else {
                cerr << "Error: reading from socket " << s << endl;
                if (!clients.at(id).getName().empty())
                    cerr << "User " + name+"#"+to_string(id) + " is gone." << endl;
                clients.at(id).notify(NotificationType::LOGOUT);
                clients.at(id).close();
            }
        } else {
//            mode = 0;
//            ioctlsocket(s, FIONBIO, &mode); // Make socket blocking again
            switch (code) {
                case CODE_LOGINREQUEST:
                    clients.at(id).login();
                    name = clients.at(id).getName();
                    cout << "User " + name+"#"+to_string(id) + " logged in!" << endl;
                    break;
                case CODE_LOGOUTREQUEST:
                    clients.at(id).logout();
                    cout << "User " + name+"#"+to_string(id) + " logged out!" << endl;
                    clients.at(id).close();
                    break;
                case CODE_INMSG:
                    r = clients.at(id).transmitMsg();
                    if(r < 0) {
                        if(r == -2) clients.at(id).sendMsg("Message is not delivered - destination user is offline");
                        else clients.at(id).sendErrorMsg(42, "Failed to transmit the message");
                    }
                    break;
                case CODE_HEARTBEAT:
                    // Send heartbeat in answer
                    if(!clients.at(id).sendHeartbeat()) {
                        cerr << "User " + name+"#"+to_string(id) + " is gone." << endl;
                        clients.at(id).notify(NotificationType::LOGOUT);
                        clients.at(id).close();
                    }
                    break;
                default:
                    cerr << "Info: Incorrect packet code received from socket " << s << endl;
                    break;
            }
        }
        code = 255;
        clients.at(id).sendNotifications();
    }
    shutdown(s, SD_BOTH);
    cerr << "Info: Socket " << s << " is shut down." << endl;
    CLOSE(s);
    cerr << "Info: Socket " << s << " is closed." << endl;
    {
        SCOPE_LOCK_MUTEX(mutex.get());
        clients.erase(id);
    }
    cerr << "Info: Client with id " << id << " at socket " << s << " is erased from users list." << endl;
    return 0;
}

HANDLE th_listener;
DWORD WINAPI listener_run(LPVOID) {
    //printf("%s: listener thread started.\n", program_name);
    SOCKET s;
    sockaddr_in peer;
    int peerlen = sizeof(peer);
    char buf[MAX_BUF_SIZE]; // Max data size to avoid fragmentation + 1 for null-terminating
    int rcvdb;

    for( ; ; ) {
        // Blocking recvfrom
        rcvdb = recvfrom(udpSocket, buf, sizeof(buf)-1, 0, (struct sockaddr*) &peer, &peerlen);
        if(rcvdb == SOCKET_ERROR) {
            cerr << "Error while listening for datagrams!" << endl;
            break;
        }
        if(stop) {
            break;
        }
        buf[rcvdb] = '\0';
        addr_id clientAddrID = makeAddrID(&peer);
        if(storage.find(clientAddrID) == storage.cend()) {
            ClientRec newClient(NULL, peer);
            storage[clientAddrID] = Data(buf);
            int newClientID = newClient.getClientID();
            HANDLE th = CreateThread(NULL, 0, clientThread, (LPVOID) &newClientID, 0, NULL);
            newClient.setThread(th);
            clients[newClientID] = newClient;
        } else {
            storage.at(clientAddrID).addPacket(buf);
        }
    }
    CLOSE(udpSocket);
    return 0;
}

int main(int argc, char** argv) {
    char* hostname;
    char* portname;

    INIT();
    if(argc == 2) {
        hostname = NULL;
        portname = argv[1];
    } else {
        hostname = argv[1];
        portname = argv[2];
    }

    udpSocket = udp_server(hostname, portname);
    th_listener = CreateThread(NULL, 0, listener_run, NULL, 0, NULL);
    cout << "Waiting for commands (type \"help\" for more information):" << endl;
    for( ; ; ) {
        string str;
        getline(cin, str);
        if(str == "q") break;
        else if(str == "help") {
            cout << "Command list:" << endl
                 << "m <username> - send server message to user <username>." << endl
                 << "b <username> - ban user <username> (force to logout)." << endl
                 << "q - force to logout all users and shut down the server." << endl
                 << "help - display this command list." << endl << endl
                 << "<username> ::= nickname_of_user | #user_id" << endl << endl;
        }
        else if(str.length() > 2) {
            if (str.at(0) == 'b') {
                string userToBan = str.substr(2);
                int id = findClient(userToBan);
                if (id != -1) {
                    clients.at(id).forcedLogout();
                    clients.at(id).close();
                }
            } else if (str.at(0) == 'm') {
                string userToInform = str.substr(2);
                int id = findClient(userToInform);
                if (id != -1) {
                    string msg;
                    cout << "Type message for " << userToInform << ":" << endl;
                    getline(cin, msg);
                    clients.at(id).sendMsg(msg);
                }
            }
        }
        else cout << "Unknown command (type \"help\" for more information): " << str << endl;
    }
    //Traverse clients calling close() on each one and joining their threads.
    for(auto&& client : clients) {
        client.second.forcedLogout();
        cout << "Closing socket " << client.first << endl;
        client.second.close();
    }
    stop = true;
    EXIT(0);
}

int findClient(string IDorName) {
    auto pos = IDorName.find('#');
    if(pos != string::npos) {
        // This is ID
        int id = atoi(IDorName.substr(pos+1).c_str());
        auto it = clients.find(id);
        if(it != clients.cend()) {
            return it->first;
        } else {
            cout << "User with id " << id << " not found." << endl;
        }
    } else {
        auto it = clients.cbegin();
        for( ; it != clients.cend(); ++it) {
            if(it->second.getName() == IDorName) {
                return it->first;
            }
        }
        if(it == clients.cend()) {
            cout << "User \"" + IDorName + "\" not found." << endl;
        }
    }
    return -1;
}