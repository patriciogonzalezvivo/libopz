
#include <iostream>
#include <fstream>

#include <thread>
#include <atomic>
#include <mutex>
#include <math.h>
#include <signal.h>
#include <algorithm>
#include <cstring>

#include <ncurses.h>
#include "libopz/opz_rtmidi.h"

#define sign(_x) (_x<0?-1:1)

std::string version = "0.1";
std::string name = "opz_companion";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

opz::opz_rtmidi device;
std::atomic<bool> keepRunnig(true);

std::vector<WINDOW*> windows;

// The project/pattern-grid panel (windows[5]) can sit beside the track
// property windows (SIDE), stacked below them (STACKED), or - if the
// terminal isn't big enough for either - be hidden entirely and only
// shown as a temporary full-screen overlay while pressing project/mixer/
// tempo/mic (COMPACT).
enum project_layout_t { LAYOUT_SIDE, LAYOUT_STACKED, LAYOUT_COMPACT };
project_layout_t project_layout = LAYOUT_COMPACT;
bool show_project_panel = false; // true when the panel is always visible (SIDE or STACKED)

// Track property windows (0-4) occupy x:0-80, y:1-19 (fixed, regardless of layout).
const int TRACK_PROPS_WIDTH  = 80;   // windows[2] (66 wide) + windows[4] (14 wide)
const int TRACK_PROPS_HEIGHT = 19;   // windows[2]/[4] bottom out at y=19
const int PROJECT_MIN_WIDTH  = 60;   // narrowest the project grid is still legible
const int PROJECT_FULL_HEIGHT = 23;  // draw_project()'s content never exceeds this
const int BOTTOM_BAR_MARGIN  = 6;    // step display rows + a blank line of breathing room

bool change = true;

// --- PROJECT view cursor (arrow keys, only while pressing_project) ---
// Up/Down moves the highlighted track (row) and pushes a real track-select to
// the device; Left/Right moves the highlighted step (column), local-only, to
// pick a step to inspect/edit.
std::atomic<int> proj_cursor_track(0);
std::atomic<int> proj_cursor_step(0);

// --- MIXER view cursor (arrow keys, only while pressing_mixer) ---
// Left/Right picks which track's channel strip is active; Up/Down nudges
// that track's mixer level; Enter toggles its mute - all pushed live via 0x12.
std::atomic<int> mixer_cursor_track(0);

// --- Track editing / write-back state ---
// Local editable copy of the 16-pattern bank. Edits are staged here and only
// pushed to the device (as a full 0x09/0x0a bank write) when the user asks to
// send them, since inbound 0x02 edits are not applied by the device.
std::mutex edit_mtx;
std::atomic<bool> edit_mode(false);
opz::opz_pattern edit_bank[16];
size_t edit_step_cursor = 0;
std::string edit_status = "";

void sync_edit_bank_from_device() {
    std::lock_guard<std::mutex> lock(edit_mtx);
    memcpy(edit_bank, &device.getProjectData().pattern[0], sizeof(edit_bank));
}

opz::opz_pattern get_display_pattern() {
    std::lock_guard<std::mutex> lock(edit_mtx);
    return edit_mode.load() ? edit_bank[device.getActivePatternId()] : device.getActivePattern();
}

// Places (or clears, if already set) a note at the cursor step of the active
// track/pattern in the local edit_bank.
void toggle_note_at_cursor() {
    std::lock_guard<std::mutex> lock(edit_mtx);

    size_t pattern_id = device.getActivePatternId();
    opz::opz_track_id track = device.getActiveTrackId();
    opz::opz_pattern& pat = edit_bank[pattern_id];

    size_t step_count = pat.track_param[track].step_count;
    if (step_count == 0)
        return;

    size_t step = edit_step_cursor % step_count;
    size_t note_offset = device.getNoteIdOffset(track, step);
    size_t notes_total = device.getNotesPerTrack(track);

    bool is_set = pat.note[note_offset].note != 0xFF;
    for (size_t i = 0; i < notes_total; i++) {
        if (is_set)
            pat.note[note_offset + i].note = 0xFF;
        else {
            pat.note[note_offset + i].duration = 6200;         // ~1 step
            pat.note[note_offset + i].note = 0x3C;             // middle C
            pat.note[note_offset + i].velocity = 0x64;
            pat.note[note_offset + i].micro_adjustment = 0;
            pat.note[note_offset + i].age = 0;
        }
    }
}

