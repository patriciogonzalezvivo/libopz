#pragma once

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "opz_file.h"

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
enum opz_event_id {
    VOLUME_CHANGE = 0,
    KEY_PROJECT, KEY_MIXER, KEY_TEMPO, KEY_SCREEN,
    KEY_TRACK, KEY_KICK, KEY_SNARE, KEY_PERC, KEY_SAMPLE, KEY_BASS, KEY_LEAD, KEY_ARP, KEY_CHORD, KEY_FX1, KEY_FX2, KEY_TAPE, KEY_MASTER, KEY_PERFORM, KEY_MODULE, KEY_LIGHT, KEY_MOTION,
    KEY_RECORD, PLAY_CHANGE, KEY_STOP,
    OCTAVE_CHANGE, KEY_SHIFT,
    PROJECT_CHANGE, PATTERN_CHANGE, TRACK_CHANGE, PAGE_CHANGE, 
    MICROPHONE_MODE_CHANGE, MICROPHONE_LEVEL_CHANGE, MICROPHONE_FX_CHANGE,
    PARAMETER_CHANGE, 
    PATTERN_PACKAGE_RECIVED, PATTERN_DOWNLOADED,
    NO_CONNECTION
};

enum opz_project_parameter_id {
    DRUM_LEVEL = 0, SYNTH_LEVEL,    PUNCH_LEVEL,    MASTER_LEVEL,   PROJECT_TEMPO,
    SWING, 
    // 44 unknown  
    METRONOME_LEVEL,    METRONOME_SOUND
    // 4 unknown
};

enum opz_page_id {
    PAGE_ONE = 0, PAGE_TWO, PAGE_TREE, PAGE_FOUR
};

enum opz_mic_fx_id {
    MIC_NO_FX = 0, MIC_FX_1, MIC_FX_2, MIC_FX_1_AND_2
};


typedef struct {
    uint8_t sysex_id;
    uint8_t vendor_id[3];
    uint8_t protocol_version;
    uint8_t parm_id;
} opz_sysex_header, *p_opz_sysex_header;

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
    
} opz_key_state, *p_key_state;

typedef struct {
    uint8_t pattern;
    uint8_t unknown1[16];
    uint8_t address; // id: project + pattern
    uint8_t unknown2;
    uint8_t project;
} opz_sequence_change, *p_opz_sequence_change;

std::string  toString( midi_id    _id );
std::string& toString( opz_page_id   _id );
std::string& toString( opz_event_id  _id );
std::string& toString( opz_mic_fx_id _id );

const std::vector<unsigned char>* opz_init_msg();
const std::vector<unsigned char>* opz_heartbeat();
const std::vector<unsigned char>* opz_config_cmd();
std::vector<unsigned char>        opz_confirm_package_cmd(unsigned char _id);

class opz_device : public opz_file {
public:
    opz_device();

    void            process_message(unsigned char *_message, size_t _length);

    void            setEventCallback(std::function<void(opz_event_id, int)> _callback) { m_event = _callback; m_event_enable = true; }
    void            setMidiCallback(std::function<void(midi_id, size_t, size_t, size_t)> _callback) { m_midi = _callback; m_midi_enable = true; }

    uint8_t         getActiveProject() const { return m_active_project; }
    uint8_t         getActivePattern() const { return m_active_pattern; }
    opz_track_id    getActiveTrack() const { return m_active_track; }
    opz_page_id     getActivePage() const { return m_active_page; }
    uint8_t         getActiveStep() const { return m_active_step; };
    int             getActiveOctave() const { return m_octave[(size_t)m_active_track]; }

    float           getLevel() const { return m_level; }
    float           getMicLevel() const { return m_mic_level; }
    opz_mic_fx_id   getMicFx() const { return m_mic_fx; }
    uint8_t         getMicMode() const { return m_mic_mode; }

    float           getTrackPageParameter(opz_track_id _track, opz_page_parameter_id _prop) const {  return opz_file::getTrackPageParameter(m_active_pattern, _track, _prop); };
    float           getActivePageParameter(opz_page_parameter_id _prop) const { return getTrackPageParameter(m_active_track, _prop); }

    const opz_track_parameter&  getTrackParameters(opz_track_id _track) const { return opz_file::getTrackParameters( m_active_pattern, _track); }
    const opz_track_parameter&  getActiveTrackParameters() const { return getTrackParameters(m_active_track); }
    
    const opz_page_parameter&  getTrackPageParameters(opz_track_id _track) const { return opz_file::getTrackPageParameters(m_active_pattern, _track); };
    const opz_page_parameter&  getActiveTrackPageParameters() const { return getTrackPageParameters(m_active_track); };
    
    const bool&     isTrackMute(opz_track_id _id) const { return m_mutes[_id]; }
    bool            isPlaying() const { return m_play; }

    size_t          verbose;    // 0 off
                                // 1 event description
                                // 2 event HEX messages
                                // 3 interpreted data 
                                // 4 interpreted data without hex

protected:
    void                process_sysex(unsigned char *_message, size_t _length);
    void                process_event(unsigned char *_message, size_t _length);

    std::vector<unsigned char> m_packets;
    
    // Project Data
    int8_t              m_octave[16];
    bool                m_mutes[16];

    // Active states
    uint8_t             m_active_project;
    uint8_t             m_active_pattern;
    opz_track_id        m_active_track;
    opz_page_id         m_active_page;
    uint8_t             m_active_step;

    // Non-musical key states 
    opz_key_state       m_key_state;
    opz_key_state       m_key_prev_state;

    // non-project or pattern related states 
    float               m_level;
    float               m_mic_level;
    uint8_t             m_mic_mode;
    opz_mic_fx_id       m_mic_fx;
    bool                m_play;

    // CALLBACKS
    std::function<void(opz_event_id, int)> m_event;
    bool                m_event_enable;

    std::function<void(midi_id, size_t, size_t, size_t)> m_midi;
    bool                m_midi_enable;
};

}