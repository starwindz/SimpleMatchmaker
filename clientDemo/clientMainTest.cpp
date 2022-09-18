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

void customP2PConnected()
{
}

void customSendUserMessage()
{
}

void customReceiveUserMessage()
{
}

void customOnStart()
{    
}

void customOnCancel()
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
    g_MatchMaker.customP2PConnected = customP2PConnected;
    g_MatchMaker.customSendUserMessage = customSendUserMessage;
    g_MatchMaker.customReceiveUserMessage = customReceiveUserMessage;
    g_MatchMaker.customOnStart = customOnStart;
    g_MatchMaker.customOnCancel = customOnCancel;

    g_MatchMaker.testClient();

    g_MatchMaker.close();

    return 1;
}