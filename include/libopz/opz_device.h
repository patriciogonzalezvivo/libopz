#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <functional>

#include "opz_project.h"

namespace opz {

    // Standard MIDI events
    enum midi_id {
        NOTE_OFF = 0x80,    NOTE_ON = 0x90,         KEY_PRESSURE = 0xA0,    CONTROLLER_CHANGE = 0xB0,   PROGRAM_CHANGE = 0xC0,  CHANNEL_PRESSURE = 0xD0,    PITCH_BEND = 0xE0,
        SYSEX_HEAD = 0xF0,  SONG_POSITION = 0xF2,   SONG_SELECT = 0xF3,     TUNE_REQUEST = 0xF6,        SYSEX_END = 0xF7,
        TIMING_TICK = 0xF8, START_SONG = 0xFA,      CONTINUE_SONG = 0xFB,   STOP_SONG = 0xFC,
        ACTIVE_SENSING = 0xFE,  SYSTEM_RESET = 0xFF
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
        PROJECT_CHANGE, PATTERN_CHANGE, TRACK_CHANGE, PAGE_CHANGE, SEQUENCE_CHANGE, MUTE_CHANGE, STEP_CHANGE, STEP_ZERO,
        MICROPHONE_MODE_CHANGE, MICROPHONE_LEVEL_CHANGE, MICROPHONE_FX_CHANGE,
        TRACK_PARAMETER_CHANGE, SOUND_PARAMETER_CHANGE,
        PATTERN_PACKAGE_RECIVED, PATTERN_DOWNLOADED,
        NO_CONNECTION
    };

    enum opz_page_id {      PAGE_ONE = 0,   PAGE_TWO,   PAGE_TREE,  PAGE_FOUR };
    enum opz_mic_fx_id {    MIC_NO_FX = 0,  MIC_FX_1,   MIC_FX_2,   MIC_FX_1_AND_2   };

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

    std::string toString(midi_id _id);
    std::string &toString(opz_page_id _id);
    std::string &toString(opz_event_id _id);
    std::string &toString(opz_mic_fx_id _id);

    const std::vector<unsigned char> *opz_init_msg();
    const std::vector<unsigned char> *opz_heartbeat();
    const std::vector<unsigned char> *opz_config_cmd();
    std::vector<unsigned char> opz_confirm_package_cmd(uint8_t *_data, size_t _length);

    class opz_device : public opz_project{
    public:
        opz_device();

        void process_message(unsigned char *_message, size_t _length);

        void setEventCallback(std::function<void(opz_event_id, int)> _callback) {
            m_event = _callback;
            m_event_enable = true;
        }
        void setMidiCallback(std::function<void(midi_id, size_t, size_t, size_t)> _callback) {
            m_midi = _callback;
            m_midi_enable = true;
        }
        // Raw inbound SysEx/MIDI bytes, before any parsing (useful for capture/replay)
        void setRawCallback(std::function<void(unsigned char*, size_t)> _callback) {
            m_raw = _callback;
            m_raw_enable = true;
        }

        virtual bool                isPlaying() const { return m_play; }

        virtual float               getLevel() const { return m_level; }
        virtual float               getMicLevel() const { return m_mic_level; }
        virtual opz_mic_fx_id       getMicFx() const { return m_mic_fx; }
        virtual uint8_t             getMicMode() const { return m_mic_mode; }

        virtual bool                isTrackMute(opz_track_id _id) const { return getMuteTrack(m_active_pattern, (size_t)_id); }

        virtual const opz_midi_config& getMidiConfig() const { return m_midi_config; }
        virtual bool                hasMidiConfig() const { return m_has_midi_config; }

        virtual const opz_mixer_state& getMixerState() const { return m_mixer_state; }
        virtual bool                hasMixerState() const { return m_has_mixer_state; }

        virtual void                setActivePatternId(size_t _id) { m_active_pattern = _id; }
        virtual void                setActiveTrackId(opz_track_id _id) { m_active_track = _id; }

        // Overwrite the in-memory 16-pattern bank. Use right after a confirmed
        // sendPattern() push so the local model (and any display reading it) reflects
        // what was just written, without waiting for the device to send a fresh dump.
        void                        loadPatternBank(const opz_pattern* _bank16) { memcpy(&m_project.pattern[0], _bank16, sizeof(opz_pattern) * 16); }

