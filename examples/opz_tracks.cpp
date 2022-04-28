
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <ncurses.h>
#include "opz_rtmidi.h"

std::string version = "0.1";
std::string name = "opz_tracks";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

std::atomic<bool> keepRunnig(true);

int main(int argc, char** argv) {

    T3::opz_rtmidi opz;
    opz.connect();
    
    initscr();
    start_color();

    init_color(COLOR_BLACK, 0, 0, 0);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    bool change = true;
    bool pressing_track = false;
    bool pressing_project = false;
    bool pressing_mixer = false;
    bool pressing_tempo = false;
    bool mic_on = false;

    // // Listen to key events (no cc, neighter notes)
    // opz.setEventCallback( [&](T3::opz_event_id _id, int _value) {
    // } );

    std::thread waitForKeys([](){
        char ch;
        while ( true ) {
            ch = getch();
            if (ch == 'q') {
                keepRunnig = false;
                keepRunnig.store(false);
                break;
            }
        }
    });

    int y_beg, x_beg, y_max, x_max;
    while (keepRunnig.load()) {
        getbegyx(stdscr, y_beg, x_beg);
        getmaxyx(stdscr, y_max, x_max);

        opz.keepawake();

        T3::opz_pattern pattern = opz.getActivePattern();

        clear();
        for (size_t y = 0; y < 17; y++) 
            for (size_t x = 0; x < 17; x++) {
                if (y == 0 && x > 0)
                    mvprintw(y, 7 + x * 5, "%02i ", x);
                else {
                    if (x == 0 && y < 16)
                        mvprintw(y+2 , 0, "%7s", T3::toString( T3::opz_track_id(y) ).c_str() );
                    
                    if (x > 0) {
                        size_t i = opz.getNoteIdOffset(y-1, x-1);
                        // mvprintw(y+1, x * 5 + 7, "%i", i );
                        mvprintw(y+1, x * 5 + 7, "%i", pattern.note[ i ].duration );

                        size_t track = y;
                        size_t step = x;
                    }
                } 
            }

        refresh();
    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}
