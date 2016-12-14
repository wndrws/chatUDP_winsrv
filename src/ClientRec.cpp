#include <unordered_map>
#include <sstream>
#include <iostream>
#include "ClientRec.h"

extern unordered_map<int, ClientRec> clients;
extern SOCKET udpSocket;
addr_id makeAddrID(const sockaddr_in *sap);

ClientRec::ClientRec(HANDLE h_thread, sockaddr_in addr) {
    this->h_thread = h_thread;
    m_sockaddr_in = addr;
    m_addrid = makeAddrID(&m_sockaddr_in);
    m_id = idCounter++;
}

ClientRec::ClientRec() {
    h_thread = NULL;
    bzero(&m_sockaddr_in, sizeof(m_sockaddr_in));
    m_addrid = 0;
    m_id = 0;
}

void ClientRec::close() {
    toClose = true;
}

void ClientRec::setName(const string& name) {
    m_name = name;
}

void ClientRec::setThread(HANDLE hnd) {
    h_thread = hnd;
}

string ClientRec::getName() const {
    return m_name;
}

string ClientRec::getFullName() const {
    return m_name + "#" + to_string(m_id);
}

HANDLE ClientRec::getThread() const{
    return h_thread;
}

addr_id ClientRec::getAddrID() const {
    return m_addrid;
}

SOCKET ClientRec::getSockToSend() const {
    return udpSocket;
}

int ClientRec::getClientID() const {
    return m_id;
}

sockaddr_in ClientRec::getSockaddr_in() const {
    return m_sockaddr_in;
}

bool ClientRec::isToClose() const {
    return toClose;
}

void ClientRec::login() {
    char username [MAX_USERNAME_LENGTH+1];
    int r = readvrec(getAddrID(), username, MAX_USERNAME_LENGTH);
    if(r == -1) {
        cerr << "Error while reading request for login." << endl;
        return;
    }
    username[r] = '\0';
    setName(string(username));
    //Send users list
    string msg = formUsersList();
    //uint16_t len = (uint16_t) htons((uint16_t) msg.size());
    msg.insert(0, 1, (char) CODE_LOGINANSWER);
    //msg.insert(1, (char*) &len, 2);
    sockaddr_in peer = getSockaddr_in();
    r = sendto(getSockToSend(), msg.c_str(), (int) msg.size(), 0, (sockaddr*) &peer, sizeof(peer));
    if(r == -1) {
        cerr << "Failed to send users list to " << getFullName() << endl;
    }
    notify(NotificationType::LOGIN);
}

string ClientRec::formUsersList() const {
    ostringstream ss;
    for(auto it = clients.cbegin(); it != clients.cend(); ++it) {
        ss << (uint32_t) it->first << '\n' << it->second.getName() << '\n';
    }
    ss << "\4\n"; //End Of Transmission
    return ss.str();
}

void ClientRec::notifyIn(int id, const string& name) {
    m_notified = true;
    m_loggedIn.push_back(to_string(id));
    m_loggedIn.push_back(name);
}

void ClientRec::notifyOut(int id) {
    m_notified = true;
    m_loggedOut.push_back(to_string(id));
}

inline void ClientRec::notify(NotificationType nt) {
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->first != m_id) {
            switch (nt) {
                case LOGIN: it->second.notifyIn(m_id, m_name); break;
                case LOGOUT: it->second.notifyOut(m_id); break;
                default: cerr << "Illegal argument for notify()" << endl; return;
            }
        }
    }
}

void ClientRec::sendNotifications() {
    if(!m_notified) return;
    if(m_loggedIn.empty() && m_loggedOut.empty()) {
        cerr << "Error: notified by nobody." << endl;
        return;
    }

    ostringstream ss;
    string msg;
    int r;

    if(!m_loggedIn.empty()) {
        for (int i = 0; i < m_loggedIn.size(); i += 2) {
            ss << m_loggedIn[i] << '\n' << m_loggedIn[i + 1] << '\n';
        }
        ss << "\4\n"; //End Of Transmission
        msg = ss.str();
        msg.insert(0, 1, (char) CODE_LOGINNOTIFY);
        sockaddr_in peer = getSockaddr_in();
        r = sendto(getSockToSend(), msg.c_str(), (int) msg.size(), 0, (sockaddr*) &peer, sizeof(peer));
        if (r == -1) {
            cerr << "Failed to send login notification to " << getFullName() << endl;
        } else m_loggedIn.clear();

        ss.str("");
    }

    if(!m_loggedOut.empty()) {
        for (int i = 0; i < m_loggedOut.size(); i++) {
            ss << m_loggedOut[i] << '\n';
        }
        ss << "\4\n"; //End Of Transmission
        msg = ss.str();
        msg.insert(0, 1, (char) CODE_LOGOUTNOTIFY);
        sockaddr_in peer = getSockaddr_in();
        r = sendto(getSockToSend(), msg.c_str(), (int) msg.size(), 0, (sockaddr*) &peer, sizeof(peer));
        if (r == -1) {
            cerr << "Failed to send logout notification to " << getFullName() << endl;
        } else m_loggedOut.clear();
    }
    m_notified = false;
}

