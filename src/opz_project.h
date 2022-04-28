#pragma once

#include <stdint.h>
#include <string>

namespace T3 {

enum opz_pattern_parameter_id {
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

enum opz_track_id {
    KICK = 0, SNARE, PERC, SAMPLE, BASS, LEAD, ARP, CHORD, FX1, FX2, TAPE, MASTER, PERFORM, MODULE, LIGHT, MOTION  
};

enum opz_sound_parameter_id {
    SOUND_PARAM1 = 0,   SOUND_PARAM2,   SOUND_FILTER,       SOUND_RESONANCE, 
    ENVELOPE_ATTACK,    ENVELOPE_DECAY, ENVELOPE_SUSTAIN,   ENVELOPE_RELEASE,
    SOUND_FX1,          SOUND_FX2,      SOUND_PAN,          SOUND_LEVEL,
    LFO_DEPTH,          LFO_SPEED,      LFO_VALUE,          LFO_SHAPE,
    NOTE_LENGTH,        NOTE_STYLE,     QUANTIZE,           PORTAMENTO,
    STEP_COUNT,         STEP_LENGTH
};

enum opz_note_style_id {
    DRUM_RETRIG = 0,    DRUM_MONO,  DRUM_GATE,      DRUM_LOOP,
    SYNTH_POLY,         SYNTH_MONO, SYNTH_LEGATO     
};

const size_t opz_notes_per_track[] = {    2, 2, 2, 2, 4,  4,  8,  4,  1,  1,  1,  4,  6,  6, 4,   4 };
const size_t opz_notes_offset_track[] = { 0, 2, 4, 6, 8, 12, 16, 24, 28, 29, 30, 31, 35, 41, 47, 51 };

// https://github.com/lrk/z-po-project/wiki/Project-file-format#track-parameters
typedef struct {
    uint8_t     param1;
    uint8_t     param2;
    uint8_t     attack;
    uint8_t     decay;
    uint8_t     sustain;
    uint8_t     release;
    uint8_t     fx1;    
    uint8_t     fx2;
    uint8_t     filter;
    uint8_t     resonance;
    uint8_t     pan;
    uint8_t     level;
    uint8_t     portamento;
    uint8_t     lfo_depth;
    uint8_t     lfo_speed;
    uint8_t     lfo_value;
    uint8_t     lfo_shape;
    uint8_t     note_style;
} opz_sound_parameter, *p_opz_sound_parameter;     

// https://github.com/lrk/z-po-project/wiki/Project-file-format#track-chunk
typedef struct {
    uint32_t    plug;
    uint8_t     step_count;
    uint8_t     unknown1;
    uint8_t     step_length;
    uint8_t     quantize;
    uint8_t     note_style;
    uint8_t     note_length;
    uint8_t     unknown2[2];
} opz_track_parameter, *p_opz_track_parameter;


// https://github.com/lrk/z-po-project/wiki/Project-file-format#note-chunk
typedef struct {
    int32_t duration;
    uint8_t note;
    uint8_t velocity;
    int8_t  micro_adjustment;
    uint8_t age;
} opz_note, *p_opz_note;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#step-chunk
typedef struct {
    uint16_t    components_bitmask;             // Bit mask on 2 bytes to show which step component are enabled. See below for bitmask values
    uint8_t     components_parameters[16];      // Array of selected parameter value for each step components
    uint8_t     components_locked_values[18];   // Array of current lock value for each tracks parameters
    uint8_t     parameter_mask[18];             // Array of actually enabled parameter lock
} opz_step, *p_opz_step;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#pattern-chunk
typedef struct {
    opz_track_parameter track_param[16];
    opz_note            note[880];
    opz_step            step[256];
    opz_sound_parameter page_param[16];
    uint8_t             mute[40];          // mute config, tracks are mapped with bitmask
    uint16_t            send_tape;          // Send mapping for Tape track using bitmask
    uint16_t            send_master;        // SendMaster  Send mapping for Master track using bitmask
    uint8_t             active_mute_group;  // Active mute group
    uint8_t             unknown[3];
} opz_pattern, *p_opz_pattern;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#pattern-chain-chunk
typedef struct {
    uint8_t         pattern[32];   // Array of 32 bytes for the patterns id (from 0 to 15).
} opz_pattern_chain, *p_opz_pattern_chain;

// https://github.com/lrk/z-po-project/wiki/Project-file-format#project-file-format
typedef struct {
    // uint32_t            file_id;            // seem to be always the same number: 0x00000049
    opz_pattern_chain   pattern_chain[16];  // Array of saved pattern chains
    uint8_t             drum_level;
    uint8_t             synth_level;
    uint8_t             punch_level;
    uint8_t             master_level;
    uint8_t             tempo;              // Project tempo from 40 to 200
    uint8_t             unknown1[44];       // unknown, values are often 0x00
    uint8_t             swing;              // Swing level from 0 to 255
    uint8_t             metronome_level;    // Metronome sound level
    uint8_t             metronome_sound;    // Metronome sound selection from 0x00 to 0xFF. Values might be mapped with some linear interpolated indexes 
    uint32_t            unknown2;           // unknown, mostly 0x000000FF
    opz_pattern         pattern[16];
} opz_project_data, *p_opz_project_data;

std::string& toString( opz_track_id  _id );
std::string& toString( opz_note_style_id  _id );
std::string& toString( opz_sound_parameter_id _id);
std::string& toString( opz_pattern_parameter_id _id );

class opz_project {
public:
    opz_project();    
    virtual const opz_project_data&     getProjectData() const { return m_project; }

    virtual const opz_pattern&          getPattern(size_t _id) const { return m_project.pattern[_id]; }

    virtual size_t                      getNoteIdOffset(size_t _track, size_t _step) { return _step * 55 + opz_notes_offset_track[_track]; }
    virtual size_t                      getNotesPerTrack(size_t _track) { return opz_notes_per_track[_track]; }

    virtual const opz_track_parameter&  getTrackParameters(uint8_t patterm, opz_track_id _track) const { return m_project.pattern[patterm].track_param[(size_t)_track]; }
    virtual const opz_sound_parameter&  getSoundParameters(uint8_t patterm, opz_track_id _track) const { return m_project.pattern[patterm].page_param[(size_t)_track]; };
    virtual float                       getSoundParameter(uint8_t patterm, opz_track_id _track, opz_sound_parameter_id _prop) const;

protected:
    opz_project_data    m_project;
};

};