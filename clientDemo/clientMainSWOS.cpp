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
#include "clientMainSWOS.h"

struct TestStruct {
    char a;
    char b;
};

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

void MatchMaker::initServer(char* playerName, char* ip, int port)
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
    };

    severCbs.JoinRequestOK = [&]()
    {
        //std::cout << "[CALLBACK] severCbs.JoinRequestOK\n";
        std::cout << "**[MM_2] " << "Requsted to join game. Waiting for host to respond. Press l to leave.\n";
        menuMode = MMM_2_REQUESTED_TO_JOIN;
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
        p2pClient.reset(new P2PConnection(info, [](const std::string& s) {std::cout << s; }));
        menuMode = MMM_4_LOBBY;
    };

    severCbs.LeftGameOK = []()
    {
        //std::cout << "[CALLBACK] severCbs.LeftGameOK\n";
        std::cout << "We left the game.\n";
    };

    severCbs.RemovedFromGame = []()
    {
        //std::cout << "[CALLBACK] severCbs.RemovedFromGame\n";
        std::cout << "We were removed from the game.\n";
    };

    severCbs.Approved = [](const std::string& name)
    {
        //std::cout << "[CALLBACK] severCbs.Approved\n";
        std::cout << "Approved the join request of " << name << "\n";
    };

    severCbs.UserList = [](const std::vector<PlayerInfo>& userNames)
    {
        //std::cout << "[CALLBACK] severCbs.UserList\n";
        std::cout << "Active Users: ";
        for (const auto& u : userNames) {
            auto userDataHash = hash_range(u.data.begin(), u.data.end());
            std::cout << u.name << "(hash:" << userDataHash % 10000 << "), ";
        }

        std::cout << "\n";
    };

    severCbs.ServerMessage = [](const std::string& msg)
    {
        //std::cout << "[CALLBACK] severCbs.ServerMessage\n";
        std::cout << "Server Message: " << msg << "\n";
    };

    severCbs.OpenGames = [&](const std::vector<std::string>& games)
    {
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
        }
        else {
            std::cout << "\n";
            if (!isHosting) {
                std::cout << "**[MM_0] Press <number> to request to join a game\n";
                menuMode = MMM_0_DEFAULT;
            }
            else {
                std::cout << "**[MM_1] Waiting for others to join\n";
                menuMode = MMM_1_WAIT_TO_JOIN;
            }
        }

        updateMatchMakingMenu();
    };

    severCbs.GameInfo = [&](const GameInfoStruct& info)
    {
        //std::cout << "[CALLBACK] severCbs.GameInfo\n";
        requestedJoiners = info.requested;
        joined = info.joined;
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
    };
}

void MatchMaker::onStart()
{
    std::cout << "[...] onStart\n";
    std::cout << "**[MM_5] P2PConnection closed and game can start, killed p2pClient\n";
    p2pClient = nullptr;
    menuMode = MMM_5_GGPO;
};

void MatchMaker::init(char* playerName, char* ip, int port)
{
    srand((int)time(NULL));
    initENet();
    initServer(playerName, ip, port);
    setCallbacks();

    loop = true;
}

void MatchMaker::close()
{
    serverConnection->Disconnect();
    closeENet();
}

void MatchMaker::processCallbacks()
{
    serverConnection->Update(severCbs);

    if (p2pClient) {
        p2pClient->Update(p2pCbs);
        if (p2pClient->ReadyToStart())
            onStart();
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
        if (serverConnection->Connect(serverIP, serverPort, loggedPlayerName, userBlob.data(), userBlob.size(), "SWOS_MatchMaker"))
            std::cout << "Attempt connection to " << serverIP << ":" << serverPort << ", with name " << loggedPlayerName << "\n";
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
        if (requestedJoiners.size()) {
            //  std::cout << "pressed Yes to request to join from " << requestedJoiners[0] << "\n";
            serverConnection->ApproveJoinRequest(requestedJoiners[0]);
        }
    }
    else if (c == 'n') {
        if (requestedJoiners.size()) {
            //  std::cout << "pressed No to request to join from " << requestedJoiners[0] << "\n";
            serverConnection->EjectPlayer(requestedJoiners[0]);
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
        auto ping = p2pClient->GetPing();
        std::cout << "Ping to opponent is " << ping << "\n";
    }
    else if (c == 'r') {
        p2pClient->SendReady();
    }
    else if (c == 'l') {
        p2pClient = nullptr;
    }
    else if (c == 'i') {
        p2pClient->Info();
    }
    else if (c == 's') {
        p2pClient->SendStart();
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

void MatchMaker::testClientLoop()
{
    processCallbacks();  // <--

    if (_kbhit()) {
        auto c = _getch();
        if (c == 'q') {
            loop = false;
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

void MatchMaker::testClient()
{
    while (loop) {
        testClientLoop();
    }
}