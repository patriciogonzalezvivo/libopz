#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>

#include "OPZ_RtMidi.h"

std::string version = "0.1";
std::string name = "opz_dump";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 


int main(int argc, char** argv) {
    T3::OPZ_RtMidi  opz;
    opz.connect();

    while (true)
        opz.keepawake();
    
    opz.disconnect();

    exit(0);
}
