
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
    start_color();

    init_color(COLOR_BLACK, 0, 0, 0);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);

    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    bool track_pressed = false;

    // Listen to key events (no cc, neighter notes)
    opz.setEventCallback( [&](T3::event_id _id, int _value) {
        // refresh();
        if (_id == T3::KEY_TRACK)
            track_pressed = _value;
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

    std::vector<WINDOW*> page = {
        newwin(5, 42, 1, 0),
        newwin(4, 42, 6, 0),
        newwin(5, 42, 10, 0),
        newwin(14, 25, 1, 42),
        newwin(14, 15, 1, 67) 
    };
    refresh();

    while (keepRunnig.load()) {
        getbegyx(stdscr, y_beg, x_beg);
        getmaxyx(stdscr, y_max, x_max);

        opz.keepawake();
        // attron(COLOR_PAIR(1));

        uint8_t pattern = opz.getActivePattern();
        T3::track_id track = opz.getActiveTrack();
        std::string track_name =  T3::OPZ::toString(track);
        T3::project project = opz.getProject();
        T3::track_chunck tc = project.pattern[pattern].track[track];

        mvprintw(0, (x_max-x_beg)/2 - track_name.size()/2, "%s", track_name.c_str() );
    
        for (size_t i = 0; i < page.size(); i++) {
            if (track_pressed)
                wattron(page[4], COLOR_PAIR(2));
            else if (i == (size_t)opz.getActivePage())
                wattron(page[i], COLOR_PAIR(2));
            box(page[i], 0, 0);
            wattroff(page[i], COLOR_PAIR(2));
        }

        // PAGE 1: SOUND
        
        mvwprintw(page[0], 1, 1, "SOUND   P1      P2      FILTER  RESONA.");
        mvwprintw(page[0], 2, 1, "        %s %s %s %s",   hBar(7, (size_t)opz.getActiveTrackParameters().param1).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().param2).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().filter).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().resonance).c_str() );
        mvwprintw(page[0], 3, 1, "        %7i %7i %7i %7i", 
                                                (int)((int)opz.getActiveTrackParameters().param1 / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().param2 / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().filter / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().resonance / 2.55f) );

        // PAGE 2: ENVELOPE
        mvwprintw(page[1], 1, 1, "ENV.    ATTACK  DECAY   SUSTAIN RELEASE");
        mvwprintw(page[1], 2, 1, "        %7i %7i %7i %7i",
                                                (int)((int)opz.getActiveTrackParameters().attack / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().decay / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().sustain / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().release / 2.55f) );

        // PAGE 3: LFO
        mvwprintw(page[2],1, 1, "LFO     DEPTH   RATE    DEST    SHAPE");
        mvwprintw(page[2],2, 1, "        %s %s %s %s",  hBar(7, (size_t)opz.getActiveTrackParameters().lfo_depth).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().lfo_speed).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().lfo_value).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().lfo_shape).c_str() );
        mvwprintw(page[2],3, 1, "        %7i %7i %7i %7i", 
                                                (int)((int)opz.getActiveTrackParameters().lfo_depth / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().lfo_speed / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().lfo_value / 2.55f), 
                                                (int)((int)opz.getActiveTrackParameters().lfo_shape / 2.55f) );

        // PAGE 4: FX / PAN & LEVEL
        mvwprintw(page[3], 1, 1, " FX  1       2");
        mvwprintw(page[3], 2, 1, "     %s %s", hBar(7, (size_t)opz.getActiveTrackParameters().fx1).c_str(),
                                                hBar(7, (size_t)opz.getActiveTrackParameters().fx2).c_str());
        mvwprintw(page[3], 3, 1, "     %7i %7i", 
                                                (int)((int)opz.getActiveTrackParameters().fx1 / 2.55f),
                                                (int)((int)opz.getActiveTrackParameters().fx2 / 2.55f) );

        mvwprintw(page[3], 6, 1, " PAN L             R");
        mvwprintw(page[3], 7, 1, "     ");
        for (size_t i = 0; i < 15; i++) {
            size_t p = opz.getActiveTrackParameters().pan;
            p = (p/254.0)*15;
            if (i + 2 > p  && i < p ) wprintw(page[3], "|");
            else wprintw(page[3],".");
        }
        
        mvwprintw(page[3], 10, 2, " LEVEL");
        mvwprintw(page[3], 11, 2, "    %s", hBar(15, (size_t)opz.getActiveTrackParameters().level).c_str() );
        mvwprintw(page[3], 12, 2, "          %03i", (int)( (int)opz.getActiveTrackParameters().level / 2.55f));

        mvwprintw(page[4], 1,  2, "NOTE LENGTH");
        mvwprintw(page[4], 2,  2, "%i", tc.note_length);
        mvwprintw(page[4], 4,  2, "NOTE STYLE");
        mvwprintw(page[4], 5,  2, "%i", opz.getActiveTrackParameters().note_style );
        mvwprintw(page[4], 8,  2, "QUANTIZE");
        mvwprintw(page[4], 9,  2, "%i", tc.quantize);
        mvwprintw(page[4], 11, 2, "PORTAMENTO");
        mvwprintw(page[4], 12, 2, "%i", opz.getActiveTrackParameters().portamento );

        for (size_t i = 0; i < page.size(); i++)
            wrefresh(page[i]);

        for (size_t i = 0; i < 16; i++) {
            int x = i * 5 + (i / 4) * 2 - ((i > 10)? 1 : 0);
            mvprintw(y_max-4, x, "%i", i+1);
            mvprintw(y_max-3, x, "*");
        }
        mvprintw(y_max-2, 0, "STEP COUNT %2i      STEP LENGHT %2i                                           SUM %2i", 
                                        tc.step_count, tc.step_length, tc.step_count * tc.step_length);

        std::string status = (opz.isPlaying())? ">" : "#";
        mvprintw(y_max-1, (x_max-x_beg)/2 - status.size()/2, "%s", status.c_str() );
        // attroff(COLOR_PAIR(1));
        refresh();
    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}
