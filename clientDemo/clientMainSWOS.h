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
#define MMM_5_GGPO					5

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


<5: MMM_5_GGPO>
*/

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

    int menuMode;
    int itemsPerPage = 7;
    int curPageOpenGames, totPageOpenGames;

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
    std::function<void(void)> customOnStart;
    std::function<void(void)> customStartP2P;

    // -- commands
    void processServerCommands(int c);
    void processP2PCommands(int c);
    void processCommands(int c);

    // test
    void testClientLoop();
    void testClient();

    // misc
    void getPagesInfoOpenGames();
};