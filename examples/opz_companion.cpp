
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>
#include <math.h>
#include <signal.h>

#include <ncurses.h>
#include "libopz/opz_rtmidi.h"

#define sign(_x) (_x<0?-1:1)

std::string version = "0.1";
std::string name = "opz_companion";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

T3::opz_rtmidi opz;
std::atomic<bool> keepRunnig(true);

std::vector<WINDOW*> windows;
bool large_screen = false;
bool change = true;

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
void handle_winch(int sig);

int main(int argc, char** argv) {
    opz.connect();
    
    initscr();
    start_color();
    use_default_colors();

    init_color(COLOR_MAGENTA, 1000, 100, 100);
    init_color(COLOR_YELLOW, 800, 800, 800);
    init_color(COLOR_GREEN, 600, 600, 600);
    init_color(COLOR_BLUE, 300, 300, 300);

    init_pair(1, COLOR_MAGENTA, -1);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_GREEN, -1);
    init_pair(5, COLOR_BLUE, -1);

    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    large_screen = (COLS >= 159); 

    windows.push_back( newwin(5, 41, 1, 0) );    //  PAGE ONE
    windows.push_back( newwin(8, 41, 6, 0) );    //  PAGE TWO
    windows.push_back( newwin(5, 66, 14, 0) );   //  PAGE THREE
    windows.push_back( newwin(13, 25, 1, 41) );  //  PAGE FOUR

    windows.push_back( newwin(18, 14, 1, 66) );  //  STEP / NOTE
    
    // Extra window that can be display on the side or on top of the pages depending of the size of the terminal
    windows.push_back( newwin((large_screen ? LINES-2 : 0), (large_screen ? COLS-80 : 0) , 1, (large_screen ? 80 : 0) ) );

    signal(SIGWINCH, handle_winch);

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
        mvprintw(0, COLS/2 - title_name.size()/2, "%s", title_name.c_str() );

        size_t step_count = opz.getActiveTrackParameters().step_count;
        size_t step_length = opz.getActiveTrackParameters().step_length;

        if (opz.isPlaying()) {
            size_t step = (opz.getActiveStepId() / step_length) % step_count;
            mvprintw(LINES-4, 2 + step * 4 + ( (step/4) * 4 ) , "[ ]");
        }
        
        for (size_t i = 0; i < step_count; i++) {
            size_t x = 3 + i * 4 + ( (i/4) * 4 );
            mvprintw(LINES-5, x, "%02i", i + 1 );
            size_t note = opz.getNoteIdOffset(track_id, i);
            if ( pattern.note[ note ].note == 0xFF)
                mvprintw(LINES-4, x, "-");
            else {
                attron(COLOR_PAIR(2));
                mvprintw(LINES-4, x, "o");
                attroff(COLOR_PAIR(2));
            }
        }

        mvprintw(LINES-2, 0, "STEP COUNT %2i      STEP LENGTH %2i                                        SUM %2i", 
                                step_count, step_length, step_count * step_length);
        mvprintw(LINES-1, COLS/2 - 3, "%s %02i", ((opz.isPlaying())? "|> " : "[ ]"), opz.getActiveStepId() + 1 );
        refresh();

        if (pressing_project)  draw_project(windows[5]);
        else if (pressing_mixer)    draw_mixer(windows[5]);
        else if (pressing_tempo)    draw_tempo(windows[5]);
        else if (mic_on) draw_mic(windows[5]);
        else if (large_screen)  draw_project(windows[5]);

        if ( large_screen || (!mic_on && !pressing_project && !pressing_mixer && !pressing_tempo)){
            // werase(windows[5]);

            if (pressing_track)
                wattron(windows[4], COLOR_PAIR(2));

            size_t page = (size_t)opz.getActivePageId();

            for (size_t i = 0; i < 5; i++) {
                if (!pressing_track && i == page)
                    wattron(windows[i], COLOR_PAIR(1));
                box(windows[i], 0, 0);
                wattroff(windows[i], COLOR_PAIR(1));
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

void handle_winch(int sig) {
    endwin();

    refresh();
    for (size_t i = 0; i < windows.size(); i++)
        wrefresh(windows[i]);

    wresize( windows[5], LINES-2, COLS-80 );
}

void draw_mic(WINDOW* _win) {
    werase(_win);
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
    T3::opz_track_id track_active = opz.getActiveTrackId();

    int lines, cols;
    getmaxyx(_win, lines, cols);

    werase(_win);
    box(_win, 0, 0);

    mvwprintw(_win, 0, 2, " PROJECT  %02i ", project_id);
    mvwprintw(_win, 0, 20, " PATTERN  %02i ", pattern_id );

    // CHAINED PATTERNS (TODO)
    int song_width = 4;
    int x_margin = (cols - song_width * 16) / 2;
    for (size_t i = 0; i < 16; i++) {
        int y = 2;
        int x = x_margin + i * song_width;
        mvwprintw(_win, y, x, "%02X", opz.getProjectData().pattern_chain[pattern_id].pattern[i]);
        mvwprintw(_win, y+1, x, "%02X", opz.getProjectData().pattern_chain[pattern_id].pattern[i+16]);
    }

    // PATTERN TRACK
    int name_width = 10;
    int step_width = (cols - name_width) / 16;
    x_margin = 2 + (cols - step_width * 16 - name_width) / 2;
    size_t step_current = opz.getActiveStepId();
    
    size_t tracks = 16;
    for (size_t t = 0; t < tracks; t++) {
        int y = 5 + t;
        if (t == track_active) wattron(_win, COLOR_PAIR(2));
        else if (t > 7) wattron(_win, COLOR_PAIR(4));
        mvwprintw(_win, y, x_margin, "%7s", T3::toString( T3::opz_track_id(t) ).c_str() );
        if (t == track_active) wattroff(_win, COLOR_PAIR(2));
        else if (t > 7) wattroff(_win, COLOR_PAIR(4));

        size_t step_count = opz.getTrackParameters(T3::opz_track_id(t) ).step_count;
        size_t step_length = opz.getTrackParameters(T3::opz_track_id(t) ).step_length;
        size_t step = (step_current / step_length) % step_count;
        mvwprintw(_win,y, x_margin + name_width + step * step_width - 1, "[  ]");
        for (size_t s = 0; s < step_count; s++) {
            int x = x_margin + name_width + s * step_width;
            size_t i = opz.getNoteIdOffset(t, s);

            if ( pattern.note[ i ].note == 0xFF) {
                if (t > 7) wattron(_win, COLOR_PAIR(4));
                mvwprintw(_win, y, x, "--");
                if (t > 7) wattroff(_win, COLOR_PAIR(4));
            }
            else {
                if (t == track_active) wattron(_win, COLOR_PAIR(1));
                else if (t > 7) wattron(_win, COLOR_PAIR(2));
                mvwprintw(_win, y, x, "%02X", pattern.note[ i ].note );
                if (t == track_active) wattroff(_win, COLOR_PAIR(1));
                else if (t > 7) wattroff(_win, COLOR_PAIR(2));
            }
        }
    }
    wrefresh(_win);
}

void draw_mixer(WINDOW* _win) {
    T3::opz_project_data project = opz.getProjectData();

    werase(_win);
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
    int lines, cols;
    getmaxyx(_win, lines, cols);

    T3::opz_project_data project = opz.getProjectData();
    double pct = (opz.getActiveStepId() % 8) / 8.0;
    
    werase(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2,           "TEMPO               SWING");
    mvwprintw(_win, 2, 2,           "%03i                 %03i", project.tempo, (int)((int)project.swing / 2.55f) - 50);

    mvwprintw(_win, 1, cols - 28, "SOUND                LEVEL");
    mvwprintw(_win, 2, cols - 31, "%8s                  %03i", T3::metronomeSoundString(project.metronome_sound).c_str(), (int)((int)project.metronome_level / 2.55f) );

    size_t w = 12;
    mvwprintw(_win, 3, cols/2 - w, "        ####|####");
    mvwprintw(_win, 4, cols/2 - w, "        ####|####");
    mvwprintw(_win, 5, cols/2 - w, "       #####|#####");
    mvwprintw(_win, 6, cols/2 - w, "       #####|#####");
    mvwprintw(_win, 7, cols/2 - w, "      ######|######");
    mvwprintw(_win, 8, cols/2 - w, "      ######|######");
    mvwprintw(_win, 9, cols/2 - w, "     #######|#######");
    mvwprintw(_win,10, cols/2 - w, "     #######|#######");
    mvwprintw(_win,11, cols/2 - w, "    ########|########");
    mvwprintw(_win,12, cols/2 - w, "    ########|########");
    mvwprintw(_win,13, cols/2 - w, "   #########|#########");
    mvwprintw(_win,14, cols/2 - w, "   #########|#########");
    mvwprintw(_win,15, cols/2 - w, "  ##########|##########");
    mvwprintw(_win,16, cols/2 - w, "  ---------------------");
    mvwprintw(_win,17, cols/2 - w, " #######################");
    mvwprintw(_win,18, cols/2 - w, " ######### %03i #########", project.tempo);
    mvwprintw(_win,19, cols/2 - w, "#########################"); 

    wattron(_win, COLOR_PAIR(1));
    float x = w * sin( pct * 6.2831 );
    float y = 2 * abs( cos( pct * 6.2831 ) );
    dline(_win, cols/2,    16, 
                cols/2 + x, 5 - y,  '|');
    wattroff(_win, COLOR_PAIR(1));

    wrefresh(_win);
}

// PAGE 1: SOUND     
void draw_page_one(WINDOW* _win) {
    mvwprintw(_win, 1, 1, "SOUND  P1      P2      FILTER  RESONA.");
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win, 2, 1, "       %s %s %s %s",   hBar(7, (size_t)opz.getActivePageParameters().param1).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().param2).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().filter).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().resonance).c_str() );
    wattroff(_win, COLOR_PAIR(4));
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

    werase(_win);

    if (page_id == 1) wattron(_win, COLOR_PAIR(1));
    box(_win, 0, 0);
    wattroff(_win, COLOR_PAIR(1));

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

        wattron(_win, COLOR_PAIR(4));
        dline(_win, s, 5, a, 1, '.');
        dline(_win, a, 1, h, 1, '.');
        dline(_win, h, 1, d, 5, '.');
        wattroff(_win, COLOR_PAIR(4));

        wattron(_win, COLOR_PAIR(2));
        mvwprintw(_win, 5, s, "+");
        mvwprintw(_win, 1, a, "+");
        mvwprintw(_win, 1, h, "+");
        mvwprintw(_win, 5, d, "+");
        wattroff(_win, COLOR_PAIR(2));

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

        wattron(_win, COLOR_PAIR(4));
        dline(_win, x, 5, a, 1, '.');
        dline(_win, a, 1, d, h, '.');
        dline(_win, d, h, r, h, '.');
        dline(_win, r, h, x+w, 5, '.');
        wattroff(_win, COLOR_PAIR(4));

        wattron(_win, COLOR_PAIR(2));
        mvwprintw(_win, 1, a, "+");
        mvwprintw(_win, h, d, "+");
        mvwprintw(_win, h, r, "+");
        wattroff(_win, COLOR_PAIR(2));

        mvwprintw(_win, 6,  8, "A %i", (int)(100*attack));
        mvwprintw(_win, 6, 17, "D %i", (int)(100*decay));
        mvwprintw(_win, 6, 25, "H %i", (int)(100*sustain));
        mvwprintw(_win, 6, 34, "R %i", (int)(100*release));
    }

}

