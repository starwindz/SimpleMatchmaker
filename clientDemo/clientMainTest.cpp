#include <stdio.h>
#include <stdint.h>
#include <conio.h>
 
#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <iostream>
 
#include "clientMain.h"
 
MatchMaker g_MatchMaker;
 
void customStartP2P()
{
}
 
void customStartP2P_1()
{
}
 
void customStartP2P_2()
{
}
 
void customP2PConnected()
{
}
 
void customSendUserMessage()
{
}
 
void customReceiveUserMessage()
{
}
 
void customStartGame()
{   
}
 
void customCancelGame()
{
}
 
void customJoinRequestFromOtherPlayer()
{
 
}
 
int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("invalid command line parameters\n");
        printf("usage: Client <name> <ServerIP> <SeverPort>\n");
        return 0;
    }
 
    g_MatchMaker.init(argv[1], argv[2], std::atoi(argv[3]));
    g_MatchMaker.customStartP2P = customStartP2P;
    g_MatchMaker.customStartP2P_1 = customStartP2P_1;
    g_MatchMaker.customStartP2P_2 = customStartP2P_2;
    g_MatchMaker.customP2PConnected = customP2PConnected;
    g_MatchMaker.customSendUserMessage = customSendUserMessage;
    g_MatchMaker.customReceiveUserMessage = customReceiveUserMessage;
    g_MatchMaker.customStartGame = customStartGame;
    g_MatchMaker.customCancelGame = customCancelGame;
    g_MatchMaker.customJoinRequestFromOtherPlayer = customJoinRequestFromOtherPlayer;
 
    g_MatchMaker.testClient();
 
    g_MatchMaker.close();
 
    return 1;
}