        virtual uint8_t             getActiveProjectId() const { return m_active_project; }
        virtual uint8_t             getActivePatternId() const { return m_active_pattern; }
        virtual opz_track_id        getActiveTrackId() const { return m_active_track; }
        virtual opz_page_id         getActivePageId() const { return m_active_page; }
        size_t                      getActiveStepId() const { return m_active_step; }
        uint8_t                     getActiveChainPos() const { return m_active_chain_pos; }

        virtual const opz_pattern&  getActivePattern() const { return getPattern(m_active_pattern); }

        virtual float               getTrackPageParameter(opz_track_id _track, opz_sound_parameter_id _prop) const { return opz_project::getSoundParameter(m_active_pattern, _track, _prop); };
        virtual float               getActivePageParameter(opz_sound_parameter_id _prop) const { return getTrackPageParameter(m_active_track, _prop); }

        const opz_track_parameter&  getTrackParameters(opz_track_id _track) const { return opz_project::getTrackParameters(m_active_pattern, _track); }
        const opz_track_parameter&  getActiveTrackParameters() const { return getTrackParameters(m_active_track); }

        const opz_sound_parameter&  getSoundParameters(opz_track_id _track) const { return opz_project::getSoundParameters(m_active_pattern, _track); };
        const opz_sound_parameter&  getActivePageParameters() const { return getSoundParameters(m_active_track); };

        // Pattern-transfer fields captured from the last received dump
        uint16_t                    getPatternId() const { return m_pattern_id; }
        uint8_t                     getPatternAddress() const { return m_pattern_address; }
        uint32_t                    getAckCount() const { return m_ack_count; }

        // Last-seen 0x02 change-record counter. The device increments this per Track
        // Setting it emits (telemetry only, inbound 0x02 writes are not applied by the
        // device, so this is for observation, not for crafting writes).
        uint8_t                     getCounter() const { return m_counter; }

        // Count of full-bank dumps (0x0a) applied to m_project. Increments each time a
        // fresh dump lands; used by requestPatternSync() to detect a completed sync.
        uint32_t                    getDumpCount() const { return m_dump_count; }

        size_t verbose; // 0 off
                        // 1 event description
                        // 2 event HEX messages
                        // 3 interpreted data
                        // 4 interpreted data without hex

    protected:
        void process_sysex(unsigned char *_message, size_t _length);
        void process_event(unsigned char *_message, size_t _length);
        
        void process_track_change(unsigned char *_data, size_t _length);

        std::vector<unsigned char> m_packets;

        // Project Data
        int8_t          m_octave[16];

        // Active states (IDs)
        uint8_t         m_active_project;
        uint8_t         m_active_pattern;
        opz_track_id    m_active_track;
        opz_page_id     m_active_page;
        size_t          m_active_step;
        opz_note        m_active_note;

        // Non-musical key states
        opz_key_state   m_key_state;
        opz_key_state   m_key_prev_state;
        uint8_t         m_active_chain_pos;

        // non-project or pattern related states
        float           m_level;
        float           m_mic_level;
        uint8_t         m_mic_mode;
        opz_mic_fx_id   m_mic_fx;

        uint8_t         m_counter;
        bool            m_counter_valid;    // false until the first 0x02 counter is seen
        bool            m_play;

        opz_midi_config m_midi_config;
        bool            m_has_midi_config;

        opz_mixer_state m_mixer_state;
        bool            m_has_mixer_state;

        // Pattern transfer bookkeeping (0x09/0x0a/0x0b)
        uint16_t        m_pattern_id;       // header field data[2..3] from last dump
        uint8_t         m_pattern_address;  // header field data[0] (proj<<4|pattern)
        uint32_t        m_ack_count;        // count of 0x0b ACKs received
        uint32_t        m_dump_count;       // count of full-bank dumps (0x0a) applied

        // CALLBACKS
        bool m_packet_recived_enabled;
        std::function<void(uint8_t, uint8_t *, size_t)> m_packet_recived;

        std::function<void(opz_event_id, int)> m_event;
        bool m_event_enable;

        std::function<void(midi_id, size_t, size_t, size_t)> m_midi;
        bool m_midi_enable;

        std::function<void(unsigned char*, size_t)> m_raw;
        bool m_raw_enable;
    };

}