// PAGE 3: LFO
void draw_page_three(WINDOW* _win) {
    mvwprintw(_win,1, 1, "LFO    DEPTH   RATE    DEST    SHAPE");
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win,2, 1, "       %s %s         %s",   hBar(7, (size_t)opz.getActivePageParameters().lfo_depth).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().lfo_speed).c_str(),
                                            T3::lfoShapeShapeString( opz.getActivePageParameters().lfo_shape ).c_str());
    wattroff(_win, COLOR_PAIR(4));
    mvwprintw(_win,3, 1, "       %3i     %3i     %3s    %5s", 
                                            (int)((int)opz.getActivePageParameters().lfo_depth / 2.55f), 
                                            (int)((int)opz.getActivePageParameters().lfo_speed / 2.55f),
                                            T3::lfoDestinationShortString( opz.getActivePageParameters().lfo_value ).c_str(),
                                            T3::lfoShapeShortString( opz.getActivePageParameters().lfo_shape ).c_str() );
}

void draw_page_four(WINDOW* _win) {
    // PAGE 4: FX / PAN & LEVEL
    mvwprintw(_win, 1, 1, " FX  1       2");
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win, 2, 1, "     %s %s", hBar(7, (size_t)opz.getActivePageParameters().fx1).c_str(),
                                            hBar(7, (size_t)opz.getActivePageParameters().fx2).c_str());
    wattroff(_win, COLOR_PAIR(4));
    mvwprintw(_win, 3, 1, "     %7i %7i", 
                                            (int)((int)opz.getActivePageParameters().fx1 / 2.55f),
                                            (int)((int)opz.getActivePageParameters().fx2 / 2.55f) );

    mvwprintw(_win, 5, 1, " PAN L             R");
    mvwprintw(_win, 6, 1, "     ");
    for (size_t i = 0; i < 15; i++) {
        size_t p = opz.getActivePageParameters().pan;
        p = (p/254.0)*15;
        if (i + 2 > p  && i < p ) {
            wattron(_win, COLOR_PAIR(2));
            wprintw(_win, "|");
            wattroff(_win, COLOR_PAIR(2));
        }
        else {
            wattron(_win, COLOR_PAIR(4));
            wprintw(_win,".");
            wattroff(_win, COLOR_PAIR(4));
        }
    }

    mvwprintw(_win, 9, 1, " LEVEL     %03i", (int)( (int)opz.getActivePageParameters().level / 2.55f));
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win, 10, 1, "     %s", hBar(15, (size_t)opz.getActivePageParameters().level).c_str());
    wattroff(_win, COLOR_PAIR(4));
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
