
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
const int PROJECT_FULL_HEIGHT = 23;  // draw_pattern()'s content never exceeds this
const int BOTTOM_BAR_MARGIN  = 6;    // step display rows + a blank line of breathing room

bool change = true;

// --- PROJECT view cursor (arrow keys, only while pressing_project) ---
// Up/Down moves the highlighted track (row) and pushes a real track-select to
// the device; Left/Right moves the highlighted step (column), local-only, to
// pick a step to inspect/edit.
std::atomic<int> proj_cursor_track(0);
std::atomic<int> proj_cursor_step(0);

// --- MIXER view cursor (arrow keys, only while mixer mode is active) ---
// Left/Right cycles through a single combined list: the 16 individual tracks
// (0-15) followed by the 4 mixer groups drum/synth/punch/master (16-19).
// Up/Down adjusts gain for whichever is selected: an individual track's
// sound_param.level (part of the pattern data, written via the same proven
// full-bank rewrite note-editing uses) or a group's level (written via 0x0c).
// Enter always mutes/unmutes whichever track is currently selected.
const int MIXER_CURSOR_COUNT = 20; // 16 tracks + 4 groups
std::atomic<int> mixer_cursor(0);
bool mixer_cursor_is_group() { return mixer_cursor.load() >= 16; }
int mixer_cursor_group_index() { return mixer_cursor.load() - 16; } // 0=drum 1=synth 2=punch 3=master
int mixer_cursor_track_index() { return mixer_cursor.load() % 16; }

// --- TEMPO view cursor (arrow keys, only while tempo mode is active) ---
// Left/Right cycles which property is selected; Up/Down adjusts it by +-1.
// Entirely independent from track/pattern state - no side effects there.
enum tempo_prop_t { TEMPO_PROP_BPM = 0, TEMPO_PROP_SWING, TEMPO_PROP_METRO_LEVEL, TEMPO_PROP_METRO_SOUND, TEMPO_PROP_COUNT };
std::atomic<int> tempo_cursor_prop(0);

// pressing_tempo/pressing_mixer (below, in main()) mirror the OP-Z's own
// physical tempo/mixer key state, which requires physically holding a button
// on the device - unusable together with typing arrow keys on the keyboard.
// These give a keyboard-only way to enter the same views: 't'/'m' toggle
// them, and every check below is "pressing_X || X_mode".
std::atomic<bool> tempo_mode(false);
std::atomic<bool> mixer_mode(false);
std::atomic<bool> project_mode(false);
std::atomic<bool> screen_mode(false);
std::atomic<bool> need_pattern_refresh(false);

// Note editing: Enter starts editing, hex digits type, Enter confirms
std::atomic<bool> grid_editing_note(false);
std::string grid_note_input;

// Selection: SHIFT+arrow extends, stored as anchor + current cursor
std::atomic<bool> grid_sel_active(false);
int grid_sel_anchor_track = 0;
int grid_sel_anchor_step = 0;

// Clipboard: rectangular region of MIDI note values (0xFF = empty)
std::vector<std::vector<uint8_t>> grid_clipboard;

// --- Track editing / write-back state ---
// Local editable copy of the 16-pattern bank. Edits are staged here and only
// pushed to the device (as a full 0x09/0x0a bank write) when the user asks to
// send them, since inbound 0x02 edits are not applied by the device.
std::mutex edit_mtx;
std::atomic<bool> edit_mode(false);
opz::opz_pattern edit_bank[16];
size_t edit_step_cursor = 0;
std::string edit_status = "";

char step_component_char(uint16_t mask) {
    if (!mask) return ' ';
    if (mask & (mask - 1)) {
        int count = __builtin_popcount(mask);
        return (count <= 9) ? ('0' + count) : '+';
    }
    if (mask & 0x0001) return 'H';  // 1/2 (half)
    if (mask & 0x0002) return 'Q';  // 1/4 (quarter)
    if (mask & 0x0004) return 'E';  // 3/4
    if (mask & 0x0008) return 'W';  // 1/8
    if (mask & 0x0010) return '/';  // ramp up
    if (mask & 0x0020) return '\\'; // ramp down
    if (mask & 0x0040) return '?';  // random
    if (mask & 0x0080) return '~';  // pulse width
    if (mask & 0x0100) return '>';  // sweep right
    if (mask & 0x0200) return '<';  // sweep left
    if (mask & 0x0400) return 'x';  // multiply
    if (mask & 0x0800) return 'L';  // note length
    if (mask & 0x1000) return 'S';  // note style
    if (mask & 0x2000) return 'T';  // tonality
    if (mask & 0x4000) return '!';  // parameter spark
    return '+';
}

bool grid_in_selection(int track, int step) {
    if (!grid_sel_active.load()) return false;
    int t0 = std::min(grid_sel_anchor_track, proj_cursor_track.load());
    int t1 = std::max(grid_sel_anchor_track, proj_cursor_track.load());
    int s0 = std::min(grid_sel_anchor_step, proj_cursor_step.load());
    int s1 = std::max(grid_sel_anchor_step, proj_cursor_step.load());
    return track >= t0 && track <= t1 && step >= s0 && step <= s1;
}

