#include <stdio.h>
#include <stdint.h>
#include <conio.h>

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "clientMainSWOS.h"

MatchMaker g_MatchMaker;

void customOnStart()
{
}

void customStartP2P()
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
    g_MatchMaker.customOnStart = customOnStart;
    g_MatchMaker.customStartP2P = customStartP2P;

    g_MatchMaker.testClient();

    g_MatchMaker.close();

    return 1;
}