// Refreshes the device's own bank first (so any live edits made on the OP-Z
// itself aren't clobbered), then pushes the local edit_bank as a full write.
bool send_edit_bank_to_device() {
    device.requestPatternSync(1.0);

    uint8_t  address = device.getPatternAddress();
    uint16_t id      = device.getPatternId();

    std::lock_guard<std::mutex> lock(edit_mtx);
    int acked = device.sendPattern(edit_bank, address, id);
    device.loadPatternBank(edit_bank);
    return acked > 0;
}

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

project_layout_t compute_project_layout() {
    if (COLS >= TRACK_PROPS_WIDTH + PROJECT_MIN_WIDTH)
        return LAYOUT_SIDE;
    if (LINES >= TRACK_PROPS_HEIGHT + PROJECT_FULL_HEIGHT + BOTTOM_BAR_MARGIN)
        return LAYOUT_STACKED;
    return LAYOUT_COMPACT;
}

// (Re)sizes and repositions the project panel (windows[5]) to match the
// current terminal dimensions. Safe to call at startup and from the
// SIGWINCH handler.
void layout_project_window() {
    project_layout = compute_project_layout();
    show_project_panel = (project_layout != LAYOUT_COMPACT);

    int h, w, y, x;
    if (project_layout == LAYOUT_SIDE) {
        w = COLS - TRACK_PROPS_WIDTH;
        h = std::min(LINES - 2, PROJECT_FULL_HEIGHT);
        y = 1; x = TRACK_PROPS_WIDTH;
    }
    else if (project_layout == LAYOUT_STACKED) {
        w = COLS;
        h = std::min(LINES - TRACK_PROPS_HEIGHT - BOTTOM_BAR_MARGIN, PROJECT_FULL_HEIGHT);
        y = TRACK_PROPS_HEIGHT + 2; x = 0;
    }
    else {
        w = COLS;
        h = std::min(LINES - 2, PROJECT_FULL_HEIGHT);
        y = 1; x = 0;
    }

    h = std::max(h, 1);
    w = std::max(w, 1);

    wresize(windows[5], h, w);
    mvwin(windows[5], y, x);
}

