#pragma once

#include <stdio.h>
#include <stdint.h>

#include <vector>
#include <functional>

namespace T3 {

// MIDI Standar
const uint8_t CONTROLLER_CHANGE   = 0xB0;
const uint8_t NOTE_ON             = 0x90;
const uint8_t NOTE_OFF            = 0x80;
const uint8_t KEY_PRESSURE        = 0xA0;
const uint8_t PROGRAM_CHANGE      = 0xC0;
const uint8_t CHANNEL_PRESSURE    = 0xD0;
const uint8_t PITCH_BEND          = 0xE0;
const uint8_t SYSEX_HEAD          = 0xF0;
const uint8_t SONG_POSITION       = 0xF2;
const uint8_t SONG_SELECT         = 0xF3;
const uint8_t TUNE_REQUEST        = 0xF6;
const uint8_t SYSEX_END           = 0xF7;
const uint8_t TIMING_TICK         = 0xF8;
const uint8_t START_SONG          = 0xFA;
const uint8_t CONTINUE_SONG       = 0xFB;
const uint8_t STOP_SONG           = 0xFC;
const uint8_t ACTIVE_SENSING      = 0xFE;
const uint8_t SYSTEM_RESET        = 0xFF;

const uint8_t OPZ_VENDOR_ID[3] = {0x00, 0x20, 0x76};
const uint8_t OPZ_MAX_PROTOCOL_VERSION = 0x01;


enum track_id {
    KICK = 0, SNARE, PERC, SAMPLE, BASS, LEAD, ARP, CHORD, FX1, FX2, TAPE, MASTER, PERFORM, MODULE, LIGHT, MOTION  
};

enum page_id {
    PAGE_ONE = 0, PAGE_TWO, PAGE_TREE, PAGE_FOUR
};

enum pattern_parameter_id {
    KICK_PLUG = 0,  KICK_PLUG1,     KICK_PLUG2,     KICK_PLUG3,     KICK_STEP_COUNT,    KICK_UNKNOWN,   KICK_STEP_LENGTH,   KICK_QUANTIZE,  KICK_NOTE_STYLE,    KICK_NOTE_LENGTH,   KICK_BYTE1,     KICK_BYTE2,
    SNARE_PLUG,     SNARE_PLUG1,    SNARE_PLUG2,    SNARE_PLUG3,    SNARE_STEP_COUNT,   SNARE_UNKNOWN,  SNARE_STEP_LENGTH,  SNARE_QUANTIZE, SNARE_NOTE_STYLE,   SNARE_NOTE_LENGTH,  SNARE_BYTE1,    SNARE_BYTE2,
    PERC_PLUG,      PERC_PLUG1,     PERC_PLUG2,     PERC_PLUG3,     PERC_STEP_COUNT,    PERC_UNKNOWN,   PERC_STEP_LENGTH,   PERC_QUANTIZE,  PERC_NOTE_STYLE,    PERC_NOTE_LENGTH,   PERC_BYTE1,     PERC_BYTE2,
    SAMPLE_PLUG,    SAMPLE_PLUG1,   SAMPLE_PLUG2,   SAMPLE_PLUG3,   SAMPLE_STEP_COUNT,  SAMPLE_UNKNOWN, SAMPLE_STEP_LENGTH, SAMPLE_QUANTIZE, SAMPLE_NOTE_STYLE, SAMPLE_NOTE_LENGTH, SAMPLE_BYTE1,   SAMPLE_BYTE2,
    BASS_PLUG,      BASS_PLUG1,     BASS_PLUG2,     BASS_PLUG3,     BASS_STEP_COUNT,    BASS_UNKNOWN,   BASS_STEP_LENGTH,   BASS_QUANTIZE,  BASS_NOTE_STYLE,    BASS_NOTE_LENGTH,   BASS_BYTE1,     BASS_BYTE2,
    LEAD_PLUG,      LEAD_PLUG1,     LEAD_PLUG2,     LEAD_PLUG3,     LEAD_STEP_COUNT,    LEAD_UNKNOWN,   LEAD_STEP_LENGTH,   LEAD_QUANTIZE,  LEAD_NOTE_STYLE,    LEAD_NOTE_LENGTH,   LEAD_BYTE1,     LEAD_BYTE2,
    ARC_PLUG,       ARC_PLUG1,      ARC_PLUG2,      ARC_PLUG3,      ARC_STEP_COUNT,     ARC_UNKNOWN,    ARC_STEP_LENGTH,    ARC_QUANTIZE,   ARC_NOTE_STYLE,     ARC_NOTE_LENGTH,    ARC_BYTE1,      ARC_BYTE2,
    CHORD_PLUG,     CHORD_PLUG1,    CHORD_PLUG2,    CHORD_PLUG3,    CHORD_STEP_COUNT,   CHORD_UNKNOWN,  CHORD_STEP_LENGTH,  CHORD_QUANTIZE, CHORD_NOTE_STYLE,   CHORD_NOTE_LENGTH,  CHORD_BYTE1,    CHORD_BYTE2,
    FX1_PLUG,       FX1_PLUG1,      FX1_PLUG2,      FX1_PLUG3,      FX1_STEP_COUNT,     FX1_UNKNOWN,    FX1_STEP_LENGTH,    FX1_QUANTIZE,   FX1_NOTE_STYLE,     FX1_NOTE_LENGTH,    FX1_BYTE1,      FX1_BYTE2,
    FX2_PLUG,       FX2_PLUG1,      FX2_PLUG2,      FX2_PLUG3,      FX2_STEP_COUNT,     FX2_UNKNOWN,    FX2_STEP_LENGTH,    FX2_QUANTIZE,   FX2_NOTE_STYLE,     FX2_NOTE_LENGTH,    FX2_BYTE1,      FX2_BYTE2,
    TAPE_PLUG,      TAPE_PLUG1,     TAPE_PLUG2,     TAPE_PLUG3,     TAPE_STEP_COUNT,    TAPE_UNKNOWN,   TAPE_STEP_LENGTH,   TAPE_QUANTIZE,  TAPE_NOTE_STYLE,    TAPE_NOTE_LENGTH,   TAPE_BYTE1,     TAPE_BYTE2,
    MASTER_PLUG,    MASTER_PLUG1,   MASTER_PLUG2,   MASTER_PLUG3,   MASTER_STEP_COUNT,  MASTER_UNKNOWN, MASTER_STEP_LENGTH, MASTER_QUANTIZE,MASTER_NOTE_STYLE,  MASTER_NOTE_LENGTH, MASTER_BYTE1,   MASTER_BYTE2,
    PERFORM_PLUG,   PERFORM_PLUG1,  PERFORM_PLUG2,  PERFORM_PLUG3,  PERFORM_STEP_COUNT, PERFORM_UNKNOWN,PERFORM_STEP_LENGTH,PERFORM_QUANTIZE,PERFORM_NOTE_STYLE,PERFORM_NOTE_LENGTH,PERFORM_BYTE1,  PERFORM_BYTE2,
    MODULE_PLUG,    MODULE_PLUG1,   MODULE_PLUG2,   MODULE_PLUG3,   MODULE_STEP_COUNT,  MODULE_UNKNOWN, MODULE_STEP_LENGTH, MODULE_QUANTIZE,MODULE_NOTE_STYLE,  MODULE_NOTE_LENGTH, MODULE_BYTE1,   MODULE_BYTE2,
    LIGHT_PLUG,     LIGHT_PLUG1,    LIGHT_PLUG2,    LIGHT_PLUG3,    LIGHT_STEP_COUNT,   LIGHT_UNKNOWN,  LIGHT_STEP_LENGTH,  LIGHT_QUANTIZE, LIGHT_NOTE_STYLE,   LIGHT_NOTE_LENGTH,  LIGHT_BYTE1,    LIGHT_BYTE2,
    MOTION_PLUG,    MOTION_PLUG1,   MOTION_PLUG2,   MOTION_PLUG3,   MOTION_STEP_COUNT,  MOTION_UNKNOWN, MOTION_STEP_LENGTH, MOTION_QUANTIZE,MOTION_NOTE_STYLE,  MOTION_NOTE_LENGTH, MOTION_BYTE1,   MOTION_BYTE2
};

enum track_parameter_id {
    SOUND_PARAM1 = 0,   SOUND_PARAM2,   SOUND_FILTER,       SOUND_RESONANCE, 
    ENVELOPE_ATTACK,    ENVELOPE_DECAY, ENVELOPE_SUSTAIN,   ENVELOPE_RELEASE,
    SOUND_FX1,          SOUND_FX2,      SOUND_PAN,          SOUND_LEVEL,
    LFO_DEPTH,          LFO_SPEED,      LFO_VALUE,          LFO_SHAPE,
    NOTE_LENGHT,        NOTE_STYLE,     QUANTIZE,           PORTAMENTO
};

// All but the musical keyboard
enum key_id {
    KEY_POWER = 0,
    KEY_PROJECT, KEY_MIXER, KEY_TEMPO, KEY_SCREEN,
    KEY_TRACK, KEY_KICK, KEY_SNARE, KEY_PERC, KEY_SAMPLE, KEY_BASS, KEY_LEAD, KEY_ARP, KEY_CHORD, KEY_FX1, KEY_FX2, KEY_TAPE, KEY_MASTER, KEY_PERFORM, KEY_MODULE, KEY_LIGHT, KEY_MOTION,
    KEY_RECORD, KEY_PLAY, KEY_STOP,
    KEY_MINUS, KEY_PLUS, KEY_SHIFT
};

typedef struct {
    uint8_t sysex_id;
    uint8_t vendor_id[3];
    uint8_t protocol_version;
    uint8_t parm_id;
} sysex_header, *p_sysex_header;

typedef struct {
    uint8_t bit11 : 1;
    uint8_t bit12 : 1;
    uint8_t bit13 : 1;  
    uint8_t bit14 : 1;
    uint8_t bit15 : 1;
    uint8_t bit16 : 1;
    uint8_t page : 2;

    uint8_t step : 5;
    uint8_t shift : 1;
    uint8_t tempo : 1;
    uint8_t mixer : 1;

    uint8_t bit31 : 1;
    uint8_t bit32 : 1;
    uint8_t bit33 : 1;  
    uint8_t bit34 : 1;
    uint8_t bit35 : 1;
    uint8_t screen : 1;
    uint8_t stop : 1;
    uint8_t record : 1;

    uint8_t track : 1;
    uint8_t project : 1;
    uint8_t bit43 : 1;  
    uint8_t bit44 : 1;
    uint8_t bit45 : 1;
    uint8_t bit46 : 1;
    uint8_t bit47 : 1;
    uint8_t bit48 : 1;
    
} key_state, *p_key_state;

typedef struct {
    int8_t octave;
    uint8_t track;
} track_keyboard, *p_track_keyboard;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#track-parameters
typedef struct {
    uint8_t param1;
    uint8_t param2;
    uint8_t attack;
    uint8_t decay;
    uint8_t sustain;
    uint8_t release;
    uint8_t fx1;    
    uint8_t fx2;
    uint8_t filter;
    uint8_t resonance;
    uint8_t pan;
    uint8_t level;
    uint8_t portamento;
    uint8_t lfo_depth;
    uint8_t lfo_speed;
    uint8_t lfo_value;
    uint8_t lfo_shape;
    uint8_t note_style;
} track_parameter, *p_track_parameter;     

// https://github.com/lrk/z-po-project/wiki/Project-file-format#track-chunk
typedef struct {
    uint32_t plug;
    uint8_t step_count;
    uint8_t unknown1;
    uint8_t step_length;
    uint8_t quantize;
    uint8_t note_style;
    uint8_t note_length;
    uint8_t unused[2];
} track_chunck, *p_track_chunck;

typedef struct {
    uint8_t unknown1[2];
    uint8_t value_type;
    uint8_t unknown2[4];
    uint8_t value;
} track_change, *p_track_change;

typedef struct {
    uint8_t sequence;
    uint8_t unknown1[15];
    uint8_t unknown2;
    uint8_t pattern; // id: project + sequence
    uint8_t unknown3;
    uint8_t project;
} sequence_change, *p_sequence_change;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#note-chunk
typedef struct {
    int32_t duration[4];
    uint8_t note;
    uint8_t velocity;
    int8_t  micro_adjustment;
    uint8_t age;
} note_chunck, *p_note_chunck;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#step-chunk
typedef struct {
    uint16_t    components_bitmask;             // Bit mask on 2 bytes to show which step component are enabled. See below for bitmask values
    uint8_t     components_parameters[16];      // Array of selected parameter value for each step components
    uint8_t     components_locked_values[18];   // Array of current lock value for each tracks parameters
    uint8_t     parameter_mask[18];             // Array of actually enabled parameter lock
} step_chunck, *p_step_chunck;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#pattern-chunk
typedef struct {
    track_chunck    tracks[16];
    note_chunck     notes[880];
    step_chunck     steps[256];
    track_parameter parameters[16];
    uint8_t         mytes[40];      // mute config, tracks are mapped with bitmask
    uint16_t        send_tape;      // Send mapping for Tape track using bitmask
    uint16_t        send_master;    // SendMaster  Send mapping for Master track using bitmask
    uint8_t         active_mute_group;   // Active mute group
    uint8_t         unused[3];
} pattern_chunck, *p_pattern_chunck;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#pattern-chain-chunk
typedef struct {
    uint8_t         patterns[16];   // Array of 32 bytes for the patterns id (from 0 to 15).
} pattern_chain_chunk, *p_pattern_chain_chunk;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#project-file-format
typedef struct {
    uint32_t            file_id;            // seem to be always the same number: 0x00000049
    pattern_chain_chunk pattern_chain[16];  // Array of saved pattern chains
    uint8_t             drum_level;
    uint8_t             synth_level;
    uint8_t             punch_level;
    uint8_t             master_level;
    uint8_t             project_tempo;      // Project tempo from 40 to 200
    uint8_t             unknown1[44];       // unknown, values are often 0x00
    uint8_t             swing;              // Swing level from 0 to 255
    uint8_t             metronome_level;    // Metronome sound level
    uint8_t             metronome_sound;    // Metronome sound selection from 0x00 to 0xFF. Values might be mapped with some linear interpolated indexes 
    uint8_t             unknown2[4];        // unknown, mostly 0x000000FF
    pattern_chunck      patterns[16];
} project, *p_project;

class OPZ {
public:
    OPZ();

