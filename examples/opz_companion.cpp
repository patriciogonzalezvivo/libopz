
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>

#include <ncurses.h>
#include "opz_rtmidi.h"

std::string version = "0.1";
std::string name = "opz_companion";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

T3::opz_rtmidi opz;
std::atomic<bool> keepRunnig(true);

// global
void draw_mic(WINDOW* _window);
void draw_project(WINDOW* _window);

// pattern
void draw_mixer(WINDOW* _window);
void draw_tempo(WINDOW* _window);

// pattern / track parameters
void draw_track_params(WINDOW* _window);

// pattern / track / sound parameters
void draw_page_one(WINDOW* _window);
void draw_page_two(WINDOW* _window);
void draw_page_three(WINDOW* _window);
void draw_page_four(WINDOW* _window);

int main(int argc, char** argv) {
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
    bool change_data = true;
    bool pressing_track = false;
    bool pressing_project = false;
    bool pressing_mixer = false;
    bool pressing_tempo = false;
    bool mic_on = false;

    // Listen to key events (no cc, neighter notes)
    opz.setEventCallback( [&](T3::opz_event_id _id, int _value) {
        change = true;

        if (_id == T3::KEY_TRACK)           pressing_track = _value;
        else if (_id == T3::KEY_PROJECT)    pressing_project = _value;
        else if (_id == T3::KEY_MIXER)      pressing_mixer = _value;
        else if (_id == T3::KEY_TEMPO)      pressing_tempo = _value;
        else if (_id == T3::MICROPHONE_MODE_CHANGE) mic_on = _value != 0;
        else if (_id == T3::PATTERN_DOWNLOADED || _id == T3::PATTERN_CHANGE || _id == T3::TRACK_CHANGE || _id == T3::SEQUENCE_CHANGE || _id == T3::PAGE_CHANGE || _id == T3::PARAMETER_CHANGE ) change_data = true;
    } );

    std::thread waitForKeys([&](){
        char ch;
        while ( true ) {
            ch = getch();
            if (ch == 'x') {
                keepRunnig = false;
                keepRunnig.store(false);
                break;
            }
        }
    });

    int y_beg, x_beg, y_max, x_max;

    std::vector<WINDOW*> windows = {
        newwin(5, 41, 1, 0),    //  PAGE ONE
        newwin(8, 41, 6, 0),    //  PAGE TWO
        newwin(5, 41, 14, 0),   //  PAGE THREE
        newwin(18, 25, 1, 41),  //  PAGE FOUR
        newwin(18, 15, 1, 66),  //  STEP / NOTE
        newwin(18, 80, 1, 0),   //  PROJECT
    };
    refresh();

    while (keepRunnig.load()) {
        getbegyx(stdscr, y_beg, x_beg);
        getmaxyx(stdscr, y_max, x_max);

        opz.keepawake();

        if (!change)
            continue;

        // size_t project_id = opz.getActiveProjectId();
        // uint8_t pattern_id = opz.getActivePatternId();
        T3::opz_track_id track_id = opz.getActiveTrackId();
        // T3::opz_project_data project = opz.getProjectData();
        T3::opz_pattern pattern = opz.getActivePattern();

        std::string title_name = T3::toString(track_id);

        if (mic_on) title_name = "MICROPHONE";
        else if (pressing_project) title_name = "PROJECTS";
        else if (pressing_mixer)   title_name = "MIXER";
        else if (pressing_tempo)   title_name = "TEMPO";

        clear();
        mvprintw(0, (x_max-x_beg)/2 - title_name.size()/2, "%s", title_name.c_str() );

        for (size_t i = 0; i < 16; i++) {
            size_t x = 3 + i * 4 + ( (i/4) * 4 );
            mvprintw(y_max-5, x, "%02i", i + 1 );
            size_t note = opz.getNoteIdOffset(track_id, i);

            if ( pattern.note[ note ].note == 0xFF)
                mvprintw(y_max-4, x, "-");
            else {
                attron(COLOR_PAIR(2));
                mvprintw(y_max-4, x, "o");
                attroff(COLOR_PAIR(2));
            }
        }
        
        mvprintw(y_max-2, 0, "STEP COUNT %2i      STEP LENGHT %2i                                        SUM %2i", 
                                opz.getActiveTrackParameters().step_count, 
                                opz.getActiveTrackParameters().step_length,
                                opz.getActiveTrackParameters().step_count * opz.getActiveTrackParameters().step_length);
        mvprintw(y_max-1, (x_max-x_beg)/2 - 3, "%s %02i", ((opz.isPlaying())? "|> " : "[ ]"), opz.getActiveStepId() + 1 );
        refresh();

        if (mic_on) draw_mic(windows[5]);
        else if (pressing_project)  draw_project(windows[5]);
        else if (pressing_mixer)    draw_mixer(windows[5]);
        else if (pressing_tempo)    draw_tempo(windows[5]);
        else {
            // wclear(windows[5]);

            if (pressing_track)
                wattron(windows[4], COLOR_PAIR(2));

            size_t page = (size_t)opz.getActivePageId();

            for (size_t i = 0; i < 5; i++) {
                if (!pressing_track && i == page)
                    wattron(windows[i], COLOR_PAIR(2));
                box(windows[i], 0, 0);
                wattroff(windows[i], COLOR_PAIR(2));
            }

            if (page == 0 || change_data) draw_page_one(windows[0]);
            if (page == 1 || change_data) draw_page_two(windows[1]);
            if (page == 2 || change_data) draw_page_three(windows[2]);
            if (page == 3 || change_data) draw_page_four(windows[3]);

            if (pressing_track || change_data) draw_track_params(windows[4]);

            for (size_t i = 0; i < 5; i++)
                wrefresh(windows[i]);

            change = false;
            change_data = false;
        }

    }
    
    waitForKeys.join();
    endwin();
    opz.disconnect();

    exit(0);
}

