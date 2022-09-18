/*
1. adding callback functions for P2PConnection
- Connected
- Disconncted  (// not needed?)
- PlayerReady
- StartGame

2. adding public functions for P2PConnection
- void SendReady(bool status);  // status is true or false
- bool MeReady();    // returns m_bMeReady
- bool OtheReady();  // returns m_bOtherReady
- bool PrimaryConnectionEstablished(); // returns m_bPrimaryConnectionEstablished
*/

#pragma once
#include <functional>
#include <enet/enet.h>
#include <string>
#include "Utils.h"
#include <vector>
#include <string>
#include "ServerConnection.h"
#include <chrono>

struct P2PCallbacks
{
    std::function<void()> Connected;
    //std::function<void()> Disconncted;
    //std::function<void()> PlayersReady;
    //std::function<void()> StartGame;
    std::function<void(const void*,size_t)> ReceiveUserMessage;
};

class PingHandler
{
public:
    PingHandler();
    void Update(ENetPeer* peerToPing);
    void OnPong();
    double GetPing() const { return m_pingEMA; }
private:
    // Exponential moving average, taken twice a second, over the last 15 seconds
    double m_pingEMA = 0;
    const int emaPeriodMS = 15 * 1000;
    const int pingPeriodMS = 500;
    const double EMA_Constant = 2 / (1.0 + (emaPeriodMS / pingPeriodMS));
    unsigned int m_bPingSent =false;
    std::chrono::steady_clock::time_point lastPing;
};

class P2PConnection
{
    // Can't copy or move
    P2PConnection(const P2PConnection&) = delete;
    P2PConnection& operator=(const P2PConnection&) = delete;
    P2PConnection(const P2PConnection&&) = delete;
    P2PConnection& operator=(const P2PConnection&&) = delete;
public:
    P2PConnection(GameStartInfo info,std::function<void(const std::string&)> logger);
    ~P2PConnection();    
    void SendReady();
    void SendReady(bool status);
    void Update(P2PCallbacks& callbacks);
    bool ReadyToStart() const;
    bool ReadyToCancel() const;
    void Info();
    void SendStart();
    void SendCancel();
    double GetPing() const;
    void SendUserMessage(char* buffer, size_t length);
private:
    GameStartInfo m_info;
    ENetAddress localAddress{ 0,0 };
    ENetHostPtr local;
    std::vector<ENetPeer*> outGoingPeerCandidates;
    std::vector<ENetPeer*> peerConnections;
    bool m_bMeReady=false;
    bool MeReady()
    {
        return m_bMeReady;
    }
    bool m_bOtherReady=false;
    bool OtherReady()
    {
        return m_bMeReady;
    }
    bool m_Start = false;
    bool m_Cancel = false;
    void OnReadyChange();
    
    bool m_bPrimaryConnectionEstablished = false;
    bool PrimaryConnectionEstablished()
    {
        return m_bPrimaryConnectionEstablished;
    }
    size_t TotalActivePeers() const {
        return outGoingPeerCandidates.size() + peerConnections.size();
    }
    std::function<void(std::string)> m_logger;
    void CleanRedundantConnections(P2PCallbacks& callbacks);
    PingHandler m_pingHandler;
    enum class P2PState { Connecting, Connected, Ready};
};