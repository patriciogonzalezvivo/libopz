
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>
#include <math.h>

#include <ncurses.h>
#include "opz_rtmidi.h"

#define sign(_x) (_x<0?-1:1)

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
    bool large_screen = false;

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
    getbegyx(stdscr, y_beg, x_beg);
    getmaxyx(stdscr, y_max, x_max);

    large_screen = (x_max >= 159); 

    std::vector<WINDOW*> windows = {
        newwin(5, 41, 1, 0),    //  PAGE ONE
        newwin(8, 41, 6, 0),    //  PAGE TWO
        newwin(5, 41, 14, 0),   //  PAGE THREE
        newwin(18, 25, 1, 41),  //  PAGE FOUR
        newwin(18, 14, 1, 66),  //  STEP / NOTE
        newwin((large_screen ? y_max-2 : 0), (large_screen ? x_max-80 : 0) , 1, (large_screen ? 80 : 0) ), // Extra
    };
    refresh();

    while (keepRunnig.load()) {
        opz.update();

        if (!change)
            continue;

        T3::opz_track_id track_id = opz.getActiveTrackId();
        T3::opz_pattern pattern = opz.getActivePattern();

        std::string title_name = T3::toString(track_id);

        if (mic_on) title_name = "MICROPHONE";
        else if (pressing_project) title_name = "PROJECTS";
        else if (pressing_mixer)   title_name = "MIXER";
        else if (pressing_tempo)   title_name = "TEMPO";

        clear();
        mvprintw(0, (x_max-x_beg)/2 - title_name.size()/2, "%s", title_name.c_str() );

        if (opz.isPlaying()) {
            size_t step = opz.getActiveStepId();
            mvprintw(y_max-4, 2 + step * 4 + ( (step/4) * 4 ) , "[ ]");
        }
            
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

        if (pressing_project)  draw_project(windows[5]);
        else if (pressing_mixer)    draw_mixer(windows[5]);
        else if (pressing_tempo)    draw_tempo(windows[5]);
        else if (mic_on) draw_mic(windows[5]);
        else if (large_screen)  draw_project(windows[5]);

        if ( large_screen || (!mic_on && !pressing_project && !pressing_mixer && !pressing_tempo)){
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

void vBar(WINDOW* _win, int y, int x, size_t _height, size_t _value) {
    std::string rta = "";
    size_t l = (_value/254.0) * _height;
    for (size_t i = 0; i < _height; i++)
        mvwprintw(_win, y - i, x, "%s", (i < l ) ? "#" : ".");
}

/* Plot a point */
void plot(WINDOW* _win, int x, int y, int col) {
    mvwaddch(_win, y, x, (chtype) col);
}

/* Draw a diagonal(arbitrary) line using Bresenham's alogrithm. */
void dline(WINDOW* _win, int from_x, int from_y, int x2, int y2, int ch) {
    int dx, dy;
    int ax, ay;
    int sx, sy;
    int x, y;
    int d;

    dx = x2 - from_x;
    dy = y2 - from_y;

    ax = abs(dx * 2);
    ay = abs(dy * 2);

    sx = sign(dx);
    sy = sign(dy);

    x = from_x;
    y = from_y;

    if (ax > ay) {
        d = ay - (ax / 2);

        while (1) {
            plot(_win, x, y, ch);
            if (x == x2)
                return;

            if (d >= 0) {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    } else {
        d = ax - (ay / 2);

        while (1) {
            plot(_win, x, y, ch);
            if (y == y2)
                return;

            if (d >= 0) {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
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
    int y_beg, x_beg, y_max, x_max;
    getbegyx(_win, y_beg, x_beg);
    getmaxyx(_win, y_max, x_max);

    T3::opz_project_data project = opz.getProjectData();
    double pct = (opz.getActiveStepId() % 8) / 8.0;
    
    wclear(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2,           "TEMPO               SWING");
    mvwprintw(_win, 2, 2,           "%03i                 %03i", project.tempo, (int)((int)project.swing / 2.55f) - 50);

    mvwprintw(_win, 1, x_max - 28, "SOUND                LEVEL");
    mvwprintw(_win, 2, x_max - 31, "%8s                  %03i", T3::metronomeSoundString(project.metronome_sound).c_str(), (int)((int)project.metronome_level / 2.55f) );

    size_t w = 12;
    mvwprintw(_win, 3, x_max/2 - w, "        ####|####");
    mvwprintw(_win, 4, x_max/2 - w, "        ####|####");
    mvwprintw(_win, 5, x_max/2 - w, "       #####|#####");
    mvwprintw(_win, 6, x_max/2 - w, "       #####|#####");
    mvwprintw(_win, 7, x_max/2 - w, "      ######|######");
    mvwprintw(_win, 8, x_max/2 - w, "      ######|######");
    mvwprintw(_win, 9, x_max/2 - w, "     #######|#######");
    mvwprintw(_win,10, x_max/2 - w, "     #######|#######");
    mvwprintw(_win,11, x_max/2 - w, "    ########|########");
    mvwprintw(_win,12, x_max/2 - w, "    ########|########");
    mvwprintw(_win,13, x_max/2 - w, "   #########|#########");
    mvwprintw(_win,14, x_max/2 - w, "   #########|#########");
    mvwprintw(_win,15, x_max/2 - w, "  ##########|##########");
    mvwprintw(_win,16, x_max/2 - w, "  ---------------------");
    mvwprintw(_win,17, x_max/2 - w, " #######################");
    mvwprintw(_win,18, x_max/2 - w, " ######### %03i #########", project.tempo);
    mvwprintw(_win,19, x_max/2 - w, "#########################"); 

    wattron(_win, COLOR_PAIR(2));
    float x = w * sin( pct * 6.2831 );
    float y = 2 * abs( cos( pct * 6.2831 ) );
    dline(_win, x_max/2,    16, 
                x_max/2 + x, 5 - y,  '|');
    wattroff(_win, COLOR_PAIR(2));

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
    size_t track_id = opz.getActiveTrackId();
    size_t page_id = opz.getActivePageId();

    wclear(_win);

    if (page_id == 1)
        wattron(_win, COLOR_PAIR(2));
    box(_win, 0, 0);
    wattroff(_win, COLOR_PAIR(2));

    mvwprintw(_win, 1, 1, "ENVEL.");

    float attack = (int)opz.getActivePageParameters().attack / 255.0f;
    float decay = (int)opz.getActivePageParameters().decay / 255.0f;
    float sustain = (int)opz.getActivePageParameters().sustain / 255.0f;
    float release = (int)opz.getActivePageParameters().release / 255.0f;
    size_t x = 8;
    size_t w = 30;

    if (track_id < 4) {
        // TODO:
        //  - draw envelope
        // ┌───────────────────────────────────────┐
        // │ENVEL.                                 │
        // │                                       │
        // │                                       │
        // │                                       │
        // │                                       │
        // │       S 0     A 0     H 0     D 0     │
        // └───────────────────────────────────────┘
        size_t s = attack * w;
        size_t a = s + ( (w-s)/2 ) * decay;
        size_t h = a + ( (w-a) * sustain);
        size_t d = h + ( (w-h) * release);

        s += x;
        a += x;
        h += x;
        d += x;

        dline(_win, s, 5, a, 1, '.');
        dline(_win, a, 1, h, 1, '.');
        dline(_win, h, 1, d, 5, '.');
        mvwprintw(_win, 5, s, "+");
        mvwprintw(_win, 1, a, "+");
        mvwprintw(_win, 1, h, "+");
        mvwprintw(_win, 5, d, "+");

        mvwprintw(_win, 6,  8, "S %i", (int)(100*attack));
        mvwprintw(_win, 6, 17, "A %i", (int)(100*decay));
        mvwprintw(_win, 6, 25, "H %i", (int)(100*sustain));
        mvwprintw(_win, 6, 34, "D %i", (int)(100*release));
    }
    else {

        size_t a = attack * (w/4);
        size_t d = a + (w/4-a) * decay;
        size_t h = 5 - 4 * sustain;
        size_t r = d + ( (w-d) * release);

        a += x;
        d += x;
        r += x;

        dline(_win, x, 5, a, 1, '.');
        dline(_win, a, 1, d, h, '.');
        dline(_win, d, h, r, h, '.');
        dline(_win, r, h, x+w, 5, '.');

        mvwprintw(_win, 1, a, "+");
        mvwprintw(_win, h, d, "+");
        mvwprintw(_win, h, r, "+");

        mvwprintw(_win, 6,  8, "A %i", (int)(100*attack));
        mvwprintw(_win, 6, 17, "D %i", (int)(100*decay));
        mvwprintw(_win, 6, 25, "H %i", (int)(100*sustain));
        mvwprintw(_win, 6, 34, "R %i", (int)(100*release));
    }

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

    vBar(_win, 15, 13, 7, (size_t)opz.getActivePageParameters().level );
    mvwprintw(_win, 16, 12, "%03i", (int)( (int)opz.getActivePageParameters().level / 2.55f));
}

void draw_track_params(WINDOW* _win) {
    mvwprintw(_win, 1,  2, "NOTE");
    mvwprintw(_win, 3,  2, "    LENGTH");
    mvwprintw(_win, 4,  2, "%10s", T3::noteLengthString( opz.getActiveTrackParameters().note_length ).c_str() );
    mvwprintw(_win, 6,  2, "     STYLE");
    mvwprintw(_win, 7,  2, "%10s", T3::noteStyleString( opz.getActiveTrackId(), opz.getActivePageParameters().note_style ).c_str() );
    mvwprintw(_win, 10, 2, "  QUANTIZE");
    mvwprintw(_win, 11, 2, "%9i%%", (int)((int)opz.getActiveTrackParameters().quantize / 2.55f));
    mvwprintw(_win, 13, 2, "PORTAMENTO");
    mvwprintw(_win, 14, 2, "%9i%%", (int)((int)opz.getActivePageParameters().portamento / 2.55f) );
}