bool set_note_at(int track, int step, uint8_t note_val) {
    device.requestPatternSync(1.0);
    uint8_t address = device.getPatternAddress();
    uint16_t id = device.getPatternId();
    size_t pattern_id = device.getActivePatternId();

    std::lock_guard<std::mutex> lock(edit_mtx);
    memcpy(edit_bank, &device.getProjectData().pattern[0], sizeof(edit_bank));

    size_t note_offset = device.getNoteIdOffset(track, step);
    size_t notes_total = device.getNotesPerTrack(opz::opz_track_id(track));
    for (size_t i = 0; i < notes_total; i++) {
        if (note_val == 0xFF) {
            edit_bank[pattern_id].note[note_offset + i].note = 0xFF;
        } else {
            edit_bank[pattern_id].note[note_offset + i].note = note_val;
            edit_bank[pattern_id].note[note_offset + i].duration = 6200;
            edit_bank[pattern_id].note[note_offset + i].velocity = 0x64;
            edit_bank[pattern_id].note[note_offset + i].micro_adjustment = 0;
            edit_bank[pattern_id].note[note_offset + i].age = 0;
        }
    }

    int acked = device.sendPattern(edit_bank, address, id);
    device.loadPatternBank(edit_bank);
    return acked > 0;
}

void grid_copy_selection() {
    int t0 = std::min(grid_sel_anchor_track, proj_cursor_track.load());
    int t1 = std::max(grid_sel_anchor_track, proj_cursor_track.load());
    int s0 = std::min(grid_sel_anchor_step, proj_cursor_step.load());
    int s1 = std::max(grid_sel_anchor_step, proj_cursor_step.load());

    opz::opz_pattern pattern = device.getActivePattern();
    grid_clipboard.clear();
    for (int t = t0; t <= t1; t++) {
        std::vector<uint8_t> row;
        for (int s = s0; s <= s1; s++) {
            size_t note_offset = device.getNoteIdOffset(t, s);
            row.push_back(pattern.note[note_offset].note);
        }
        grid_clipboard.push_back(row);
    }
}

void grid_paste_at_cursor() {
    if (grid_clipboard.empty()) return;

    device.requestPatternSync(1.0);
    uint8_t address = device.getPatternAddress();
    uint16_t id = device.getPatternId();
    size_t pattern_id = device.getActivePatternId();

    int start_t = proj_cursor_track.load();
    int start_s = proj_cursor_step.load();

    std::lock_guard<std::mutex> lock(edit_mtx);
    memcpy(edit_bank, &device.getProjectData().pattern[0], sizeof(edit_bank));

    for (size_t dt = 0; dt < grid_clipboard.size(); dt++) {
        int t = start_t + (int)dt;
        if (t >= 16) break;
        for (size_t ds = 0; ds < grid_clipboard[dt].size(); ds++) {
            int s = start_s + (int)ds;
            if (s >= 16) break;
            size_t note_offset = device.getNoteIdOffset(t, s);
            size_t notes_total = device.getNotesPerTrack(opz::opz_track_id(t));
            uint8_t val = grid_clipboard[dt][ds];
            for (size_t i = 0; i < notes_total; i++) {
                if (val == 0xFF) {
                    edit_bank[pattern_id].note[note_offset + i].note = 0xFF;
                } else {
                    edit_bank[pattern_id].note[note_offset + i].note = val;
                    edit_bank[pattern_id].note[note_offset + i].duration = 6200;
                    edit_bank[pattern_id].note[note_offset + i].velocity = 0x64;
                    edit_bank[pattern_id].note[note_offset + i].micro_adjustment = 0;
                    edit_bank[pattern_id].note[note_offset + i].age = 0;
                }
            }
        }
    }

    device.sendPattern(edit_bank, address, id);
    device.loadPatternBank(edit_bank);
}

void grid_delete_selection() {
    int t0 = std::min(grid_sel_anchor_track, proj_cursor_track.load());
    int t1 = std::max(grid_sel_anchor_track, proj_cursor_track.load());
    int s0 = std::min(grid_sel_anchor_step, proj_cursor_step.load());
    int s1 = std::max(grid_sel_anchor_step, proj_cursor_step.load());

    device.requestPatternSync(1.0);
    uint8_t address = device.getPatternAddress();
    uint16_t id = device.getPatternId();
    size_t pattern_id = device.getActivePatternId();

    std::lock_guard<std::mutex> lock(edit_mtx);
    memcpy(edit_bank, &device.getProjectData().pattern[0], sizeof(edit_bank));

    for (int t = t0; t <= t1; t++) {
        for (int s = s0; s <= s1; s++) {
            size_t note_offset = device.getNoteIdOffset(t, s);
            size_t notes_total = device.getNotesPerTrack(opz::opz_track_id(t));
            for (size_t i = 0; i < notes_total; i++)
                edit_bank[pattern_id].note[note_offset + i].note = 0xFF;
        }
    }

    device.sendPattern(edit_bank, address, id);
    device.loadPatternBank(edit_bank);
}

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

// Toggles mute for a single track by flipping its bit in the active pattern's
// mute[] bitmask and pushing a full bank rewrite - mute lives in the pattern
// data (like notes/steps), not in the unconfirmed 0x12 message, so this
// reuses the same proven write path as note editing instead of guessing.
bool toggle_track_mute_and_send(opz::opz_track_id _track) {
    device.requestPatternSync(1.0);

    uint8_t  address = device.getPatternAddress();
    uint16_t id      = device.getPatternId();
    size_t   pattern_id = device.getActivePatternId();

    std::lock_guard<std::mutex> lock(edit_mtx);
    memcpy(edit_bank, &device.getProjectData().pattern[0], sizeof(edit_bank));
    edit_bank[pattern_id].mute[(size_t)_track / 4] ^= opz::opz_mute_masks[(size_t)_track % 4];

    int acked = device.sendPattern(edit_bank, address, id);
    device.loadPatternBank(edit_bank);
    return acked > 0;
}

