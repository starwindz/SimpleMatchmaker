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

#define MAX_USER_MESSAGE_SIZE       4096

#define MMM_X_SERVER_NOT_CONNECTED  -1
#define MMM_0_DEFAULT				0
#define MMM_1_WAIT_TO_JOIN	        1
#define MMM_2_REQUESTED_TO_JOIN		2
#define MMM_3_ALLOW_DENY_LEAVE		3	
#define MMM_4_LOBBY					4

/*
<0: MMM_0_DEFAULT>
              [MATCH MAKING]

                OPEN GAMES
                    - (O)
                    -

            OPEN OR JOIN THE GAME

            [OPEN] (LEAVE)
                 [EXIT]


<1: MMM_1_WAIT_TO_JOIN>
              [MATCH MAKING]

                OPEN GAMES
                 PLAYER1 (X)
                    -

          WAITING FOR OTHERS TO JOIN

            (OPEN) [LEAVE]
                 [EXIT]


<2: MMM_2_REQUESTED_TO_JOIN>
              [MATCH MAKING]

                OPEN GAMES
                 PLAYER1 (X)
                    -

REQUESTED TO JOIN GAME. WAITING FOR HOST TO RESPOND

            (OPEN) [LEAVE]
                 [EXIT]


<3: MMM_3_ALLOW_DENY_LEAVE>
              [MATCH MAKING]

                OPEN GAMES
                 PLAYER1 (X)
                    -

          PLAYER2 WANTS TO JOIN

            [ALLOW] [DENY]
                 [LEAVE]


<4: MMM_4_LOBBY>
               [LOBBY]
                         [READY]
                         [READY]



                [CANCEL] [START]
*/

class MatchMaker {
private:
    ServerCallbacks severCbs;
    P2PCallbacks p2pCbs;

    std::string loggedPlayerName;
    std::string serverIP;;
    int serverPort;

    // Some randome buffer representnig userdata - this can be whatever you want in swos
    std::vector<char> userBlob;
    // take hash of blob to check that the buffer is correctly received and passed on by the server
    std::size_t blobHash;

public:
    std::unique_ptr<ServerConnection> serverConnection;
    std::unique_ptr<P2PConnection> p2pClient;

    std::vector<std::string> openGames;
    std::vector<std::string> requestedJoiners;
    std::vector<std::string> joined;
    std::vector<PlayerInfo> users;
    bool userRemoved = false;
    int curJoinerIndex = 0;

    char userMessageSent[MAX_USER_MESSAGE_SIZE];
    char userMessageReceived[MAX_USER_MESSAGE_SIZE];
    int userMessageSizeSent;
    int userMessageSizeReceived;

    int menuMode;
    int itemsPerPage = 7;
    int curPageOpenGames, totPageOpenGames;
    int itemsPerPageUsers = 10;
    int curPageUsers, totPageUsers;

    int requestedOpenGameID = -1;
    bool ggpoGo = false;
    bool testLoop = true;

    GameStartInfo gameStartInfo;
    GGPOStartInfo ggpoStartInfo;

    double curPing = 0;
    int pingCnt = 0;

public:
    // init
    // -- internal
    void initServer(const char* playerName, const char* ip, int port);
    void initENet();
    void closeENet();
    void setCallbacks();
    void onStart();
    void onCancel();

    // -- external
    void init(const char* playerName, const char* ip, int port);
    void init(const char* playerName);
    void close();

    // process
    // -- custom additional functions for callbacks
    void processCallbacks(bool checkPing);
    std::function<void(void)> customStartP2P;
    std::function<void(void)> customStartP2P_1;
    std::function<void(void)> customStartP2P_2;
    std::function<void(void)> customP2PConnected;

    std::function<void(void)> customSendUserMessage;
    std::function<void(void)> customReceiveUserMessage;

    std::function<void(void)> customStartGame;
    std::function<void(void)> customCancelGame;

    std::function<void(void)> customJoinRequestFromOtherPlayer;

    // -- commands
    void processServerCommands(int c);
    void processP2PCommands(int c);
    void processCommands(int c);

    // test
    void testClient();

    // misc
    void getPagesInfoOpenGames();
    void getPagesInfoUsers();
};