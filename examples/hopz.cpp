
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <ncurses.h>
#include "OPZ_RtMidi.h"

std::string version = "0.1";
std::string name = "opz_dump";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

std::atomic<bool> keepRunnig(true);

int main(int argc, char** argv) {

    T3::OPZ_RtMidi  opz;
    opz.connect();
    
    initscr();
    // raw();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    // opz.verbose = 1; // message type
    // opz.verbose = 2; // hex values
    // opz.verbose = 3; // interpreted message

    // // Listen to key events (no cc, neighter notes)
    opz.setEventCallback( [](T3::event_id _id, int _value) {
        clear();
        printw("%s %i\n", T3::OPZ::toString(_id).c_str(), _value );
        refresh();
    } );

    std::thread waitForKeys([&](){
        char ch;
        while ( (ch = getch()) != KEY_F(1) ) {
            if (ch = 'q') {
                keepRunnig = false;
                keepRunnig.store(false);
                break;
            }
        }
    });

    while (keepRunnig.load()) {
        opz.keepawake();
        refresh();
    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}
