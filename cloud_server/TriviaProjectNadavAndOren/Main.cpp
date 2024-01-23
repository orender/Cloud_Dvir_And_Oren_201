#include "Winsock2.h"
#include "container.h"
#include "dataSplitter.h"
#include <iostream>
#include "DBHelper.hpp"

int main() {
    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    dataSplitter ds = dataSplitter("test.db");

    ds.saveNewFile("beemovie", "I'm giving you a night call to tell you how I feel (We'll go all, all, all night long)\nI want to drive you through the night, down the hills(We'll go all, all, all night long)\nI'm gonna tell you something you don't want to hear(We'll go all, all, all night long)\nI'm gonna show you where it's dark, but have no fear(We'll go all, all, all night long)\nThere's something inside you\nIt's hard to explain\nThey're talking about you, boy\nBut you're still the same\nThere's something inside youIt's hard to explain\nThey're talking about you, boy\nBut you're still the same\nI'm giving you a night call to tell you how I feel (We'll go all, all, all night long)\nI want to drive you through the night, down the hills (We'll go all, all, all night long)\nI'm gonna tell you something you don't want to hear (We'll go all, all, all night long)\nI'm gonna show you where it's dark, but have no fear (We'll go all, all, all night long)\nThere's something inside you\nIt's hard to explain\nThey're talking about you, boy\nBut you're still the same\nThere's something inside you\nIt's hard to explain\nThey're talking about you boy\nBut you're still the same\nThere's something inside you (there's something inside you)\nIt's hard to explain (It's hard to explain)\nThey're talking about you boy (They're talking about you boy)\nBut you're still the same");
    std::cout << ds.getFileData("beemovie") << std::endl;
    ds.saveNewFile("fortnite_lore", "The Fortnite character Midas is killed by a shark, turning him into a vengeful ghost, as the agencies GHOST and SHADOW found new locations on the map. Eventually, spaceships began to appear throughout the island, re-opening the rifts in the sky. Elsewhere, in another reality, Marvel Comics' Thor is working with Galactus, eventually realizing Galactus was evil and needed to be destroyed. He brought several Marvel characters through the rift to the Fortnite map to aid in the fight against Galactus.");
    std::cout << ds.getFileData("fortnite_lore") << std::endl;
    std::cout << ds.getFiles() << std::endl;
    ds.deleteFile("beemovie");
    std::cout << ds.getFiles() << std::endl;


    WSACleanup();
    /*WSADATA              wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    unsigned int         PortContainer1 = 12345;
    unsigned int         PortContainer2 = 12350;
    unsigned int         PortContainer3 = 12355;


    container c1 = container("127.0.0.1", PortContainer1);
    std::cout << c1.start();
    c1.save("ok", "000001");
    c1.getBlob("000001");
    c1.deleteBlob(1);

    WSACleanup();*/

    return 0;
}