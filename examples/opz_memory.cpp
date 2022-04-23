
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <ncurses.h>
#include "OPZ_RtMidi.h"

std::string version = "0.1";
std::string name = "opz_memory";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

std::atomic<bool> keepRunnig(true);

int main(int argc, char** argv) {

    T3::OPZ_RtMidi  opz;
    opz.connect();
    
    initscr();

    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    int y_beg, x_beg, y_max, x_max;
    getbegyx(stdscr, y_beg, x_beg);
    getmaxyx(stdscr, y_max, x_max);

    // Listen to key events (no cc, neighter notes)
    opz.setEventCallback( [&](T3::event_id _id, int _value) {
        
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

        clear();
        T3::project p = opz.getProject();
        u_int8_t pro = opz.getActiveProject();
        u_int8_t pat = opz.getActivePattern();
        T3::track_id t = opz.getActiveTrack();

        // Pattern Chain
        mvprintw(0,0, "            00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15    00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");
        for (size_t i = 0; i < 16; i++) {
            int y = 2 + i;
            mvprintw(y, 0, "pattern %02i  %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X    %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", i,
            p.pattern_chain[i].pattern[0], p.pattern_chain[i].pattern[1], p.pattern_chain[i].pattern[2], p.pattern_chain[i].pattern[3], p.pattern_chain[i].pattern[4], p.pattern_chain[i].pattern[5], p.pattern_chain[i].pattern[6], p.pattern_chain[i].pattern[7], p.pattern_chain[i].pattern[8], p.pattern_chain[i].pattern[9], p.pattern_chain[i].pattern[10], p.pattern_chain[i].pattern[11], p.pattern_chain[i].pattern[12], p.pattern_chain[i].pattern[13], p.pattern_chain[i].pattern[14], p.pattern_chain[i].pattern[15],
            p.pattern_chain[i].pattern[16], p.pattern_chain[i].pattern[17], p.pattern_chain[i].pattern[18], p.pattern_chain[i].pattern[19], p.pattern_chain[i].pattern[20], p.pattern_chain[i].pattern[21], p.pattern_chain[i].pattern[22], p.pattern_chain[i].pattern[23], p.pattern_chain[i].pattern[24], p.pattern_chain[i].pattern[25], p.pattern_chain[i].pattern[26], p.pattern_chain[i].pattern[27], p.pattern_chain[i].pattern[28], p.pattern_chain[i].pattern[29], p.pattern_chain[i].pattern[30],  p.pattern_chain[i].pattern[31]);
        }

        // mvprintw(0,0, "LEVEL %03i DRUMS %03i, SYNTH %03i, PUNCH %03i, MASTER %03i", (int)(opz.getVolume() * 100), p.drum_level, p.synth_level, p.punch_level, p.master_level);

        refresh();
    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}
