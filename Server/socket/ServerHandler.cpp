#include "ServerHandler.h"
#include "../utils/usersys.h"
#include "../utils/chat.h"

struct PrivateMessage {
    char receiver[20];
    char message[980];
};

void ServerHandler::sendToAll(Message msg) {
    for (int i = 0; i < (int)clientList.size(); ++i) {
        Client client = clientList[i];
        sendTo(client.socket, msg);
    }
}

string ServerHandler::findUsername(SOCKET sender) {
    char result[20] = { 0 };
    for (int i = 0; i < (int)clientList.size(); ++i) {
        if (clientList[i].socket == sender) {
            strcpy(result, clientList[i].username);
            break;
        }
    }
    return string(result);
}

SOCKET ServerHandler::findSocket(string username) {
    for (int i = 0; i < (int)clientList.size(); ++i) {
        if (string(clientList[i].username) == username) {
            return clientList[i].socket;
        }
    }
    return 0;
}




bool ServerHandler::ConnectToDatabase() {
    int err = sqlite3_open("server.db", &db);
    if (err) {
        return false;
    }
    return true;
}

void ServerHandler::DisconnectFromDatabase() {    
    sqlite3_close(db);
}

bool ServerHandler::StartListening(HWND m_hWnd) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        return false;
    }
    server = socket(AF_INET, SOCK_STREAM, 0);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(server, (SOCKADDR *)&serverAddress, sizeof(serverAddress));
    listen(server, 5);

    int err = WSAAsyncSelect(server, m_hWnd, WM_SOCKET, FD_READ | FD_ACCEPT | FD_CLOSE);
    if (err) {
        return false;
    }
    clientList.clear();
    return true;
}

string ServerHandler::HandleAuthentication(SOCKET sender, Message msg) {
    Auth auth;
    memcpy(&auth, (char*) &msg.content, sizeof auth);
    try {
        if (strcmp(msg.action, "login") == 0) {
            signin(db, auth);
        } else {
            signup(db, auth);
        }
    } catch (const char *err) {
        sendTo(sender, Message("login-response", err));
        throw 1;
    }

    char str[1000];
    sprintf(str, "%s %s.", auth.username, strcmp(msg.action, "login") == 0 ? "logged in" : "registered");

    sendTo(sender, Message("login-response", "OK"));
    return string(str);
}

void ServerHandler::UpdateSocket(SOCKET sender, const char *username) {
    Client client;
    client.socket = sender;
    strcpy(client.username, username);

    sendToAll(Message("new-user", client.username));

    char str[1000];
    sprintf(str, "%s joined the chat.", client.username);
    sendToAll(Message("message-all", str));
    
    clientList.push_back(client);
    for (int i = 0; i < (int)clientList.size(); ++i) {
        sendTo(sender, Message("new-user", clientList[i].username));
    }
    FetchHistoryAll(sender);
}

void ServerHandler::MessageToGeneral(SOCKET sender, Message msg) {
    char str[1000];
    sprintf(str, "%s: %s", findUsername(sender).c_str(), msg.content);
    generalChat(db, str);
    sendToAll(Message("message-all", str));
}

void ServerHandler::MessageToOne(SOCKET sender, Message msg) {
    PrivateMessage pvt1, pvt2;
    memcpy(&pvt1, msg.content, sizeof pvt1);

    // Find receiver's socket
    SOCKET receiver;
    for (int i = 0; i < (int)clientList.size(); ++i) {
        if (strcmp(clientList[i].username, pvt1.receiver) == 0) {
            receiver = clientList[i].socket;
            break;
        }
    }

    // Convert msg to "sender: msg"
    // and prepare message to send to both sender and receiver
    char str[980];
    string senderUsername = findUsername(sender);
    sprintf(str, "%s: %s", senderUsername.c_str(), pvt1.message);
    strcpy(pvt2.receiver, senderUsername.c_str());
    strcpy(pvt1.message, str);
    strcpy(pvt2.message, str);

    privateChat(db, pvt1.message, pvt1.receiver, pvt2.receiver);
    sendTo(sender, Message("message-one", (char*) &pvt1, sizeof pvt1));
    if (sender != receiver) {
        sendTo(receiver, Message("message-one", (char*) &pvt2, sizeof pvt2));
    }
}

string ServerHandler::Logout(SOCKET sender) {
    int pos = -1;
    char username[20];
    for (int i = 0; i < (int)clientList.size(); ++i) {
        if (clientList[i].socket == sender) {
            pos = i;
            logout(db, string(clientList[i].username));
            strcpy(username, clientList[i].username);
            break;
        }
    }
    if (pos == -1) {
        throw 1;
    }
    clientList.erase(clientList.begin() + pos);

    char str[1000];
    sprintf(str, "%s left the chat.", username);

    sendToAll(Message("message-all", str));
    sendToAll(Message("user-logout", username));

    return string(str);
}

void ServerHandler::FetchHistoryAll(SOCKET receiver) {
    Table messList = fetchHistory(db, NULL, NULL);
    for (int i = 1; i < (int)messList.size(); ++i) {
        sendTo(receiver, Message("message-all", messList[i][0].c_str()));
        Sleep(25);
    }
}

void ServerHandler::FetchHistoryOne(SOCKET sender, const char *receiverUsername) {
    string user1 = findUsername(sender);
    string user2 = string(receiverUsername);
    SOCKET receiver = findSocket(user2);

    Table messList = fetchHistory(db, user1.c_str(), user2.c_str());
    PrivateMessage pvt;
    strcpy(pvt.receiver, user1.c_str());
    for (int i = 1; i < (int)messList.size(); ++i) {
        strcpy(pvt.message, messList[i][0].c_str());
        sendTo(receiver, Message("message-one", (char*) &pvt, sizeof pvt));
    }
}