void ClientRec::logout() {
    char code = CODE_LOGOUTANSWER;
    sockaddr_in peer = getSockaddr_in();
    int r = sendto(getSockToSend(), &code, 1, 0, (sockaddr*) &peer, sizeof(peer));
    if(r == -1) {
        cerr << "Failed to send logout answer to " << getFullName() << endl;
    }
    notify(NotificationType::LOGOUT);
}

void ClientRec::forcedLogout() {
    char code = CODE_FORCEDLOGOUT;
    sockaddr_in peer = getSockaddr_in();
    int r = sendto(getSockToSend(), &code, 1, 0, (sockaddr*) &peer, sizeof(peer));
    if(r == -1) {
        cerr << "Failed to send forced logout message to " << getFullName();
        cerr << strerror(errno);
    }
    notify(NotificationType::LOGOUT);
    cout << "User " << getFullName() << " was logged out by force." << endl;
}

void ClientRec::sendErrorMsg(int errcode, const string& descr) const {
    string msg = "Server error " + to_string(errcode) + ":\n" + descr + "\n";
    uint16_t len = (uint16_t) htons((uint16_t) msg.size());
    msg.insert(0, 1, (char) CODE_SRVERR);
    msg.insert(1, (char*) &len, 2);
    sockaddr_in peer = getSockaddr_in();
    int r = sendto(getSockToSend(), msg.c_str(), (int) msg.size(), 0, (sockaddr*) &peer, sizeof(peer));
    if(r == -1) {
        cerr << "Failed to send error message to " << getFullName() << endl;
    }
}

void ClientRec::sendMsg(const string &text) const {
    string msg = text + "\n";
    uint16_t len = (uint16_t) htons((uint16_t) msg.size());
    msg.insert(0, 1, (char) CODE_SRVMSG);
    msg.insert(1, (char*) &len, 2);
    sockaddr_in peer = getSockaddr_in();
    int r = sendto(getSockToSend(), msg.c_str(), (int) msg.size(), 0, (sockaddr*) &peer, sizeof(peer));
    if(r == -1) {
        cerr << "Failed to send message to " << getFullName() << endl;
        cerr << strerror(errno);
    }
}

int ClientRec::transmitMsg() const {
    char buf [MAX_MSG_LENGTH+1];
    char id_buf [11]; //max 10 digits in a 32-bit id + '\n'
    int id;

    int r = readline(getAddrID(), id_buf, sizeof(id_buf));
    if(r < 0) {
        cerr << "Failed to extract user id from incoming message from "
             << getFullName() << endl;
    }
    id = atoi(id_buf);
    r = readline(getAddrID(), buf, sizeof(buf));
    if(r <= 0) {
        cerr << "Failed to read incoming message from " << getFullName() << endl;
        return -1;
    }

    if(clients.find(id) == clients.cend()) return -2;

    string msg(buf);
    //uint16_t len = (uint16_t) htons((uint16_t) msg.size());
    //msg.insert(0, (char*) &len, 2);
    msg = to_string(getClientID()) + "\n" + msg; // Already contains trailing "\n"
    msg.insert(0, 1, (char) CODE_OUTMSG);
    sockaddr_in peer = clients.at(id).getSockaddr_in();
    r = sendto(clients.at(id).getSockToSend(), msg.c_str(), (int) msg.size(), 0, (sockaddr*) &peer, sizeof(peer));
    if(r == -1) {
        cerr << "Failed to transmit message to " << clients.at(id).getFullName() << endl;
        return -3;
    }
    return 0;
}

bool ClientRec::sendHeartbeat() const {
    char code = CODE_HEARTBEAT;
    sockaddr_in peer = getSockaddr_in();
    int r = sendto(getSockToSend(), &code, 1, 0, (sockaddr*) &peer, sizeof(peer));
    if(r < 0) {
        cerr << "Failed to send heartbeat to " << getFullName() << ": " << strerror(errno) << endl;
        return false;
    }
    return true;
}