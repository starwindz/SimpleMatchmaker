#pragma once
#include <functional>
#include <enet/enet.h>
#include <string>
#include "Utils.h"
#include <vector>
#include <string>
struct GameStartInfo
{
    // The port you should use for the connection
    int port;

    // The list of candidate peerAddresses to try and connect to
    std::vector<ENetAddress> peerAddresses;
    
    // Which player you are - 1 or 2;
    int playerNumber;
};

struct GameInfoStruct
{
    GameInfoStruct(const std::string& msg);
    std::string ToString() const;
    std::string owner;
    std::vector<std::string> joined;
    std::vector<std::string> requested;
};

struct ServerCallbacks
{
    std::function<void()> Timeout;
    std::function<void()> Disconnected;    
    std::function<void()> Connected;

    /// You have successfully created a game and are waiting for players to join
    std::function<void()> GameCreatedOK;

    // A list of current users connected. Every time this changes, this event is called
    std::function<void(const std::vector<std::string>&)> UserList;

    // A list of current open games. Every time this change, this event is called
    std::function<void(const std::vector<std::string>&)> OpenGames;

    // Information about the current game you are involved in, either as host or joiner
    // Who hosted the game, who has joined, and who is pending
    std::function<void(const GameInfoStruct&)> GameInfo;

    // You are hosting a game and a player has asked to join
    std::function<void(const std::string&)> JoinRequestFromOtherPlayer;

    // Your request to join a game has been successful, now wait for the host to say yes or no
    std::function<void()> JoinRequestOK;

    std::function<void(const GameStartInfo&)> StartP2P;
};


enum class ServerConnectionState;
class ServerConnection
{
public:
    // Start Idle
    ServerConnection();

    // Start Connection immediately
    ServerConnection(const std::string& serverIP, int serverPort, const std::string& userName, const std::string& gameID);
    void Update(ServerCallbacks& callbacks);
    
    // Returns false if the already connected or connecting.
    bool Connect(const std::string& serverIP, int serverPort, const std::string& userName, const std::string& gameID);
    bool Disconnect();
    bool RequestToJoinGame(const std::string& gameOwner) const;
    bool LeaveGame() const;
    bool CreateGame() const;
    bool ApproveJoinRequest(const std::string& player);
    bool EjectPlayer(const std::string& player);
    bool IsConnected() const;

private:
    ENetAddress m_localAddress{ 0,0 };
    ENetHostPtr m_local;
    ENetAddress m_serverAddress{ 0,0 };
    ENetPeer* m_server=nullptr;

    std::string m_userName;
    std::string m_gameID;  
    ServerConnectionState m_state;
};