#pragma once

#include <stdio.h>
#include <stdint.h>

#include <vector>
#include <functional>

namespace T3 {

// Standard MIDI events
enum midi_id {
    NOTE_OFF            = 0x80, NOTE_ON             = 0x90, KEY_PRESSURE        = 0xA0, 
    CONTROLLER_CHANGE   = 0xB0, PROGRAM_CHANGE      = 0xC0, CHANNEL_PRESSURE    = 0xD0, PITCH_BEND          = 0xE0,
    SYSEX_HEAD          = 0xF0, SONG_POSITION       = 0xF2, SONG_SELECT         = 0xF3, TUNE_REQUEST        = 0xF6, SYSEX_END           = 0xF7, 
    TIMING_TICK         = 0xF8, START_SONG          = 0xFA, CONTINUE_SONG       = 0xFB, STOP_SONG           = 0xFC, ACTIVE_SENSING      = 0xFE, SYSTEM_RESET        = 0xFF
};

const uint8_t OPZ_VENDOR_ID[3] = {0x00, 0x20, 0x76};
const uint8_t OPZ_MAX_PROTOCOL_VERSION = 0x01;


// Non-musical keyboard events
enum event_id {
    VOLUME_CHANGE = 0,
    KEY_PROJECT, KEY_MIXER, KEY_TEMPO, KEY_SCREEN,
    KEY_TRACK, KEY_KICK, KEY_SNARE, KEY_PERC, KEY_SAMPLE, KEY_BASS, KEY_LEAD, KEY_ARP, KEY_CHORD, KEY_FX1, KEY_FX2, KEY_TAPE, KEY_MASTER, KEY_PERFORM, KEY_MODULE, KEY_LIGHT, KEY_MOTION,
    KEY_RECORD, PLAY_CHANGE, KEY_STOP,
    OCTAVE_CHANGE, KEY_SHIFT,
    PROJECT_CHANGE, PATTERN_CHANGE, TRACK_CHANGE, PAGE_CHANGE, 
    MICROPHONE_MODE_CHANGE, MICROPHONE_LEVEL_CHANGE, MICROPHONE_FX_CHANGE,
    PARAMETER_CHANGE
};


enum project_parameter_id {
    DRUM_LEVEL = 0, SYNTH_LEVEL,    PUNCH_LEVEL,    MASTER_LEVEL,   PROJECT_TEMPO,
    SWING, // 44 unknown  
    METRONOME_LEVEL,    METRONOME_SOUND
    // 4 unknown
};

enum pattern_page_parameter_id {
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

enum track_id {
    KICK = 0, SNARE, PERC, SAMPLE, BASS, LEAD, ARP, CHORD, FX1, FX2, TAPE, MASTER, PERFORM, MODULE, LIGHT, MOTION  
};

enum page_parameter_id {
    SOUND_PARAM1 = 0,   SOUND_PARAM2,   SOUND_FILTER,       SOUND_RESONANCE, 
    ENVELOPE_ATTACK,    ENVELOPE_DECAY, ENVELOPE_SUSTAIN,   ENVELOPE_RELEASE,
    SOUND_FX1,          SOUND_FX2,      SOUND_PAN,          SOUND_LEVEL,
    LFO_DEPTH,          LFO_SPEED,      LFO_VALUE,          LFO_SHAPE,
    NOTE_LENGTH,        NOTE_STYLE,     QUANTIZE,           PORTAMENTO,
    STEP_COUNT,         STEP_LENGTH
};

enum page_id {
    PAGE_ONE = 0, PAGE_TWO, PAGE_TREE, PAGE_FOUR
};

enum mic_fx_id {
    MIC_NO_FX = 0, MIC_FX_1, MIC_FX_2, MIC_FX_1_AND_2
};

enum note_style_id {
    DRUM_RETRIG = 0,    DRUM_MONO,  DRUM_GATE,      DRUM_LOOP,
    SYNTH_POLY,         SYNTH_MONO, SYNTH_LEGATO     
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
} page_parameter, *p_page_parameter;     

// https://github.com/lrk/z-po-project/wiki/Project-file-format#track-chunk
typedef struct {
    uint32_t plug;
    uint8_t step_count;
    uint8_t unknown1;
    uint8_t step_length;
    uint8_t quantize;
    uint8_t note_style;
    uint8_t note_length;
    uint8_t unknown2[2];
} track_parameter, *p_track_parameter;

typedef struct {
    uint8_t unknown1[2];
    uint8_t value_type;
    uint8_t unknown2[4];
    uint8_t value;
} track_change, *p_track_change;

typedef struct {
    uint8_t pattern;
    uint8_t unknown1[16];
    uint8_t address; // id: project + pattern
    uint8_t unknown2;
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
    track_parameter track_param[16];
    note_chunck     note[880];
    step_chunck     step[256];
    page_parameter  page_param[16];
    uint8_t         mute[40];          // mute config, tracks are mapped with bitmask
    uint16_t        send_tape;          // Send mapping for Tape track using bitmask
    uint16_t        send_master;        // SendMaster  Send mapping for Master track using bitmask
    uint8_t         active_mute_group;  // Active mute group
    uint8_t         unknown[3];
} pattern_chunck, *p_pattern_chunck;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#pattern-chain-chunk
typedef struct {
    uint8_t         pattern[32];   // Array of 32 bytes for the patterns id (from 0 to 15).
} pattern_chain_chunk, *p_pattern_chain_chunk;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#project-file-format
typedef struct {
    // uint32_t            file_id;            // seem to be always the same number: 0x00000049
    pattern_chain_chunk pattern_chain[16];  // Array of saved pattern chains
    uint8_t             drum_level;
    uint8_t             synth_level;
    uint8_t             punch_level;
    uint8_t             master_level;
    uint8_t             tempo;              // Project tempo from 40 to 200
    uint8_t             unknown1[44];       // unknown, values are often 0x00
    uint8_t             swing;              // Swing level from 0 to 255
    uint8_t             metronome_level;    // Metronome sound level
    uint8_t             metronome_sound;    // Metronome sound selection from 0x00 to 0xFF. Values might be mapped with some linear interpolated indexes 
    uint8_t             unknown2[4];        // unknown, mostly 0x000000FF
    pattern_chunck      pattern[16];
} project, *p_project;

class OPZ {
public:
    OPZ();