std::string hBar(size_t _width, size_t _value) {
    std::string rta = "";
    size_t l = (_value/254.0) * _width;
    for (size_t i = 0; i < _width; i++)
        rta += (i < l ) ? "#" : ".";
    return rta;
}

void draw_mic(WINDOW* _win) {
    wclear(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2, "MIC LEVEL                  MIC FX ");
    mvwprintw(_win, 2, 2, "%s                  %s", 
                            hBar(9, opz.getMicLevel() ).c_str(), 
                            T3::toString(opz.getMicFx()).c_str());
    wrefresh(_win);
}

void draw_project(WINDOW* _win) {
    size_t project_id = opz.getActiveProjectId();
    uint8_t pattern_id = opz.getActivePatternId();
    T3::opz_pattern pattern = opz.getActivePattern();
    T3::opz_track_id track_id = opz.getActiveTrackId();

    wclear(_win);
    box(_win, 0, 0);

    mvwprintw(_win, 0, 2, " PROJECT  %02i ", project_id);
    mvwprintw(_win, 0, 20, " PATTERN  %02i ", pattern_id );
    
    for (size_t y = 0; y < 16; y++) {
        if (y == track_id)
            wattron(_win, COLOR_PAIR(2));

        mvwprintw(_win, y+1, 2, "%7s", T3::toString( T3::opz_track_id(y) ).c_str() );

        for (size_t x = 0; x < 16; x++) {
            size_t i = opz.getNoteIdOffset(y, x);

            if ( pattern.note[ i ].note == 0xFF)
                mvwprintw(_win, y+1, 12 + x * 4, " .");
            else
                mvwprintw(_win, y+1, 12 + x * 4, "%02X", pattern.note[ i ].note );
        }
        wattroff(_win, COLOR_PAIR(2));
    }
    wrefresh(_win);
}

void draw_mixer(WINDOW* _win) {
    T3::opz_project_data project = opz.getProjectData();

    wclear(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2, "DRUMS               SYNTH                PUNCH                MASTER");
    mvwprintw(_win, 2, 2, "%03i                 %03i                  %03i                  %03i", 
                            (int)((int)project.drum_level / 2.55f), 
                            (int)((int)project.synth_level / 2.55f), 
                            (int)((int)project.punch_level / 2.55f), 
                            (int)((int)project.master_level / 2.55f) );
    wrefresh(_win);
}

