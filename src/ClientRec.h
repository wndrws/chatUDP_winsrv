#pragma once

#include <string>
#include <vector>
#include "etcp.h"
#include "Data.h"

#define CODE_SRVMSG 0
#define CODE_LOGINREQUEST 1
#define CODE_LOGOUTREQUEST 2
#define CODE_LOGINANSWER 3
#define CODE_LOGOUTANSWER 4
#define CODE_FORCEDLOGOUT 5
#define CODE_LOGINNOTIFY 6
#define CODE_LOGOUTNOTIFY 7
#define CODE_SRVERR 8
#define CODE_HEARTBEAT 9

#define CODE_INMSG 128
#define CODE_OUTMSG 129

#define MAX_USERNAME_LENGTH 32
#define MAX_MSG_LENGTH 1400
using namespace std;

enum NotificationType { LOGIN, LOGOUT };
static int idCounter = 1;

class ClientRec {
private:
    string m_name = "";
    HANDLE h_thread;
    volatile bool toClose = false;
    addr_id m_addrid;
    int m_id;
    sockaddr_in m_sockaddr_in;

    volatile bool m_notified = false;
    vector<string> m_loggedIn;
    vector<string> m_loggedOut;

    string formUsersList() const;
public:
    ClientRec(HANDLE, sockaddr_in);
    //ClientRec(const ClientRec&);
    ClientRec();
    string getName() const;
    string getFullName() const;
    HANDLE getThread() const;
    addr_id getAddrID() const;
    int getClientID() const;
    SOCKET getSockToSend() const;
    sockaddr_in getSockaddr_in() const;
    bool isToClose() const;

    inline void notify(NotificationType);

    void close();
    void setName(const string&);
    void setThread(HANDLE);
    void login();
    void logout();
    void forcedLogout();
    void notifyIn(int id, const string& username);
    void notifyOut(int id);
    void sendNotifications();
    void sendErrorMsg(int errcode, const string& descr) const;
    void sendMsg(const string& text) const;
    int transmitMsg() const;
    bool sendHeartbeat() const;
};