int main(int argc, char** argv) {
    device.connect();

    // Pull the pattern bank that's already in the OP-Z's memory before drawing
    // anything, otherwise every track looks empty until the device happens to
    // emit a runtime delta (e.g. the user edits a step) for it.
    device.requestPatternSync();

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

    windows.push_back( newwin(5, 41, 1, 0) );    //  PAGE ONE
    windows.push_back( newwin(8, 41, 6, 0) );    //  PAGE TWO
    windows.push_back( newwin(5, 66, 14, 0) );   //  PAGE THREE
    windows.push_back( newwin(13, 25, 1, 41) );  //  PAGE FOUR

    windows.push_back( newwin(18, 14, 1, 66) );  //  STEP / NOTE

    // Project/pattern-grid panel: sized and positioned by layout_project_window()
    // below, which picks side-by-side, stacked, or hidden-until-pressed based on
    // the current terminal size.
    windows.push_back( newwin(1, 1, 1, 0) );     //  PROJECT (placeholder, resized below)
    layout_project_window();

    signal(SIGWINCH, handle_winch);

    bool change_data = true;
    bool pressing_track = false;
    bool pressing_project = false;
    bool pressing_mixer = false;
    bool pressing_tempo = false;
    bool mic_on = false;
    
    // Listen to key events (no cc, neighter notes)
    device.setEventCallback( [&](opz::opz_event_id _id, int _value) {
        change = true;

        if (_id == opz::KEY_TRACK)           pressing_track = _value;
        else if (_id == opz::KEY_PROJECT)    pressing_project = _value;
        else if (_id == opz::KEY_MIXER)      pressing_mixer = _value;
        else if (_id == opz::KEY_TEMPO)      pressing_tempo = _value;
        else if (_id == opz::MICROPHONE_MODE_CHANGE) mic_on = _value != 0;
        else if (_id == opz::PATTERN_DOWNLOADED || _id == opz::PATTERN_CHANGE || _id == opz::TRACK_CHANGE || _id == opz::SEQUENCE_CHANGE || _id == opz::PAGE_CHANGE || _id == opz::TRACK_PARAMETER_CHANGE || _id == opz::MUTE_CHANGE ) change_data = true;
    } );

    std::thread waitForKeys([&](){
        int ch;
        while ( true ) {
            ch = getch();

            if (ch == 'x') {
                keepRunnig.store(false);
                break;
            }
            else if (ch == 'e') {
                bool entering = !edit_mode.load();
                if (entering) {
                    sync_edit_bank_from_device();
                    edit_step_cursor = 0;
                    edit_status = "EDIT: h/l move cursor, space toggle note, s send to device, c cancel, e exit";
                }
                else {
                    edit_status = "";
                }
                edit_mode.store(entering);
                change = true;
            }
            else if (ch == KEY_UP) {
                if (pressing_project) {
                    int t = proj_cursor_track.load() - 1;
                    if (t < 0) t = 15;
                    proj_cursor_track.store(t);
                    device.sendTrackSelect(opz::opz_track_id(t));
                    change = true;
                    change_data = true;
                }
                else if (pressing_mixer) {
                    int t = mixer_cursor_track.load();
                    int lvl = device.hasMixerState() ? (int)device.getMixerState().level[t] : 0;
                    device.sendMixerTrackLevel(opz::opz_track_id(t), (uint8_t)std::min(255, lvl + 8));
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_DOWN) {
                if (pressing_project) {
                    int t = (proj_cursor_track.load() + 1) % 16;
                    proj_cursor_track.store(t);
                    device.sendTrackSelect(opz::opz_track_id(t));
                    change = true;
                    change_data = true;
                }
                else if (pressing_mixer) {
                    int t = mixer_cursor_track.load();
                    int lvl = device.hasMixerState() ? (int)device.getMixerState().level[t] : 0;
                    device.sendMixerTrackLevel(opz::opz_track_id(t), (uint8_t)std::max(0, lvl - 8));
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_LEFT) {
                if (pressing_project) {
                    int s = proj_cursor_step.load() - 1;
                    if (s < 0) s = 15;
                    proj_cursor_step.store(s);
                    change = true;
                    change_data = true;
                }
                else if (pressing_mixer) {
                    int t = (mixer_cursor_track.load() + 15) % 16;
                    mixer_cursor_track.store(t);
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_RIGHT) {
                if (pressing_project) {
                    int s = (proj_cursor_step.load() + 1) % 16;
                    proj_cursor_step.store(s);
                    change = true;
                    change_data = true;
                }
                else if (pressing_mixer) {
                    int t = (mixer_cursor_track.load() + 1) % 16;
                    mixer_cursor_track.store(t);
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_ENTER || ch == '\n' || ch == '\r') {
                if (pressing_mixer) {
                    device.sendMixerToggleMute(opz::opz_track_id(mixer_cursor_track.load()));
                    change = true;
                    change_data = true;
                }
            }
            else if (ch >= '1' && ch <= '9') {
                device.sendProjectSelect(ch - '1');
                change = true;
                change_data = true;
            }
            else if (ch == '0') {
                device.sendProjectSelect(9);
                change = true;
                change_data = true;
            }
            else if (ch == ' ') {
                unsigned char msg = device.isPlaying() ? opz::STOP_SONG : opz::START_SONG;
                device.send(&msg, 1);
                change = true;
            }
            else if (edit_mode.load()) {
                if (ch == 'h') {
                    if (edit_step_cursor > 0) edit_step_cursor--;
                    change = true;
                }
                else if (ch == 'l') {
                    edit_step_cursor++;
                    change = true;
                }
                else if (ch == 'c') {
                    sync_edit_bank_from_device();
                    edit_status = "edits discarded, re-synced from device";
                    change = true;
                }
                else if (ch == 's') {
                    edit_status = "sending pattern bank to device...";
                    change = true;
                    bool ok = send_edit_bank_to_device();
                    edit_status = ok ? "sent to device" : "send failed (no ack from device)";
                    edit_mode.store(false);
                    change = true;
                }
            }
        }
    });

    refresh();

    while (keepRunnig.load()) {
        device.update();

        if (!change)
            continue;

        opz::opz_track_id track_id = device.getActiveTrackId();
        opz::opz_pattern pattern = get_display_pattern();

        std::string title_name = opz::toString(track_id);

        if (mic_on) title_name = "MICROPHONE";
        else if (pressing_project) title_name = "PROJECTS";
        else if (pressing_mixer)   title_name = "MIXER";
        else if (pressing_tempo)   title_name = "TEMPO";
        else if (edit_mode.load()) title_name = "EDIT " + title_name;

        clear();
        mvprintw(0, COLS/2 - title_name.size()/2, "%s", title_name.c_str() );

        size_t step_count = device.getActiveTrackParameters().step_count;
        size_t step_length = device.getActiveTrackParameters().step_length;

        // Fit all 16 steps evenly within the actual terminal width, instead of a
        // fixed spacing that clips the later steps on narrower terminals.
        int cell_width = std::max(2, (COLS - 6) / 16);

        if (device.isPlaying() && step_count > 0 && step_length > 0) {
            size_t step = (device.getActiveStepId() / step_length) % step_count;
            mvprintw(LINES-4, 2 + step * cell_width, "[ ]");
        }

        // Always render all 16 physical step slots, regardless of the track's
        // configured step_count - note[] has a slot for every one of the 16
        // steps no matter how many of them the track is currently sequencing,
        // so a smaller step_count would otherwise hide real note data.
        for (size_t i = 0; i < 16; i++) {
            size_t x = 3 + i * cell_width;
            mvprintw(LINES-5, x, "%02i", i + 1 );
            size_t note = device.getNoteIdOffset(track_id, i);

            bool cursor_here = edit_mode.load() && (i == edit_step_cursor % std::max((size_t)1, step_count));
            if (cursor_here) attron(COLOR_PAIR(1));

            size_t step_idx = i * 16 + (size_t)track_id;
            bool has_components = (step_idx < 256) && (pattern.step[step_idx].components_bitmask != 0);

            if ( pattern.note[ note ].note == 0xFF) {
                if (has_components) {
                    attron(COLOR_PAIR(5));
                    mvprintw(LINES-4, x, "~");
                    attroff(COLOR_PAIR(5));
                }
                else
                    mvprintw(LINES-4, x, "-");
            }
            else {
                if (has_components) {
                    attron(COLOR_PAIR(1));
                    mvprintw(LINES-4, x, "*");
                    attroff(COLOR_PAIR(1));
                }
                else {
                    attron(COLOR_PAIR(2));
                    mvprintw(LINES-4, x, "0");
                    attroff(COLOR_PAIR(2));
                }
            }

            if (cursor_here) attroff(COLOR_PAIR(1));
        }

        mvprintw(LINES-3, 0, "%s", edit_status.c_str());
        mvprintw(LINES-2, 0, "STEP COUNT %2i   STEP LENGTH %2i   SUM %2i   TEMPO %3i BPM",
                                step_count, step_length, step_count * step_length,
                                device.getProjectData().tempo);
        attron(COLOR_PAIR(device.isPlaying() ? 2 : 5));
        mvprintw(LINES-1, COLS/2 - 3, "%s %02zu", ((device.isPlaying())? "|> " : "[ ]"), device.getActiveStepId() + 1 );
        attroff(COLOR_PAIR(device.isPlaying() ? 2 : 5));
        refresh();

        if (pressing_project)       draw_project(windows[5]);
        else if (pressing_mixer)    draw_mixer(windows[5]);
        else if (pressing_tempo)    draw_tempo(windows[5]);
        else if (mic_on)            draw_mic(windows[5]);
        else if (show_project_panel) draw_project(windows[5]);

        if ( show_project_panel || (!mic_on && !pressing_project && !pressing_mixer && !pressing_tempo)){
            // werase(windows[5]);

            if (pressing_track)
                wattron(windows[4], COLOR_PAIR(2));

            size_t page = (size_t)device.getActivePageId();

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

            // The fixed-size windows above can overlap the step display rows
            // drawn on stdscr (e.g. their borders sit on the same physical
            // row as LINES-5..LINES-1 on shorter terminals). Re-push just
            // those rows last so the step display always wins that overlap,
            // without blanking out the rest of stdscr over the windows.
            touchline(stdscr, LINES - 5, 5);
            refresh();

            change = false;
            change_data = false;
        }
    }
    
    waitForKeys.join();
    endwin();
    device.disconnect();

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

    layout_project_window();
}

void draw_mic(WINDOW* _win) {
    werase(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2, "MIC LEVEL                  MIC FX ");
    mvwprintw(_win, 2, 2, "%s                  %s", 
                            hBar(9, device.getMicLevel() ).c_str(), 
                            opz::toString(device.getMicFx()).c_str());
    wrefresh(_win);
}

void draw_project(WINDOW* _win) {
    size_t project_id = device.getActiveProjectId();
    uint8_t pattern_id = device.getActivePatternId();
    opz::opz_pattern pattern = device.getActivePattern();
    opz::opz_track_id track_active = device.getActiveTrackId();

    int lines, cols;
    getmaxyx(_win, lines, cols);

    werase(_win);
    box(_win, 0, 0);

    mvwprintw(_win, 0, 2, " PROJECT %02i ", project_id);
    mvwprintw(_win, 0, 18, " PATTERN %02i ", pattern_id);
    mvwprintw(_win, 0, 34, " MUTE GRP %i ", pattern.active_mute_group);
    uint8_t chain_pos = device.getActiveChainPos();
    if (chain_pos > 0)
        mvwprintw(_win, 0, cols - 14, " CHAIN %02i ", chain_pos);

    // CHAINED PATTERNS (TODO)
    int song_width = 4;
    int x_margin = (cols - song_width * 16) / 2;
    for (size_t i = 0; i < 16; i++) {
        int y = 2;
        int x = x_margin + i * song_width;
        mvwprintw(_win, y, x, "%02X", device.getProjectData().pattern_chain[pattern_id].pattern[i]);
        mvwprintw(_win, y+1, x, "%02X", device.getProjectData().pattern_chain[pattern_id].pattern[i+16]);
    }

    // PATTERN TRACK
    int name_width = 12;
    int step_width = (cols - name_width) / 16;
    x_margin = 2 + (cols - step_width * 16 - name_width) / 2;
    size_t step_current = device.getActiveStepId();

    size_t tracks = 16;
    for (size_t t = 0; t < tracks; t++) {
        int y = 5 + t;
        bool muted = device.getMuteTrack(pattern_id, t);
        bool send_tape = device.isSendToTape(pattern_id, opz::opz_track_id(t));
        bool send_master = device.isSendToMaster(pattern_id, opz::opz_track_id(t));

        if (muted) wattron(_win, COLOR_PAIR(5));
        else if (t == (size_t)track_active) wattron(_win, COLOR_PAIR(2));
        else if (t > 7) wattron(_win, COLOR_PAIR(4));

        char route_ch = ' ';
        if (send_tape && send_master) route_ch = '&';
        else if (send_tape) route_ch = 'T';
        else if (send_master) route_ch = 'M';

        mvwprintw(_win, y, x_margin, "%c %7s", route_ch, opz::toString(opz::opz_track_id(t)).c_str());

        if (muted) {
            wprintw(_win, " X");
            wattroff(_win, COLOR_PAIR(5));
        }
        else {
            wprintw(_win, "  ");
            if (t == (size_t)track_active) wattroff(_win, COLOR_PAIR(2));
            else if (t > 7) wattroff(_win, COLOR_PAIR(4));
        }

        size_t step_count = device.getTrackParameters(opz::opz_track_id(t)).step_count;
        size_t step_length = device.getTrackParameters(opz::opz_track_id(t)).step_length;
        if (step_count > 0 && step_length > 0) {
            size_t step = (step_current / step_length) % step_count;
            mvwprintw(_win, y, x_margin + name_width + step * step_width - 1, "[  ]");
        }
        for (size_t s = 0; s < step_count; s++) {
            int x = x_margin + name_width + s * step_width;
            size_t i = device.getNoteIdOffset(t, s);

            if (pattern.note[i].note == 0xFF) {
                if (muted) wattron(_win, COLOR_PAIR(5));
                else if (t > 7) wattron(_win, COLOR_PAIR(4));
                mvwprintw(_win, y, x, "--");
                if (muted) wattroff(_win, COLOR_PAIR(5));
                else if (t > 7) wattroff(_win, COLOR_PAIR(4));
            }
            else {
                if (muted) wattron(_win, COLOR_PAIR(5));
                else if (t == (size_t)track_active) wattron(_win, COLOR_PAIR(1));
                else if (t > 7) wattron(_win, COLOR_PAIR(2));
                mvwprintw(_win, y, x, "%02X", pattern.note[i].note);
                if (muted) wattroff(_win, COLOR_PAIR(5));
                else if (t == (size_t)track_active) wattroff(_win, COLOR_PAIR(1));
                else if (t > 7) wattroff(_win, COLOR_PAIR(2));
            }
        }
    }
    wrefresh(_win);
}

void draw_mixer(WINDOW* _win) {
    opz::opz_project_data project = device.getProjectData();
    uint8_t pattern_id = device.getActivePatternId();
    opz::opz_pattern pattern = device.getActivePattern();

    int lines, cols;
    getmaxyx(_win, lines, cols);

    werase(_win);
    box(_win, 0, 0);

    mvwprintw(_win, 0, 2, " MIXER ");
    mvwprintw(_win, 0, cols - 16, " MUTE GRP %i ", pattern.active_mute_group);

    mvwprintw(_win, 1, 2, "DRUMS               SYNTH                PUNCH                MASTER");
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win, 2, 2, "%s              %s               %s               %s",
                            hBar(7, project.drum_level).c_str(),
                            hBar(7, project.synth_level).c_str(),
                            hBar(7, project.punch_level).c_str(),
                            hBar(7, project.master_level).c_str());
    wattroff(_win, COLOR_PAIR(4));
    mvwprintw(_win, 3, 2, "%03i                 %03i                  %03i                  %03i",
                            (int)((int)project.drum_level / 2.55f),
                            (int)((int)project.synth_level / 2.55f),
                            (int)((int)project.punch_level / 2.55f),
                            (int)((int)project.master_level / 2.55f));

    int mute_col_w = std::max(4, (cols - 4) / 16);
    for (size_t t = 0; t < 16; t++) {
        int x = 2 + (int)t * mute_col_w;
        bool muted = device.getMuteTrack(pattern_id, t);
        const char* short_names[] = {"KI","SN","PE","SA","BA","LE","AR","CH","F1","F2","TP","MA","PF","MO","LI","MT"};

        if (muted) {
            wattron(_win, COLOR_PAIR(5));
            mvwprintw(_win, 5, x, "%s", short_names[t]);
            mvwprintw(_win, 6, x, "--");
            wattroff(_win, COLOR_PAIR(5));
        }
        else {
            if (t < 4) wattron(_win, COLOR_PAIR(3));
            else if (t < 8) wattron(_win, COLOR_PAIR(4));
            else wattron(_win, COLOR_PAIR(2));
            mvwprintw(_win, 5, x, "%s", short_names[t]);
            mvwprintw(_win, 6, x, "##");
            if (t < 4) wattroff(_win, COLOR_PAIR(3));
            else if (t < 8) wattroff(_win, COLOR_PAIR(4));
            else wattroff(_win, COLOR_PAIR(2));
        }
    }

    wrefresh(_win);
}

void draw_tempo(WINDOW* _win) {
    int lines, cols;
    getmaxyx(_win, lines, cols);

    opz::opz_project_data project = device.getProjectData();
    double pct = (device.getActiveStepId() % 8) / 8.0;
    
    werase(_win);
    box(_win, 0, 0);
    mvwprintw(_win, 1, 2,           "TEMPO               SWING");
    mvwprintw(_win, 2, 2,           "%03i                 %03i", project.tempo, (int)((int)project.swing / 2.55f) - 50);

    mvwprintw(_win, 1, cols - 28, "SOUND                LEVEL");
    mvwprintw(_win, 2, cols - 31, "%8s                  %03i", opz::metronomeSoundString(project.metronome_sound).c_str(), (int)((int)project.metronome_level / 2.55f) );

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
    mvwprintw(_win, 2, 1, "       %s %s %s %s",   hBar(7, (size_t)device.getActivePageParameters().param1).c_str(),
                                            hBar(7, (size_t)device.getActivePageParameters().param2).c_str(),
                                            hBar(7, (size_t)device.getActivePageParameters().filter).c_str(),
                                            hBar(7, (size_t)device.getActivePageParameters().resonance).c_str() );
    wattroff(_win, COLOR_PAIR(4));
    mvwprintw(_win, 3, 1, "       %7i %7i %7i %7i", 
                                            (int)((int)device.getActivePageParameters().param1 / 2.55f), 
                                            (int)((int)device.getActivePageParameters().param2 / 2.55f), 
                                            (int)((int)device.getActivePageParameters().filter / 2.55f), 
                                            (int)((int)device.getActivePageParameters().resonance / 2.55f) );
}

// PAGE 2: ENVELOPE
void draw_page_two(WINDOW* _win) {
    size_t track_id = device.getActiveTrackId();
    size_t page_id = device.getActivePageId();

    werase(_win);

    if (page_id == 1) wattron(_win, COLOR_PAIR(1));
    box(_win, 0, 0);
    wattroff(_win, COLOR_PAIR(1));

    mvwprintw(_win, 1, 1, "ENVEL.");

    float attack = (int)device.getActivePageParameters().attack / 255.0f;
    float decay = (int)device.getActivePageParameters().decay / 255.0f;
    float sustain = (int)device.getActivePageParameters().sustain / 255.0f;
    float release = (int)device.getActivePageParameters().release / 255.0f;
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
    mvwprintw(_win,2, 1, "       %s %s         %s",   hBar(7, (size_t)device.getActivePageParameters().lfo_depth).c_str(),
                                            hBar(7, (size_t)device.getActivePageParameters().lfo_speed).c_str(),
                                            opz::lfoShapeShapeString( device.getActivePageParameters().lfo_shape ).c_str());
    wattroff(_win, COLOR_PAIR(4));
    mvwprintw(_win,3, 1, "       %3i     %3i     %3s    %5s", 
                                            (int)((int)device.getActivePageParameters().lfo_depth / 2.55f), 
                                            (int)((int)device.getActivePageParameters().lfo_speed / 2.55f),
                                            opz::lfoDestinationShortString( device.getActivePageParameters().lfo_value ).c_str(),
                                            opz::lfoShapeShortString( device.getActivePageParameters().lfo_shape ).c_str() );
}

void draw_page_four(WINDOW* _win) {
    // PAGE 4: FX / PAN & LEVEL
    mvwprintw(_win, 1, 1, " FX  1       2");
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win, 2, 1, "     %s %s", hBar(7, (size_t)device.getActivePageParameters().fx1).c_str(),
                                            hBar(7, (size_t)device.getActivePageParameters().fx2).c_str());
    wattroff(_win, COLOR_PAIR(4));
    mvwprintw(_win, 3, 1, "     %7i %7i", 
                                            (int)((int)device.getActivePageParameters().fx1 / 2.55f),
                                            (int)((int)device.getActivePageParameters().fx2 / 2.55f) );

    mvwprintw(_win, 5, 1, " PAN L             R");
    mvwprintw(_win, 6, 1, "     ");
    for (size_t i = 0; i < 15; i++) {
        size_t p = device.getActivePageParameters().pan;
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

    mvwprintw(_win, 9, 1, " LEVEL     %03i", (int)( (int)device.getActivePageParameters().level / 2.55f));
    wattron(_win, COLOR_PAIR(4));
    mvwprintw(_win, 10, 1, "     %s", hBar(15, (size_t)device.getActivePageParameters().level).c_str());
    wattroff(_win, COLOR_PAIR(4));
}

void draw_track_params(WINDOW* _win) {
    mvwprintw(_win, 1,  2, "NOTE");
    mvwprintw(_win, 3,  2, "    LENGTH");
    mvwprintw(_win, 4,  2, "%10s", opz::noteLengthString( device.getActiveTrackParameters().note_length ).c_str() );
    mvwprintw(_win, 6,  2, "     STYLE");
    mvwprintw(_win, 7,  2, "%10s", opz::noteStyleString( device.getActiveTrackId(), device.getActivePageParameters().note_style ).c_str() );
    mvwprintw(_win, 10, 2, "  QUANTIZE");
    mvwprintw(_win, 11, 2, "%9i%%", (int)((int)device.getActiveTrackParameters().quantize / 2.55f));
    mvwprintw(_win, 13, 2, "PORTAMENTO");
    mvwprintw(_win, 14, 2, "%9i%%", (int)((int)device.getActivePageParameters().portamento / 2.55f) );
}
