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
#include "clientMain.h"

#pragma comment(lib, "IPHLPAPI.lib")
 
template <typename T> inline void hash_combine(std::size_t& seed, const T& value)
{
    seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
 
template <class It> inline std::size_t hash_range(It first, It last)
{
    std::size_t h = 0;
    for (; first != last; ++first)
        hash_combine(h, *first);
    return h;
}
 
std::vector<char> RandomBuffer(int maxSize)
{
    int bufferSize = rand() % maxSize;
    std::vector<char> buffer(bufferSize);
    buffer[0] = 0;
    for (size_t i = 1; i < (size_t)bufferSize; i++)
        buffer[i] = rand() % 256;
    return buffer;
}
 
auto logger = [](const std::string& s) {std::cout << s; };
 
void MatchMaker::initServer(const char* playerName, const char* ip, int port)
{
    userBlob = RandomBuffer(100);
    blobHash = hash_range(userBlob.begin(), userBlob.end());
 
    loggedPlayerName = playerName;
    serverIP = ip;
    serverPort = port;
 
    serverConnection = std::make_unique<ServerConnection>(serverIP, serverPort, loggedPlayerName, userBlob.data(), userBlob.size(), "SWOS_MatchMaker", logger);
    p2pClient = nullptr;
 
    std::cout << "Attempt connection to " << serverIP << ":" << serverPort << ", with name " << loggedPlayerName << "\n";
}
 
void MatchMaker::initENet()
{
    enet_initialize();
}
 
void MatchMaker::closeENet()
{
    enet_deinitialize();
}
 
void MatchMaker::setCallbacks()
{
    // ***********************************************************************************************************
    // Callbacks from the server, react as required in SWOS. In this example we mainly just print
    // to console to show the user
    severCbs.Connected = []()
    {
        //std::cout << "[CALLBACK] severCbs.Connected\n";
        std::cout << "Connected to server. Press g to create a game. d to disconnect.\n";
    };
 
    severCbs.Disconnected = []()
    {
        //std::cout << "[CALLBACK] severCbs.Disconnected\n";
        std::cout << "Disconnected from server\n";
    };
 
    severCbs.Timeout = []()
    {
        //std::cout << "[CALLBACK] severCbs.Timeout\n";
        std::cout << "Timeout trying to connect to server\n";
    };
 
    severCbs.JoinRequestFromOtherPlayer = [&](const std::string& userName)
    {
        //std::cout << "[CALLBACK] severCbs.JoinRequestFromOtherPlayer\n";
        std::cout << "**[MM_3] " << userName << " Wants to join. y - allow. n - deny. l - leave game.\n";
        menuMode = MMM_3_ALLOW_DENY_LEAVE;
        customJoinRequestFromOtherPlayer();
    };
 
    severCbs.JoinRequestOK = [&]()
    {
        //std::cout << "[CALLBACK] severCbs.JoinRequestOK\n";
        if (userRemoved) {
            userRemoved = true;
        }
        else {
            std::cout << "**[MM_2] " << "Requsted to join game. Waiting for host to respond. Press l to leave.\n";
            menuMode = MMM_2_REQUESTED_TO_JOIN;           
            userRemoved = false;
        }
    };
 
    severCbs.GameCreatedOK = []()
    {
        //std::cout << "[CALLBACK] severCbs.GameCreatedOK\n";
        std::cout << "We successfully created a game. Waiting for others to join. Press l to leave game.\n";
    };
 
    severCbs.StartP2P = [&](const GameStartInfo& info)
    {
        //std::cout << "[CALLBACK] severCbs.StartP2P\n";
        std::cout << "**[MM_4] Ready to Start game, info:\n" << info.ToString();
        customStartP2P_1();
        p2pClient.reset(new P2PConnection(info, [](const std::string& s) {std::cout << s; }));
        customStartP2P_2();
        menuMode = MMM_4_LOBBY;
 
        gameStartInfo = info;
        customStartP2P();
    };
 
    severCbs.LeftGameOK = [&]()
    {
        //std::cout << "[CALLBACK] severCbs.LeftGameOK\n";
        std::cout << "We left the game.\n";
        userRemoved = true;
        requestedJoiners.clear();
    };
 
    severCbs.RemovedFromGame = [&]()
    {
        //std::cout << "[CALLBACK] severCbs.RemovedFromGame\n";
        std::cout << "We were removed from the game.\n";
        userRemoved = true;
        requestedJoiners.clear();
    };
 
    severCbs.Approved = [](const std::string& name)
    {
        //std::cout << "[CALLBACK] severCbs.Approved\n";
        std::cout << "Approved the join request of " << name << "\n";
    };
 
    severCbs.UserList = [&](const std::vector<PlayerInfo>& userNames)
    {
        users = userNames;
        //std::cout << "[CALLBACK] severCbs.UserList\n";
        std::cout << "Active Users: ";
        for (const auto& u : userNames) {
            auto userDataHash = hash_range(u.data.begin(), u.data.end());
            std::cout << u.name << "(hash:" << userDataHash % 10000 << "), ";
        }
 
        std::cout << "\n";
        getPagesInfoUsers();
    };
 
    severCbs.ServerMessage = [](const std::string& msg)
    {
        //std::cout << "[CALLBACK] severCbs.ServerMessage\n";
        std::cout << "Server Message: " << msg << "\n";
    };
 
    severCbs.OpenGames = [&](const std::vector<std::string>& games)
    {
        if (menuMode == MMM_3_ALLOW_DENY_LEAVE && requestedJoiners.size() > 0) return;
 
        //std::cout << "[CALLBACK] severCbs.OpenGames\n";
        openGames = games;
        std::cout << "Open Games: ";
        bool isHosting = std::find(RANGE(games), loggedPlayerName) != games.end();
        int i = 1;
        for (const auto& u : openGames) {
            std::cout << i << ":" << u;
            if (u == loggedPlayerName)
                std::cout << "[You]";
            std::cout << ", ";
            i++;
        }
 
        if (openGames.size() == 0) {
            std::cout << "<none>";
            std::cout << "\n";
            std::cout << "**[MM_0] Press g to open a game\n";
            menuMode = MMM_0_DEFAULT;
            userRemoved = false;
        }
        else {
            std::cout << "\n";
            /*
            if (userRemoved) {
                std::cout << "**[MM_0] Press <number> to request to join a game (userRemoved)\n";
                menuMode = MMM_0_DEFAULT;
            }
            else {
                if (!isHosting) {
                    bool joinerFound = std::find(RANGE(requestedJoiners), loggedPlayerName) != requestedJoiners.end();
                    if (joinerFound) {
                        std::cout << "**[MM_2] " << "Requsted to join game. Waiting for host to respond. Press l to leave.\n";
                        menuMode = MMM_2_REQUESTED_TO_JOIN;
                        userRemoved = false;
                    }
                    else {
                        std::cout << "**[MM_0] Press <number> to request to join a game\n";
                        menuMode = MMM_0_DEFAULT;
                    }
                }
                else {
                    std::cout << "**[MM_1] Waiting for others to join\n";
                    menuMode = MMM_1_WAIT_TO_JOIN;
                }
            }
            */
            if (!isHosting) {
                if (userRemoved) {
                    std::cout << "**[MM_0] Press <number> to request to join a game (userRemoved)\n";
                    menuMode = MMM_0_DEFAULT;
                }
                else {
                    bool joinerFound = std::find(RANGE(requestedJoiners), loggedPlayerName) != requestedJoiners.end();
                    if (joinerFound) {
                        std::cout << "**[MM_2] " << "Requsted to join game. Waiting for host to respond. Press l to leave.\n";
                        menuMode = MMM_2_REQUESTED_TO_JOIN;
                        userRemoved = false;
                    }
                    else {
                        std::cout << "**[MM_0] Press <number> to request to join a game\n";
                        menuMode = MMM_0_DEFAULT;
                    }
                }
            }
            else {
                //if (userRemoved) {
                //    std::cout << "**[MM_0] Press <number> to request to join a game (userRemoved)\n";
                //    menuMode = MMM_0_DEFAULT;
                //}
                //else {
                    std::cout << "**[MM_1] Waiting for others to join\n";
                    menuMode = MMM_1_WAIT_TO_JOIN;
                //}
            }
        }
 
        getPagesInfoOpenGames();
    };
 
    severCbs.GameInfo = [&](const GameInfoStruct& info)
    {
        //std::cout << "[CALLBACK] severCbs.GameInfo\n";
        requestedJoiners = info.requested;
        joined = info.joined;
        curJoinerIndex = requestedJoiners.size() - 1;
        if (curJoinerIndex < 0) curJoinerIndex = 0;
        userRemoved = false;
 
        std::cout << "GameInfo: " << info.ToString() << "\n";
    };
 
    //****************************************************************************************************************
    // Callbacks for the P2P (lobby phase). In this example we respond to usermessage by echoing back the
    // hash of the message, so the sender can visually see it's correct  
    p2pCbs.ReceiveUserMessage = [&](const void* buffer, size_t sz)
    {
        //std::cout << "[CALLBACK] cbs.ReceiveUserMessage\n";
        userMessageSizeReceived = sz;
        memcpy(userMessageReceived, (char*)buffer, userMessageSizeReceived);
 
        size_t hsh = hash_range((const char*)buffer, (const char*)buffer + sz);
        std::cout << "Received User Message, length " << sz << " hash: " << hsh % 10000 << "\n";
 
        /*
        auto buf = (const char*)buffer;
        if (buf[0] == 0) {
            size_t hsh = hash_range((const char*)buffer, (const char*)buffer + sz);
            std::cout << "Received User Message, length " << sz << " hash: " << hsh % 10000 << "\n";
            std::vector<char> response(1 + sizeof(size_t));
            response[0] = 1;
            *(size_t*)(&response[1]) = hsh;
 
            p2pClient->SendUserMessage(response.data(), response.size());
        }
        else if (buf[0] == 1) {
            size_t* hsh = (size_t*)&buf[1];
            std::cout << "Received acknowledgement, peer thinks hash was " << (*hsh) % 10000 << "\n";
        }
        */
 
        customReceiveUserMessage();
    };
 
    p2pCbs.Connected = [&]()
    {
        std::cout << "P2PCallback:*********** Connection established ******\n";
        customP2PConnected();
    };
   
    p2pCbs.Disconncted = [&]()
    {
        std::cout << "P2PCallback:*********** Connection lost ******\n";
        p2pClient = nullptr;
    };
   
    p2pCbs.Timeout = [&]()
    {
        std::cout << "P2PCallback:*********** Connection attempt timed out ******\n";
        p2pClient = nullptr;
    };
   
    p2pCbs.ReadyStatusChanged = []()
    {
        std::cout << "P2PCallback:************** Other player changed ready status\n";
    };
   
    p2pCbs.StartGame = [&](const GGPOStartInfo& info)
    {
        std::cout << "P2PCallback:************** Ready to start GGPO Session as player " << info.yourPlayerNumber <<"\n";
        std::cout << "Use local port: " << info.yourPort<< ", ";
        std::cout << "Connect to opponent at " << info.opponentIP << ", port: " << info.opponentPort << "\n";
      
        p2pClient = nullptr;
        std::cout << "P2PConnection closed and game can start, killed p2pClient\n";
       
        std::cout << "[...] StartGame\n";
        std::cout << "**[MM_5] P2PConnection closed and game can start, killed p2pClient\n";
        ggpoGo = true;
 
        ggpoStartInfo = info;
        customStartGame();
    }; 
    
    p2pCbs.CancelGame = [&]()
    {
        std::cout << "P2PCallback:************** Ready to cancel GGPO Session\n";
        p2pClient = nullptr;
 
        std::cout << "[...] CancelGame\n";
        std::cout << "**[MM_5] P2PConnection closed and game can cancel, killed p2pClient\n";
        ggpoGo = false;
 
        customCancelGame();
    }; 
}
 
void MatchMaker::init(const char* playerName, const char* ip, int port)
{
    srand((int)time(NULL));
    initENet();
    initServer(playerName, ip, port);
    requestedJoiners.clear();
 
    setCallbacks();
 
    menuMode = MMM_X_SERVER_NOT_CONNECTED;
    userRemoved = false;
    curJoinerIndex = 0;
    ggpoGo = false;
 
    testLoop = true;
    curPing = 0;
    pingCnt = 0;
}
 
void MatchMaker::init(const char* playerName)
{
    // load server ip address
    char ip[32] = { 0, };
    int port = 19602;
    FILE* fp;
 
    fp = fopen("server.txt", "rt");
    if (fp == NULL) {
        fp = fopen("server.txt", "wt");
        strcpy(ip, "127.0.0.1");
        fprintf(fp, "%s", ip);
       fclose(fp);
    }
    else {
        fscanf(fp, "%s", ip);
        fclose(fp);
    }
 
    srand((int)time(NULL));
    initENet();
    initServer(playerName, ip, port);
    requestedJoiners.clear();
 
    setCallbacks();
 
    menuMode = MMM_X_SERVER_NOT_CONNECTED;
    userRemoved = false;
    curJoinerIndex = 0;
    ggpoGo = false;
 
    testLoop = true;
    curPing = 0;
    pingCnt = 0;
}
 
void MatchMaker::close()
{
    serverConnection->Disconnect();
    closeENet();
}
 
void MatchMaker::processCallbacks(bool checkPing)
{
    if (serverConnection)
        serverConnection->Update(severCbs);
 
    if (p2pClient) {
        p2pClient->Update(p2pCbs);     
        if (checkPing) {
            if (int(curPing) == 0) {
                processP2PCommands('p');
                customSendUserMessage();
            }
        }
    }
}
 
void MatchMaker::processServerCommands(int c)
{
    if (c >= 1000) {
        int idx = c - 1000;
        // std::cout << "pressed join game " << idx << "\n";
        if (openGames.size() > 0 && idx < (int)openGames.size())
            serverConnection->RequestToJoinGame(openGames[idx]);
    }
    else if (c == 'c') {
        //  std::cout << "pressed connect\n";
        if (serverConnection->Connect(serverIP, serverPort, loggedPlayerName, userBlob.data(), userBlob.size(), "SWOS_MatchMaker")) {
            std::cout << "Attempt connection to " << serverIP << ":" << serverPort << ", with name " << loggedPlayerName << "\n";
        }
    }
    else if (c == 'd') {
        //  std::cout << "pressed disconncet\n";
        serverConnection->Disconnect();
    }
    else if (c == 'g') {
        //  std::cout << "pressed create\n";
        serverConnection->CreateGame();
    }
    else if (c == 'l') {
        //  std::cout << "pressed leave\n";
        serverConnection->LeaveGame();
    }
    else if (c == 'y') {
        printf("requestedJoiners.size() = %d\n", requestedJoiners.size());
        if (requestedJoiners.size() >= 1) {
            //  std::cout << "pressed Yes to request to join from " << requestedJoiners[0] << "\n";
            serverConnection->ApproveJoinRequest(requestedJoiners[0]);
        }
    }
    else if (c == 'Y') {
        printf("requestedJoiners.size() = %d\n", requestedJoiners.size());
        if (requestedJoiners.size() >= 2) {
            //  std::cout << "pressed Yes to request to join from " << requestedJoiners[0] << "\n";
            serverConnection->ApproveJoinRequest(requestedJoiners[1]);
        }
    }
    else if (c == 'n') {
        printf("requestedJoiners.size() = %d\n", requestedJoiners.size());
        if (requestedJoiners.size() >= 1) {
            //  std::cout << "pressed No to request to join from " << requestedJoiners[0] << "\n";
            serverConnection->EjectPlayer(requestedJoiners[0]);
        }
    }
    else if (c == 'N') {
        printf("requestedJoiners.size() = %d\n", requestedJoiners.size());
        if (requestedJoiners.size() >= 2) {
            //  std::cout << "pressed No to request to join from " << requestedJoiners[0] << "\n";
            serverConnection->EjectPlayer(requestedJoiners[1]);
        }
    }
    else if (c == 'k') {
        if (joined.size()) {
            //  std::cout << "press kick out to  " << joined[0] << "\n";
            serverConnection->EjectPlayer(joined[0]);
        }
    }
}
 
void MatchMaker::processP2PCommands(int c)
{
    if (c == 'd') {
        std::cout << "Killing p2pClient\n";
        p2pClient = nullptr;
    }
    else if (c == 'p') {
        curPing = p2pClient->GetPing();
        std::cout << "Ping to opponent is " << curPing << "\n";
    }
    else if (c == 'r') {
        p2pClient->SendReady(true);
        //p2pClient->ToggleReady();
    }
    else if (c == 'u') {
        p2pClient->SendReady(false);
        //p2pClient->ToggleReady();
    }
    else if (c == 'l') {
        p2pClient = nullptr;
    }
    else if (c == 'i') {
        p2pClient->Info();
    }
    else if (c == 's') {
        p2pClient->TryStart();
    }
    else if (c == 'a') {
        p2pClient->TryCancel();
    }
    else if (c == 'm') {
        p2pClient->SendUserMessage(userMessageSent, userMessageSizeSent);
    }
}
 
void MatchMaker::processCommands(int c)
{
    // command: match making
    if (!p2pClient) {
        if (c >= '1' && c <= '9') c = 1000 + c - '1';
        processServerCommands(c);
    }
    // command: game lobby
    else {
        processP2PCommands(c);
    }
}
 
void MatchMaker::testClient()
{
    while (testLoop) {
        Sleep(1);
        processCallbacks(false);  // <--
 
        if (_kbhit()) {
            auto c = _getch();
            if (c == 'q') {
                testLoop = false;
            }
 
            if (c == 'm') {
                auto buffer = RandomBuffer(300);
                std::cout << "Sending a random user message, length " << buffer.size() << " hash: " << hash_range(buffer.begin(), buffer.end()) % 10000 << "\n";
 
                userMessageSizeSent = buffer.size();
                memcpy(userMessageSent, buffer.data(), userMessageSizeSent);
            }
            processCommands(c);  // <--
        }
    }
}
 
void MatchMaker::getPagesInfoOpenGames()
{
    int numItems;
    int prevCurPageOpenGames = curPageOpenGames;
 
    numItems = openGames.size();
    if (numItems % itemsPerPage == 0) {
        totPageOpenGames = numItems / itemsPerPage;
        if (totPageOpenGames == 0) totPageOpenGames = 1;
    }
    else {
        totPageOpenGames = numItems / itemsPerPage + 1;
    }
   
    if (prevCurPageOpenGames < totPageOpenGames - 1) {
        curPageOpenGames = totPageOpenGames - 1;
    }
    else {
        curPageOpenGames = prevCurPageOpenGames;
    }
}
 
void MatchMaker::getPagesInfoUsers()
{
    int numItems;
    int prevCurPageUsers = curPageUsers;
 
    numItems = users.size();
    if (numItems % itemsPerPageUsers == 0) {
        totPageUsers = numItems / itemsPerPageUsers;
        if (totPageUsers == 0) totPageUsers = 1;
    }
    else {
        totPageUsers = numItems / itemsPerPageUsers + 1;
    }
 
    if (prevCurPageUsers < totPageUsers - 1) {
        curPageUsers = totPageUsers - 1;
    }
    else {
        curPageUsers = prevCurPageUsers;
    }
}

 