    static std::string toString( midi_id    _id );
    static std::string& toString( event_id  _id );
    static std::string& toString( page_id   _id );
    static std::string& toString( mic_fx_id _id );
    static std::string& toString( track_id  _id );
    static std::string& toString( note_style_id  _id );
    static std::string& toString( pattern_page_parameter_id _id );
    static std::string& toString( page_parameter_id _id);

    static const std::vector<unsigned char>* getInitMsg();
    static const std::vector<unsigned char>* getHeartBeat();
    static const std::vector<unsigned char>* getConfigCmd();
    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);

    void            setEventCallback(std::function<void(event_id, int)> _callback) { m_event = _callback; m_event_enable = true; }
    void            setMidiCallback(std::function<void(midi_id, size_t, size_t, size_t)> _callback) { m_midi = _callback; m_midi_enable = true; }

    float           getLevel() const { return m_level; }
    float           getMicLevel() const { return m_mic_level; }
    mic_fx_id       getMicFx() const { return m_mic_fx; }
    uint8_t         getMicMode() const { return m_mic_mode; }

    const project&  getProject() const { return m_project; }


    uint8_t         getActiveProject() const { return m_active_project; }
    uint8_t         getActivePattern() const { return m_active_pattern; }
    track_id        getActiveTrack() const { return m_active_track; }

    const track_parameter&  getTrackParameters(uint8_t patterm, track_id _track) const { return m_project.pattern[patterm].track_param[(size_t)_track]; }
    const track_parameter&  getTrackParameters(track_id _track) const { return m_project.pattern[m_active_pattern].track_param[(size_t)_track]; }
    const track_parameter&  getActiveTrackParameters() const { return m_project.pattern[m_active_pattern].track_param[(size_t)m_active_track]; }
    
    const page_parameter&  getTrackPageParameters(uint8_t patterm, track_id _track) const { return m_project.pattern[patterm].page_param[_track]; };
    const page_parameter&  getTrackPageParameters(track_id _track) const { return m_project.pattern[m_active_pattern].page_param[_track]; };
    const page_parameter&  getActiveTrackPageParameters() const { return m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track]; };
    
    float           getTrackPageParameter(uint8_t patterm, track_id _track, page_parameter_id _prop) const;
    float           getTrackPageParameter(track_id _track, page_parameter_id _prop) const {  return getTrackPageParameter(m_active_pattern, _track, _prop); };
    float           getActivePageParameter(page_parameter_id _prop) const { return getTrackPageParameter(m_active_pattern, m_active_track, _prop); }

    int             getActiveOctave() const { return m_octave[(size_t)m_active_track]; }
    page_id         getActivePage() const { return m_active_page; }
    uint8_t         getActiveStep() const { return m_active_step; };

    bool            isPlaying() const { return m_play; }

    size_t          verbose;    // 0 off
                                // 1 event description
                                // 2 event HEX messages
                                // 3 interpreted data 
                                // 4 interpreted data without hex

protected:
    void                process_sysex(std::vector<unsigned char>* _message);
    void                process_event(std::vector<unsigned char>* _message);

    // Project Data
    project             m_project;
    int8_t              m_octave[16];

    // Active states
    // uint8_t             m_active_address;
    uint8_t             m_active_project;
    uint8_t             m_active_pattern;
    track_id            m_active_track;
    page_id             m_active_page;
    uint8_t             m_active_step;

    // Non-musical key states 
    key_state           m_key_state;
    key_state           m_key_prev_state;

    // non-project or pattern related states 
    float               m_level;
    float               m_mic_level;
    uint8_t             m_mic_mode;
    mic_fx_id           m_mic_fx;
    bool                m_play;

    // CALLBACKS
    std::function<void(event_id, int)> m_event;
    bool                m_event_enable;

    std::function<void(midi_id, size_t, size_t, size_t)> m_midi;
    bool                m_midi_enable;
};

}