    bool            connect();
    void            disconnect();

    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);
    static std::string& toString( key_id _id );
    static std::string& toString( page_id   _id );
    static std::string& toString( track_id  _id );
    static std::string& toString( pattern_parameter_id _id );
    static std::string& toString( track_parameter_id _id);

    void            update();

    void            setKeyCallback(std::function<void(key_id, int)> _callback) { m_key = _callback; m_key_enable = true; }

    static const std::vector<unsigned char>* getInitMsg();
    static const std::vector<unsigned char>* getHeartBeat();

    track_id        getActiveTrack() const { return m_active_track; }
    int             getActiveOctave() const { return m_track_keyboard[m_active_track].octave; }
    page_id         getActivePage() const { return m_active_page; }

    float           getVolume() const { return m_volume; }
    float           getTrackPageParameter(track_id _track, track_parameter_id _prop);

    size_t          verbose;

protected:
    void            process_sysex(std::vector<unsigned char>* _message);
    void            process_event(std::vector<unsigned char>* _message);

    track_keyboard  m_track_keyboard[16];
    track_parameter m_track_parameter[16];
    float           m_note_lenght[16];
    float           m_quantize[16];

    key_state       m_key_state;
    key_state       m_key_prev_state;

    float           m_volume;

    track_id        m_active_track;
    page_id         m_active_page;

    uint8_t         m_active_project;
    uint8_t         m_active_sequence;
    uint8_t         m_active_pattern;


    unsigned char   m_microphone_mode;
    bool            m_play;

    std::function<void(key_id, int)> m_key;
    bool            m_key_enable;
};

}