// Adjusts one track's individual channel gain (sound_param.level, the same
// LEVEL parameter shown on page 4 for the active track) by _delta and pushes
// a full bank rewrite - same proven write path as mute/notes.
bool adjust_track_level_and_send(opz::opz_track_id _track, int _delta) {
    device.requestPatternSync(1.0);

    uint8_t  address = device.getPatternAddress();
    uint16_t id      = device.getPatternId();
    size_t   pattern_id = device.getActivePatternId();

    std::lock_guard<std::mutex> lock(edit_mtx);
    memcpy(edit_bank, &device.getProjectData().pattern[0], sizeof(edit_bank));
    int cur = edit_bank[pattern_id].sound_param[(size_t)_track].level;
    edit_bank[pattern_id].sound_param[(size_t)_track].level = (uint8_t)std::min(255, std::max(0, cur + _delta));

    int acked = device.sendPattern(edit_bank, address, id);
    device.loadPatternBank(edit_bank);
    return acked > 0;
}

// global
void draw_mic(WINDOW* _window);
void draw_pattern(WINDOW* _window);
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

    raw();
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

    // Overlay that temporarily replaces the track-parameter windows (0-4) with
    // the project/mixer/tempo views when the project panel is always visible.
    windows.push_back( newwin(TRACK_PROPS_HEIGHT + 1, TRACK_PROPS_WIDTH, 1, 0) );  //  MIXER / TEMPO

    signal(SIGWINCH, handle_winch);

    bool change_data = true;
    bool was_params_overlay = false;
    bool pressing_track = false;
    bool pressing_project = false;
    bool pressing_mixer = false;
    bool pressing_tempo = false;
    bool pressing_screen = false;
    bool mic_on = false;

    // Listen to key events (no cc, neighter notes)
    device.setEventCallback( [&](opz::opz_event_id _id, int _value) {
        change = true;

        if (_id == opz::KEY_TRACK)           pressing_track = _value;
        else if (_id == opz::KEY_PROJECT)    pressing_project = _value;
        else if (_id == opz::KEY_MIXER)      pressing_mixer = _value;
        else if (_id == opz::KEY_TEMPO)      pressing_tempo = _value;
        else if (_id == opz::KEY_SCREEN)     pressing_screen = _value;
        else if (_id == opz::MICROPHONE_MODE_CHANGE) mic_on = _value != 0;
        else if (_id == opz::PATTERN_DOWNLOADED || _id == opz::PATTERN_CHANGE || _id == opz::TRACK_CHANGE || _id == opz::SEQUENCE_CHANGE || _id == opz::PAGE_CHANGE || _id == opz::TRACK_PARAMETER_CHANGE || _id == opz::MUTE_CHANGE || _id == opz::PROJECT_CHANGE) {
            change_data = true;
            if (_id == opz::PROJECT_CHANGE || _id == opz::PATTERN_CHANGE)
                need_pattern_refresh.store(true);
        }
    } );

    std::thread waitForKeys([&](){
        int ch;
        while ( true ) {
            ch = getch();

            if (ch == 'x' || ch == 'q' || ch == 'Q') {
                keepRunnig.store(false);
                break;
            }
            else if (ch == 't') {
                bool entering = !tempo_mode.load();
                tempo_mode.store(entering);
                if (entering) { mixer_mode.store(false); project_mode.store(false); screen_mode.store(false); }
                change = true;
                change_data = true;
            }
            else if (ch == 'm') {
                bool entering = !mixer_mode.load();
                mixer_mode.store(entering);
                if (entering) { tempo_mode.store(false); project_mode.store(false); screen_mode.store(false); }
                change = true;
                change_data = true;
            }
            else if (ch == 'p') {
                bool entering = !project_mode.load();
                project_mode.store(entering);
                if (entering) { mixer_mode.store(false); tempo_mode.store(false); screen_mode.store(false); }
                change = true;
                change_data = true;
            }
            else if (ch == 's' && !edit_mode.load()) {
                bool entering = !screen_mode.load();
                screen_mode.store(entering);
                if (entering) { mixer_mode.store(false); tempo_mode.store(false); project_mode.store(false); }
                change = true;
                change_data = true;
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
                if ((pressing_mixer || mixer_mode.load())) {
                    if (mixer_cursor_is_group()) {
                        int g = mixer_cursor_group_index();
                        const opz::opz_project_data& proj = device.getProjectData();
                        uint8_t cur = (g == 0) ? proj.drum_level : (g == 1) ? proj.synth_level : (g == 2) ? proj.punch_level : proj.master_level;
                        device.sendGroupLevel(g, (uint8_t)std::min(255, (int)cur + 8));
                    }
                    else {
                        adjust_track_level_and_send(opz::opz_track_id(mixer_cursor_track_index()), 8);
                    }
                    change = true;
                    change_data = true;
                }
                else if ((pressing_tempo || tempo_mode.load())) {
                    int p = tempo_cursor_prop.load();
                    const opz::opz_project_data& proj = device.getProjectData();
                    if (p == TEMPO_PROP_BPM)               { uint8_t v = proj.tempo;           if (v < 200) device.sendTempo(v + 1); }
                    else if (p == TEMPO_PROP_SWING)        { uint8_t v = proj.swing;           if (v < 255) device.sendSwing(v + 1); }
                    else if (p == TEMPO_PROP_METRO_LEVEL)  { uint8_t v = proj.metronome_level; if (v < 255) device.sendMetronomeLevel(v + 1); }
                    else if (p == TEMPO_PROP_METRO_SOUND)  { uint8_t v = proj.metronome_sound; if (v < 255) device.sendMetronomeSound(v + 1); }
                    change = true;
                    change_data = true;
                }
                else if (pressing_project || project_mode.load() || show_project_panel) {
                    int t = proj_cursor_track.load() - 1;
                    if (t < 0) t = 15;
                    proj_cursor_track.store(t);
                    device.sendTrackSelect(opz::opz_track_id(t));
                    grid_sel_active.store(false);
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_DOWN) {
                if ((pressing_mixer || mixer_mode.load())) {
                    if (mixer_cursor_is_group()) {
                        int g = mixer_cursor_group_index();
                        const opz::opz_project_data& proj = device.getProjectData();
                        uint8_t cur = (g == 0) ? proj.drum_level : (g == 1) ? proj.synth_level : (g == 2) ? proj.punch_level : proj.master_level;
                        device.sendGroupLevel(g, (uint8_t)std::max(0, (int)cur - 8));
                    }
                    else {
                        adjust_track_level_and_send(opz::opz_track_id(mixer_cursor_track_index()), -8);
                    }
                    change = true;
                    change_data = true;
                }
                else if ((pressing_tempo || tempo_mode.load())) {
                    int p = tempo_cursor_prop.load();
                    const opz::opz_project_data& proj = device.getProjectData();
                    if (p == TEMPO_PROP_BPM)               { uint8_t v = proj.tempo;           if (v > 40)  device.sendTempo(v - 1); }
                    else if (p == TEMPO_PROP_SWING)        { uint8_t v = proj.swing;           if (v > 0)   device.sendSwing(v - 1); }
                    else if (p == TEMPO_PROP_METRO_LEVEL)  { uint8_t v = proj.metronome_level; if (v > 0)   device.sendMetronomeLevel(v - 1); }
                    else if (p == TEMPO_PROP_METRO_SOUND)  { uint8_t v = proj.metronome_sound; if (v > 0)   device.sendMetronomeSound(v - 1); }
                    change = true;
                    change_data = true;
                }
                else if (pressing_project || project_mode.load() || show_project_panel) {
                    int t = (proj_cursor_track.load() + 1) % 16;
                    proj_cursor_track.store(t);
                    device.sendTrackSelect(opz::opz_track_id(t));
                    grid_sel_active.store(false);
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_LEFT) {
                if ((pressing_mixer || mixer_mode.load())) {
                    mixer_cursor.store((mixer_cursor.load() + MIXER_CURSOR_COUNT - 1) % MIXER_CURSOR_COUNT);
                    change = true;
                    change_data = true;
                }
                else if ((pressing_tempo || tempo_mode.load())) {
                    tempo_cursor_prop.store((tempo_cursor_prop.load() + TEMPO_PROP_COUNT - 1) % TEMPO_PROP_COUNT);
                    change = true;
                    change_data = true;
                }
                else if (pressing_project || project_mode.load() || show_project_panel) {
                    int s = proj_cursor_step.load() - 1;
                    if (s < 0) s = 15;
                    proj_cursor_step.store(s);
                    grid_sel_active.store(false);
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == KEY_RIGHT) {
                if ((pressing_mixer || mixer_mode.load())) {
                    mixer_cursor.store((mixer_cursor.load() + 1) % MIXER_CURSOR_COUNT);
                    change = true;
                    change_data = true;
                }
                else if ((pressing_tempo || tempo_mode.load())) {
                    tempo_cursor_prop.store((tempo_cursor_prop.load() + 1) % TEMPO_PROP_COUNT);
                    change = true;
                    change_data = true;
                }
                else if (pressing_project || project_mode.load() || show_project_panel) {
                    int s = (proj_cursor_step.load() + 1) % 16;
                    proj_cursor_step.store(s);
                    grid_sel_active.store(false);
                    change = true;
                    change_data = true;
                }
            }
            // SHIFT+arrow keys for selection in track panel
            else if (ch == KEY_SR || ch == 337) { // shift-up
                if (!grid_sel_active.load()) {
                    grid_sel_active.store(true);
                    grid_sel_anchor_track = proj_cursor_track.load();
                    grid_sel_anchor_step = proj_cursor_step.load();
                }
                int t = proj_cursor_track.load() - 1;
                if (t >= 0) proj_cursor_track.store(t);
                change = true;
            }
            else if (ch == KEY_SF || ch == 336) { // shift-down
                if (!grid_sel_active.load()) {
                    grid_sel_active.store(true);
                    grid_sel_anchor_track = proj_cursor_track.load();
                    grid_sel_anchor_step = proj_cursor_step.load();
                }
                int t = proj_cursor_track.load() + 1;
                if (t < 16) proj_cursor_track.store(t);
                change = true;
            }
            else if (ch == KEY_SLEFT || ch == 393) { // shift-left
                if (!grid_sel_active.load()) {
                    grid_sel_active.store(true);
                    grid_sel_anchor_track = proj_cursor_track.load();
                    grid_sel_anchor_step = proj_cursor_step.load();
                }
                int s = proj_cursor_step.load() - 1;
                if (s >= 0) proj_cursor_step.store(s);
                change = true;
            }
            else if (ch == KEY_SRIGHT || ch == 402) { // shift-right
                if (!grid_sel_active.load()) {
                    grid_sel_active.store(true);
                    grid_sel_anchor_track = proj_cursor_track.load();
                    grid_sel_anchor_step = proj_cursor_step.load();
                }
                int s = proj_cursor_step.load() + 1;
                if (s < 16) proj_cursor_step.store(s);
                change = true;
            }
            else if (ch == KEY_ENTER || ch == '\n' || ch == '\r') {
                if (grid_editing_note.load()) {
                    if (!grid_note_input.empty()) {
                        int val = (int)strtol(grid_note_input.c_str(), nullptr, 16);
                        if (val >= 0 && val <= 127) {
                            set_note_at(proj_cursor_track.load(), proj_cursor_step.load(), (uint8_t)val);
                            change_data = true;
                        }
                    }
                    grid_editing_note.store(false);
                    grid_note_input.clear();
                    change = true;
                }
                else if ((pressing_mixer || mixer_mode.load()) && !mixer_cursor_is_group()) {
                    edit_status = "toggling mute...";
                    change = true;
                    bool ok = toggle_track_mute_and_send(opz::opz_track_id(mixer_cursor_track_index()));
                    edit_status = ok ? "" : "mute toggle failed (no ack from device)";
                    change = true;
                    change_data = true;
                }
                else if (pressing_project || project_mode.load() || show_project_panel) {
                    grid_editing_note.store(true);
                    grid_note_input.clear();
                    grid_sel_active.store(false);
                    change = true;
                }
            }
            else if (ch == 27) { // Escape
                if (grid_editing_note.load()) {
                    grid_editing_note.store(false);
                    grid_note_input.clear();
                    change = true;
                }
                else if (grid_sel_active.load()) {
                    grid_sel_active.store(false);
                    change = true;
                }
            }
            else if (ch == KEY_DC || ch == 127) { // DEL / Backspace
                if (grid_editing_note.load()) {
                    if (!grid_note_input.empty()) grid_note_input.pop_back();
                    change = true;
                }
                else if (grid_sel_active.load()) {
                    grid_delete_selection();
                    grid_sel_active.store(false);
                    change = true;
                    change_data = true;
                }
                else if (pressing_project || project_mode.load() || show_project_panel) {
                    set_note_at(proj_cursor_track.load(), proj_cursor_step.load(), 0xFF);
                    change = true;
                    change_data = true;
                }
            }
            else if (ch == 0x03) { // CTRL+C
                if (grid_sel_active.load()) {
                    grid_copy_selection();
                    edit_status = "copied";
                } else if (pressing_project || project_mode.load() || show_project_panel) {
                    grid_sel_anchor_track = proj_cursor_track.load();
                    grid_sel_anchor_step = proj_cursor_step.load();
                    grid_copy_selection();
                    edit_status = "copied step";
                }
                change = true;
            }
            else if (ch == 0x18) { // CTRL+X (cut)
                if (grid_sel_active.load()) {
                    grid_copy_selection();
                    grid_delete_selection();
                    grid_sel_active.store(false);
                    edit_status = "cut";
                } else if (pressing_project || project_mode.load() || show_project_panel) {
                    grid_sel_anchor_track = proj_cursor_track.load();
                    grid_sel_anchor_step = proj_cursor_step.load();
                    grid_copy_selection();
                    set_note_at(proj_cursor_track.load(), proj_cursor_step.load(), 0xFF);
                    edit_status = "cut step";
                }
                change = true;
                change_data = true;
            }
            else if (ch == 0x16) { // CTRL+V (paste)
                if (!grid_clipboard.empty() && (pressing_project || project_mode.load() || show_project_panel)) {
                    grid_paste_at_cursor();
                    grid_sel_active.store(false);
                    edit_status = "pasted";
                    change = true;
                    change_data = true;
                }
            }
            else if (grid_editing_note.load()) {
                if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
                    if (grid_note_input.size() < 2)
                        grid_note_input += (char)toupper(ch);
                    change = true;
                }
            }
            else if ((ch >= '1' && ch <= '9') || ch == '0') {
                int idx = (ch == '0') ? 9 : (ch - '1');
                edit_status = "switching pattern...";
                change = true;
                bool ok = device.sendPatternSelect(idx);
                edit_status = ok ? "" : "pattern select failed (no chain baseline yet, or bank refresh timed out)";
                change = true;
                change_data = true;
            }
            // Shift+number (standard US-keyboard shift symbols) selects the
            // project instead of the pattern: ! @ # $ % ^ & * ( ) = shift 1-0.
            else if (ch == '!' || ch == '@' || ch == '#' || ch == '$' || ch == '%' ||
                     ch == '^' || ch == '&' || ch == '*' || ch == '(' || ch == ')') {
                static const std::string shift_symbols = "!@#$%^&*()";
                int idx = shift_symbols.find(ch);
                edit_status = "switching project...";
                change = true;
                bool ok = device.sendProjectSelect(idx);
                edit_status = ok ? "" : "project select failed (no chain baseline yet, or bank refresh timed out)";
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

        if (need_pattern_refresh.load()) {
            need_pattern_refresh.store(false);
            std::vector<unsigned char> dump_req = { 0xF0, 0x00, 0x20, 0x76, 0x01, 0x08, 0xF7 };
            device.send(dump_req);
        }

        if (!change)
            continue;

        opz::opz_track_id track_id = device.getActiveTrackId();
        opz::opz_pattern pattern = get_display_pattern();

        std::string title_name = opz::toString(track_id);

        if (mic_on) title_name = "MICROPHONE";
        else if (pressing_project || project_mode.load()) title_name = "PROJECTS";
        else if ((pressing_mixer || mixer_mode.load()))   title_name = "MIXER";
        else if ((pressing_tempo || tempo_mode.load()))   title_name = "TEMPO";
        else if (screen_mode.load()) title_name = title_name;
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

            uint16_t comp_mask = (step_idx < 256) ? pattern.step[step_idx].components_bitmask : 0;
            if ( pattern.note[ note ].note == 0xFF) {
                if (has_components) {
                    attron(COLOR_PAIR(5));
                    mvprintw(LINES-4, x, "%c", step_component_char(comp_mask));
                    attroff(COLOR_PAIR(5));
                }
                else
                    mvprintw(LINES-4, x, "-");
            }
            else {
                if (has_components) {
                    attron(COLOR_PAIR(1));
                    mvprintw(LINES-4, x, "%c", step_component_char(comp_mask));
                    attroff(COLOR_PAIR(1));
                }
                else {
                    attron(COLOR_PAIR(2));
                    mvprintw(LINES-4, x, "o");
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

        bool project_active = (pressing_project || project_mode.load());
        bool mixer_active = (pressing_mixer || mixer_mode.load()) && !project_active;
        bool tempo_active = !mixer_active && (pressing_tempo || tempo_mode.load()) && !project_active;
        // With the pattern panel always visible, project/mixer/tempo take over
        // the parameters panel on the left; otherwise they take over windows[5].
        bool params_overlay = show_project_panel && (project_active || mixer_active || tempo_active);

        if (params_overlay) {
            // The tempo view is taller than the parameter windows; give it
            // room for its margin (it may cover the top of a stacked project panel).
            int overlay_h = tempo_active ? std::min(PROJECT_FULL_HEIGHT, LINES - BOTTOM_BAR_MARGIN)
                                         : TRACK_PROPS_HEIGHT + 1;
            int cur_h, cur_w;
            getmaxyx(windows[6], cur_h, cur_w);
            if (cur_h != overlay_h) {
                wresize(windows[6], overlay_h, TRACK_PROPS_WIDTH);
                touchwin(stdscr);
                touchwin(windows[5]);
            }
            draw_pattern(windows[5]);
            if (project_active)    draw_project(windows[6]);
            else if (mixer_active) draw_mixer(windows[6]);
            else                   draw_tempo(windows[6]);
        }
        else if (pressing_project || project_mode.load())  draw_pattern(windows[5]);
        else if (mixer_active)      draw_mixer(windows[5]);
        else if (tempo_active)      draw_tempo(windows[5]);
        else if (mic_on)            draw_mic(windows[5]);
        else if (show_project_panel) draw_pattern(windows[5]);

        if (was_params_overlay && !params_overlay) {
            for (size_t i = 0; i < 5; i++)
                touchwin(windows[i]);
            touchwin(stdscr);
            touchwin(windows[5]);
            wrefresh(windows[5]);
            change_data = true;
        }
        was_params_overlay = params_overlay;

        if (!params_overlay && ( show_project_panel || (!mic_on && !(pressing_project || project_mode.load()) && !(pressing_mixer || mixer_mode.load()) && !(pressing_tempo || tempo_mode.load())))){
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

void draw_pattern(WINDOW* _win) {
    size_t project_id = device.getActiveProjectId();
    uint8_t pattern_id = device.getActivePatternId();
    opz::opz_pattern pattern = device.getActivePattern();
    opz::opz_track_id track_active = device.getActiveTrackId();

    int lines, cols;
    getmaxyx(_win, lines, cols);

    werase(_win);
    box(_win, 0, 0);

    mvwprintw(_win, 0, 2, " PROJECT %02zu ", project_id + 1);
    mvwprintw(_win, 0, 18, " PATTERN %02i ", pattern_id + 1);
    mvwprintw(_win, 0, 34, " MUTE GRP %i ", pattern.active_mute_group);
    uint8_t chain_pos = device.getActiveChainPos();
    if (chain_pos > 0)
        mvwprintw(_win, 0, cols - 14, " CHAIN %02i ", chain_pos);

    int cur_track = proj_cursor_track.load() % 16;
    int cur_step = proj_cursor_step.load() % 16;
    mvwprintw(_win, 1, 2, "STEP EDIT   TRACK %-7s STEP %02i   (up/down: track   left/right: step)",
              opz::toString(opz::opz_track_id(cur_track)).c_str(), cur_step + 1);
    if (grid_sel_active.load())
        mvwprintw(_win, 1, cols - 42, " SELECT (Ctrl-C/DEL/Esc) ");
    else if (grid_editing_note.load())
        mvwprintw(_win, 1, cols - 22, " NOTE: %s_ ", grid_note_input.c_str());

    int song_width = 4;
    int x_margin = (cols - song_width * 16) / 2;

    // // CHAINED PATTERNS
    // for (size_t i = 0; i < 16; i++) {
    //     int y = 2;
    //     int x = x_margin + i * song_width;
    //     mvwprintw(_win, y, x, "%02X", device.getProjectData().pattern_chain[pattern_id].pattern[i]);
    //     mvwprintw(_win, y+1, x, "%02X", device.getProjectData().pattern_chain[pattern_id].pattern[i+16]);
    // }

    // PATTERN TRACKS
    int name_width = 12;
    int step_width = (cols - name_width) / 16;
    x_margin = 2 + (cols - step_width * 16 - name_width) / 2;
    size_t step_current = device.getActiveStepId();

    // Selection bounds
    int sel_t0 = 0, sel_t1 = 0, sel_s0 = 0, sel_s1 = 0;
    bool has_sel = grid_sel_active.load();
    if (has_sel) {
        sel_t0 = std::min(grid_sel_anchor_track, cur_track);
        sel_t1 = std::max(grid_sel_anchor_track, cur_track);
        sel_s0 = std::min(grid_sel_anchor_step, cur_step);
        sel_s1 = std::max(grid_sel_anchor_step, cur_step);
    }

    size_t tracks = 16;
    for (size_t t = 0; t < tracks; t++) {
        int y = 3 + t;
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

            bool is_cursor = (t == (size_t)cur_track && s == (size_t)cur_step);
            bool in_sel = has_sel && (int)t >= sel_t0 && (int)t <= sel_t1 && (int)s >= sel_s0 && (int)s <= sel_s1;
            if (is_cursor) wattron(_win, A_REVERSE);
            else if (in_sel) wattron(_win, A_REVERSE);

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

            if (is_cursor || in_sel) wattroff(_win, A_REVERSE);
        }
    }
    wrefresh(_win);
}

// Project overview: every pattern in the current project, how much it holds
// and the chain (song sequence) saved for it.
void draw_project(WINDOW* _win) {
    opz::opz_project_data project = device.getProjectData();
    size_t project_id = device.getActiveProjectId();
    uint8_t pattern_id = device.getActivePatternId();
    uint8_t chain_pos = device.getActiveChainPos();

    int lines, cols;
    getmaxyx(_win, lines, cols);

    werase(_win);
    box(_win, 0, 0);

    // Per-pattern content and chain length
    size_t note_count[16] = {0};
    size_t chain_len[16] = {0};
    size_t used_patterns = 0, chained_patterns = 0;
    for (size_t p = 0; p < 16; p++) {
        for (size_t n = 0; n < 880; n++)
            if (project.pattern[p].note[n].note != 0xFF)
                note_count[p]++;
        for (size_t i = 0; i < 32; i++) {
            if (project.pattern_chain[p].pattern[i] >= 16) break;
            chain_len[p]++;
        }
        if (note_count[p] > 0) used_patterns++;
        if (chain_len[p] > 0) chained_patterns++;
    }

    mvwprintw(_win, 0, 2, " PROJECT %02zu ", project_id + 1);
    mvwprintw(_win, 0, cols - 14, " %2zu/16 USED ", used_patterns);

    mvwprintw(_win, 1, 2, "TEMPO %3i BPM   SWING %3i   PATTERN %02i   CHAINS %zu", project.tempo,
              (int)((int)project.swing / 2.55f) - 50, pattern_id + 1, chained_patterns);

    // One column per pattern
    int label_w = 7;
    int col_w = std::max(3, (cols - label_w - 2) / 16);
    mvwprintw(_win, 3, 2, "PAT");
    mvwprintw(_win, 4, 2, "NOTES");
    mvwprintw(_win, 5, 2, "LEN");
    mvwprintw(_win, 7, 2, "CHAIN");

    int max_ids = std::max(1, lines - 2 - 8);
    for (size_t p = 0; p < 16; p++) {
        int x = 2 + label_w + (int)p * col_w;
        bool is_active = (p == pattern_id);
        bool is_empty = (note_count[p] == 0);

        if (is_empty) wattron(_win, COLOR_PAIR(5));
        else if (is_active) wattron(_win, COLOR_PAIR(1));

        if (is_active) wattron(_win, A_REVERSE);
        mvwprintw(_win, 3, x, "%02zu", p + 1);
        if (is_active) wattroff(_win, A_REVERSE);
        mvwprintw(_win, 4, x, "%-3zu", note_count[p]);
        if (chain_len[p] > 0) mvwprintw(_win, 5, x, "%-3zu", chain_len[p]);
        else                  mvwprintw(_win, 5, x, "-");

        if (is_empty) wattroff(_win, COLOR_PAIR(5));
        else if (is_active) wattroff(_win, COLOR_PAIR(1));

        // Chained patterns, in play order, going down
        size_t shown = std::min(chain_len[p], (size_t)max_ids);
        bool truncated = chain_len[p] > (size_t)max_ids;
        if (truncated) shown = max_ids - 1;
        for (size_t i = 0; i < shown; i++) {
            bool playing = (is_active && chain_pos > 0 && i == (size_t)chain_pos - 1);
            if (playing) wattron(_win, A_REVERSE);
            mvwprintw(_win, 8 + i, x, "%02i", project.pattern_chain[p].pattern[i] + 1);
            if (playing) wattroff(_win, A_REVERSE);
        }
        if (truncated)
            mvwprintw(_win, 8 + shown, x, "+%zu", chain_len[p] - shown);
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

    bool cursor_is_group = mixer_cursor_is_group();
    int cur_group = cursor_is_group ? mixer_cursor_group_index() : -1;
    int cur_track = cursor_is_group ? -1 : mixer_cursor_track_index();

    const char* group_names[4] = {"DRUMS", "SYNTH", "PUNCH", "MASTER"};
    uint8_t group_levels[4] = {project.drum_level, project.synth_level, project.punch_level, project.master_level};
    int group_x[4] = {2, 22, 43, 64};

    for (int g = 0; g < 4; g++) {
        bool is_cursor = (g == cur_group);
        if (is_cursor) wattron(_win, A_REVERSE);
        mvwprintw(_win, 1, group_x[g], "%s", group_names[g]);
        if (is_cursor) wattroff(_win, A_REVERSE);

        wattron(_win, COLOR_PAIR(4));
        mvwprintw(_win, 2, group_x[g], "%s", hBar(7, group_levels[g]).c_str());
        wattroff(_win, COLOR_PAIR(4));
        mvwprintw(_win, 3, group_x[g], "%03i", (int)((int)group_levels[g] / 2.55f));
    }

    if (cur_track >= 0) {
        bool track_muted = device.getMuteTrack(pattern_id, (size_t)cur_track);
        uint8_t track_level = pattern.sound_param[cur_track].level;
        mvwprintw(_win, 4, 2, "TRACK %-7s LEVEL %03i  %s   %s",
                  opz::toString(opz::opz_track_id(cur_track)).c_str(),
                  (int)((int)track_level / 2.55f), hBar(7, track_level).c_str(),
                  track_muted ? "MUTED " : "ACTIVE");
    }
    else {
        mvwprintw(_win, 4, 2, "GROUP %-7s", group_names[cur_group]);
    }
    mvwprintw(_win, lines - 2, 2, "(left/right: track/group  up/down: gain  enter: mute)");

    int mute_col_w = std::max(4, (cols - 4) / 16);
    for (size_t t = 0; t < 16; t++) {
        int x = 2 + (int)t * mute_col_w;
        bool muted = device.getMuteTrack(pattern_id, t);
        const char* short_names[] = {"KI","SN","PE","SA","BA","LE","AR","CH","F1","F2","TP","MA","PF","MO","LI","MT"};

        bool is_cursor = (t == (size_t)cur_track);
        if (is_cursor) wattron(_win, A_REVERSE);

        if (muted) {
            wattron(_win, COLOR_PAIR(5));
            mvwprintw(_win, 6, x, "%s", short_names[t]);
            mvwprintw(_win, 7, x, "--");
            wattroff(_win, COLOR_PAIR(5));
        }
        else {
            if (t < 4) wattron(_win, COLOR_PAIR(3));
            else if (t < 8) wattron(_win, COLOR_PAIR(4));
            else wattron(_win, COLOR_PAIR(2));
            mvwprintw(_win, 6, x, "%s", short_names[t]);
            mvwprintw(_win, 7, x, "##");
            if (t < 4) wattroff(_win, COLOR_PAIR(3));
            else if (t < 8) wattroff(_win, COLOR_PAIR(4));
            else wattroff(_win, COLOR_PAIR(2));
        }

        if (is_cursor) wattroff(_win, A_REVERSE);
    }

    wrefresh(_win);
}

void draw_tempo(WINDOW* _win) {
    int lines, cols;
    getmaxyx(_win, lines, cols);

    opz::opz_project_data project = device.getProjectData();
    double pct = (device.getActiveStepId() % 8) / 8.0;
    
    int prop = tempo_cursor_prop.load();

    werase(_win);
    box(_win, 0, 0);

    if (prop == TEMPO_PROP_BPM) wattron(_win, A_REVERSE);
    mvwprintw(_win, 1, 2, "TEMPO");
    if (prop == TEMPO_PROP_BPM) wattroff(_win, A_REVERSE);
    mvwprintw(_win, 2, 2, "%03i", project.tempo);

    if (prop == TEMPO_PROP_SWING) wattron(_win, A_REVERSE);
    mvwprintw(_win, 1, 22, "SWING");
    if (prop == TEMPO_PROP_SWING) wattroff(_win, A_REVERSE);
    mvwprintw(_win, 2, 22, "%03i", (int)((int)project.swing / 2.55f) - 50);

    if (prop == TEMPO_PROP_METRO_SOUND) wattron(_win, A_REVERSE);
    mvwprintw(_win, 1, cols - 28, "SOUND");
    if (prop == TEMPO_PROP_METRO_SOUND) wattroff(_win, A_REVERSE);
    mvwprintw(_win, 2, cols - 28, "%8s", opz::metronomeSoundString(project.metronome_sound).c_str());

    if (prop == TEMPO_PROP_METRO_LEVEL) wattron(_win, A_REVERSE);
    mvwprintw(_win, 1, cols - 8, "LEVEL");
    if (prop == TEMPO_PROP_METRO_LEVEL) wattroff(_win, A_REVERSE);
    mvwprintw(_win, 2, cols - 8, "%03i", (int)((int)project.metronome_level / 2.55f));

    mvwprintw(_win, lines - 2, 2, "(left/right: property  up/down: value)");

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
