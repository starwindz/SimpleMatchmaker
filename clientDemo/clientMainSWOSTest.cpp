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

void customUpdateMatchMakingMenu()
{
    //printf("customUpdateMatchMakingMenu()\n");
}

int main(int argc, char** argv)
{
    if (!g_MatchMaker.checkParameters(argc)) return 0;

    g_MatchMaker.init(argv[1], argv[2], std::atoi(argv[3]));
    g_MatchMaker.updateMatchMakingMenu = customUpdateMatchMakingMenu;

    g_MatchMaker.testClient();

    g_MatchMaker.close();

    return 1;
}