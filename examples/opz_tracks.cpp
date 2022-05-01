
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <ncurses.h>
#include "opz_rtmidi.h"
#include "tools.h"

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

    std::thread waitForKeys([&](){
        char ch;
        while ( true ) {
            ch = getch();
            if (ch == 'q') {
                opz.saveAsTxt("project" + std::to_string( opz.getActiveProjectId() + 1) + ".txt");

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

        opz.update();

        size_t project_id = opz.getActiveProjectId();
        T3::opz_pattern pattern = opz.getActivePattern();
        size_t pattern_id = opz.getActivePatternId();
        size_t track_id = opz.getActiveTrackId();

        // clear();
        mvprintw(0, (x_max-x_beg)/2 - 12, "PROJECT %02i | PATTERN %02i", project_id, pattern_id );

        int x_width = 6;
        for (size_t y = 0; y < 16; y++) {
                mvprintw(2, 12 + y * x_width, "%02i ", y+1);

                if (y == track_id)
                    attron(COLOR_PAIR(2));

                mvprintw(y+4 , 2, "%7s", T3::toString( T3::opz_track_id(y) ).c_str() );

                // mvprintw(x+2 , 10 + 16 * x_width, "%i", pattern.track_param[x].step_count );
                // mvprintw(x+2 , 10 + 16 * x_width + 5, "%i", pattern.track_param[x].step_length );
                // mvprintw(x+2 , 10 + 16 * x_width + 2, "%i", pattern.track_param[x].note_length );

            for (size_t x = 0; x < 16; x++) {

                size_t i = opz.getNoteIdOffset(y, x);
                // mvprintw(y+4, 12 + x * x_width, "%i", i );
                if ( pattern.note[ i ].note == 0xFF)
                    mvprintw(y+4, 12 + x * x_width, " .");
                else
                    mvprintw(y+4, 12 + x * x_width, "%02X", pattern.note[ i ].note );
                // mvprintw(y+4, 12 + x * x_width, "%.2f", pattern.note[ i ].duration / 4294967295.0 );
                // mvprintw(y+4, 12 + x * x_width, "%02X", pattern.note[ i ].velocity );
                // mvprintw(y+4, 12 + x * x_width, "%i", pattern.note[ i ].micro_adjustment );
                // mvprintw(y+4, 12 + x * x_width, "%02X", pattern.note[ i ].age );

                // mvprintw(y+4, 12 + x * x_width, "%04X", pattern.step[ x * 16 + y ].components_bitmask );

            }

            attroff(COLOR_PAIR(2));
        }

        refresh();
    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}