void draw_tempo(WINDOW* _win) {
    T3::opz_project_data project = opz.getProjectData();

    wclear(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2, "TEMPO               SWING                SOUND                LEVEL");
    mvwprintw(_win, 2, 2, "%03i                 %03i                  %03i                  %03i", 
                            project.tempo, 
                            (int)((int)project.swing / 2.55f) - 50, 
                            (int)((int)project.metronome_sound / 2.55f) , 
                            (int)((int)project.metronome_level / 2.55f) );
    wrefresh(_win);
}

// PAGE 1: SOUND     
void draw_page_one(WINDOW* _win) {
    mvwprintw(_win, 1, 1, "SOUND  P1      P2      FILTER  RESONA.");
    mvwprintw(_win, 2, 1, "       %s %s %s %s",   hBar(7, (size_t)opz.getActivePageParameters().param1).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().param2).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().filter).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().resonance).c_str() );
    mvwprintw(_win, 3, 1, "       %7i %7i %7i %7i", 
                                            (int)((int)opz.getActivePageParameters().param1 / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().param2 / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().filter / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().resonance / 2.55f) );
}

// PAGE 2: ENVELOPE
void draw_page_two(WINDOW* _win) {
    mvwprintw(_win, 1, 1, "ENVEL.");

    // TODO:
    //  - draw envelope

    mvwprintw(_win, 6, 8, "S %i ", (int)((int)opz.getActivePageParameters().attack / 2.55f));
    mvwprintw(_win, 6,16, "A %i", (int)((int)opz.getActivePageParameters().decay / 2.55f));
    mvwprintw(_win, 6,24, "H %i", (int)((int)opz.getActivePageParameters().sustain / 2.55f));
    mvwprintw(_win, 6,32, "D %i", (int)((int)opz.getActivePageParameters().release / 2.55f));
}

// PAGE 3: LFO
void draw_page_three(WINDOW* _win) {
    mvwprintw(_win,1, 1, "LFO    DEPTH   RATE    DEST    SHAPE");
    mvwprintw(_win,2, 1, "       %s %s %s %s",  hBar(7, (size_t)opz.getActivePageParameters().lfo_depth).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().lfo_speed).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().lfo_value).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().lfo_shape).c_str() );
    mvwprintw(_win,3, 1, "       %7i %7i %7i %7i", 
                                            (int)((int)opz.getActivePageParameters().lfo_depth / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().lfo_speed / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().lfo_value / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().lfo_shape / 2.55f) );
}

void draw_page_four(WINDOW* _win) {
    // PAGE 4: FX / PAN & LEVEL
    mvwprintw(_win, 1, 1, " FX  1       2");
    mvwprintw(_win, 2, 1, "     %s %s", hBar(7, (size_t)opz.getActivePageParameters().fx1).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().fx2).c_str());
    mvwprintw(_win, 3, 1, "     %7i %7i", 
                                            (int)((int)opz.getActivePageParameters().fx1 / 2.55f),
                                            (int)((int)opz.getActivePageParameters().fx2 / 2.55f) );

    mvwprintw(_win, 6, 1, " PAN L             R");
    mvwprintw(_win, 7, 1, "     ");
    for (size_t i = 0; i < 15; i++) {
        size_t p = opz.getActivePageParameters().pan;
        p = (p/254.0)*15;
        if (i + 2 > p  && i < p ) wprintw(_win, "|");
        else wprintw(_win,".");
    }
    
    mvwprintw(_win, 10, 1, " LEVEL");
    mvwprintw(_win, 11, 1, "     %s", hBar(15, (size_t)opz.getActivePageParameters().level).c_str() );
    mvwprintw(_win, 12, 1, "           %03i", (int)( (int)opz.getActivePageParameters().level / 2.55f));
}

void draw_track_params(WINDOW* _win) {
    mvwprintw(_win, 1,  2, "NOTE LENGTH");
    mvwprintw(_win, 2,  2, "%i", opz.getActiveTrackParameters().note_length);
    mvwprintw(_win, 4,  2, "NOTE STYLE");
    mvwprintw(_win, 5,  2, "%i", opz.getActivePageParameters().note_style );
    mvwprintw(_win, 8,  2, "QUANTIZE");
    mvwprintw(_win, 9,  2, "%i", opz.getActiveTrackParameters().quantize);
    mvwprintw(_win, 11, 2, "PORTAMENTO");
    mvwprintw(_win, 12, 2, "%i", opz.getActivePageParameters().portamento );
}