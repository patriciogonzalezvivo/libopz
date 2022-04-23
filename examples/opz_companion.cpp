
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <ncurses.h>
#include "OPZ_RtMidi.h"

std::string version = "0.1";
std::string name = "opz_companion";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

std::atomic<bool> keepRunnig(true);

std::string hBar(size_t _width, size_t _value) {
    std::string rta = "";
    size_t l = (_value/254.0) * _width;
    for (size_t i = 0; i < _width; i++)
        rta += (i < l ) ? "#" : ".";
    return rta;
}

int main(int argc, char** argv) {

    T3::OPZ_RtMidi  opz;
    opz.connect();
    
    initscr();

    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    // Listen to key events (no cc, neighter notes)
    opz.setEventCallback( [&](T3::event_id _id, int _value) {
        refresh();
    } );

    std::thread waitForKeys([&](){
        char ch;
        while ( (ch = getch()) != KEY_F(1) ) {
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
        clear();

        uint8_t pattern = opz.getActivePattern();
        T3::track_id track = opz.getActiveTrack();
        std::string track_name =  T3::OPZ::toString(track);
        T3::project project = opz.getProject();
        T3::track_chunck tc = project.pattern[pattern].track[track];

        mvprintw(0, (x_max-x_beg)/2 - track_name.size()/2, "%s", track_name.c_str() );

        mvprintw(2, 0, "SOUND   P1      P2      FILTER  RESONA.");
        mvprintw(3, 0, "        %s %s %s %s",   hBar(7, (size_t)opz.getActiveTrackParameters().param1).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().param2).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().filter).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().resonance).c_str() );
        mvprintw(4, 0, "        %03i     %03i     %03i     %03i", 
                                                (int)((int)opz.getActiveTrackParameters().param1 / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().param2 / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().filter / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().resonance / 2.55f) );
        mvprintw(6, 0, "ENV.    ATTACK  DECAY   SUSTAIN RELEASE");
        mvprintw(7, 0, "        %03i     %03i     %03i     %03i",
                                                (int)((int)opz.getActiveTrackParameters().attack / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().decay / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().sustain / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().release / 2.55f) );

        mvprintw(9, 0, "LFO     DEPTH   RATE    DEST    SHAPE");
        mvprintw(10, 0, "        %s %s %s %s",  hBar(7, (size_t)opz.getActiveTrackParameters().lfo_depth).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().lfo_speed).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().lfo_value).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().lfo_shape).c_str() );
        mvprintw(11, 0, "        %03i     %03i     %03i     %03i", 
                                                (int)((int)opz.getActiveTrackParameters().lfo_depth / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().lfo_speed / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().lfo_value / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().lfo_shape / 2.55f) );

        int middle = 40;
        mvprintw(2, middle, " | FX  1       2");
        mvprintw(3, middle, " |     %s %s", hBar(4, (size_t)opz.getActiveTrackParameters().fx1).c_str(),
                                                hBar(4, (size_t)opz.getActiveTrackParameters().fx2).c_str());
        mvprintw(4, middle, " |     %03i  %03i", 
                                                (int)((int)opz.getActiveTrackParameters().fx1 / 2.55f),
                                                (int)((int)opz.getActiveTrackParameters().fx2 / 2.55f) );

        mvprintw(5, middle, " | ");
        mvprintw(6, middle, " | PAN L       R");
        mvprintw(7, middle, " |     ");
        
        for (size_t i = 0; i < 9; i++) {
            size_t p = opz.getActiveTrackParameters().pan;
            p = (p/254.0)*9;
            if (i + 2 > p  && i < p ) printw("|");
            else printw(".");
        }
        mvprintw(8, middle, " | ");
        mvprintw(9, middle, " | LEVEL");
        mvprintw(10,middle, " |     %s", hBar(9, (size_t)opz.getActiveTrackParameters().level).c_str() );
        mvprintw(11,middle, " |        %03i", (int)( (int)opz.getActiveTrackParameters().level / 2.55f));

        mvprintw(2, x_max-15, " | NOTE LENGTH");
        mvprintw(3, x_max-15, " | %03i", tc.note_length);
        mvprintw(4, x_max-15, " | ");
        mvprintw(5, x_max-15, " | NOTE STYLE");
        mvprintw(6, x_max-15, " | %03i", opz.getActiveTrackParameters().note_style );
        mvprintw(7, x_max-15, " | ");
        mvprintw(8, x_max-15, " | QUANTIZE");
        mvprintw(9, x_max-15, " | %03i", tc.quantize);
        mvprintw(10,x_max-15, " | ");
        mvprintw(11,x_max-15, " | PORTAMENTO");
        mvprintw(12,x_max-15, " | %03i", opz.getActiveTrackParameters().portamento );

        mvprintw(14, 0, "STEP COUNT %03i      STEP LENGHT %03i", tc.step_count, tc.step_length);
        mvprintw(14, x_max-7, "SUM %03i", tc.step_count * tc.step_length);

        refresh();
    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}
