#pragma once

#include <stdio.h>
#include <stdint.h>
#include <conio.h>

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "Message.h"
#include "ServerConnection.h"
#include "P2PConnection.h"
#include "Sender.h"

#define MAX_USER_MESSAGE_SIZE   4096

class MatchMaker {
private:
    ServerCallbacks severCbs;
    P2PCallbacks p2pCbs;

    std::unique_ptr<ServerConnection> serverConnection;
    std::unique_ptr<P2PConnection> p2pClient;

    std::string loggedPlayerName;
    std::string serverIP;;
    int serverPort;

    // Some randome buffer representnig userdata - this can be whatever you want in swos
    std::vector<char> userBlob;
    // take hash of blob to check that the buffer is correctly received and passed on by the server
    std::size_t blobHash;
    bool loop = true;

public:
    std::vector<std::string> openGames;
    std::vector<std::string> requestedJoiners;
    std::vector<std::string> joined;
    char userMessageSent[MAX_USER_MESSAGE_SIZE];
    char userMessageReceived[MAX_USER_MESSAGE_SIZE];
    int userMessageSizeSent;
    int userMessageSizeReceived;

public:
    // init
    // -- internal
    void initServer(char* playerName, char* ip, int port);
    void initENet();
    void closeENet();
    void setCallbacks();
    void onStart();

    // -- external
    void init(char* playerName, char* ip, int port);
    void close();

    // process
    // -- callbacks
    void processCallbacks();
    std::function<void(void)> updateMatchMakingMenu;

    // -- commands
    void processServerCommands(int c);
    void processP2PCommands(int c);
    void processCommands(int c);

    // test
    void testClientLoop();
    void testClient();

    // utility
    void copyStructToVector(void* s, std::vector<char>& v, int size);
    void copyVectorToStruct(std::vector<char> v, void* s, int size);
    int testCopyStructAndVector();
    int checkParameters(int argc);
};