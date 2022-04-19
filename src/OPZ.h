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

enum track {
    KICK = 0, SNARE, PERC, SAMPLE, BASS, LEAD, ARP, CHORD, FX1, FX2, TAPE, MASTER, PERFORM, MODULE, LIGHT, MOTION  
};

enum page {
    PAGE_ONE = 0, PAGE_TWO, PAGE_TREE, PAGE_FOUR
};

enum sound_prop {
    SOUND_PARAM1 = 0,   SOUND_PARAM2,   SOUND_FILTER,       SOUND_RESONANCE, 
    ENVELOPE_ATTACK,    ENVELOPE_DECAY, ENVELOPE_SUSTAIN,   ENVELOPE_RELEASE,
    SOUND_FX1,          SOUND_FX2,      SOUND_PAN,          SOUND_LEVEL,
    LFO_DEPTH,          LFO_SPEED,      LFO_VALUE,          LFO_SHAPE,
    NOTE_LENGHT,        QUANTIZE,       PORTAMENTO
};

// All but the musical keyboard
enum key_id {
    KEY_POWER = 0,
    KEY_PROJECT, KEY_MIXER, KEY_TEMPO, KEY_SCREEN,
    KEY_TRACK, KEY_KICK, KEY_SNARE, KEY_PERC, KEY_SAMPLE, KEY_BASS, KEY_LEAD, KEY_ARP, KEY_CHORD, KEY_FX1, KEY_FX2, KEY_TAPE, KEY_MASTER, KEY_PERFORM, KEY_MODULE, KEY_LIGHT, KEY_MOTION,
    KEY_RECORD, KEY_PLAY, KEY_STOP,
    KEY_MINUS, KEY_PLUS, KEY_SHIFT
};

#pragma pack(1)
typedef struct {
    uint8_t sysex_id;
    uint8_t vendor_id[3];
    uint8_t protocol_version;
    uint8_t parm_id;
} sysex_header, *p_sysex_header;

#pragma pack(1)
typedef struct {
    uint8_t parm_id;
    uint8_t value_p1;
    uint8_t value_p2;
    uint8_t track;
} track_state, *p_track_state;

#pragma pack(1)
typedef struct {
    uint8_t parm_id;
} encoder_info, *p_encoder_info;


#pragma pack(1)
typedef struct {
    uint8_t parm_id;    // 5 bit  
                        // 6 bit
    uint8_t byte11 : 1;
    uint8_t param1_hf : 1;    // param1 half flag
    uint8_t param2_hf : 1;    // param2 half flag
    uint8_t attack_hf : 1;    // attack half flag

    uint8_t decay_hf : 1;     // decay half flag
    uint8_t sustain_hf : 1;   // sustain half flag
    uint8_t release_hf : 1;   // release half flag
    uint8_t byte18 : 1;

    uint8_t track;      // 7 bit
    uint8_t param1;     // 8 bit
    uint8_t param2;     // 9 bit
    uint8_t attack;     // 10 bit
    uint8_t decay;      // 11 bit
    uint8_t sustain;    // 12 bit
    uint8_t release;    // 13 bit
                        // 16 bit
    uint8_t fx1_hf : 1;     // fx1 half flag
    uint8_t fx2_hf : 1;     // fx2 half flag
    uint8_t filter_hf : 1;  // filter half flag
    uint8_t resonance_hf : 1; // resonance half flag

    uint8_t pan_hf : 1;     // pan half flag
    uint8_t level_hf : 1;   // level half flag
    uint8_t portamento_hf : 1;  // portamento half flag
    uint8_t byte28 : 1;

    uint8_t fx1;        // 14 bit
    uint8_t fx2;        // 15 bit

    uint8_t filter;     // 17 bit
    uint8_t resonance;  // 18 bit
    uint8_t pan;        // 19 bit
    uint8_t level;      // 20 bit
    uint8_t portamento; // 21 bit
                        // 22 bit
    uint8_t lfo_depth_hf : 1;   // LFO depth half flag   
    uint8_t lfo_speed_hf : 1;   // LFO speed half flag
    uint8_t lfo_value_hf : 1;   // LFO value half flag
    uint8_t lfo_shape_hf : 1;   // LFO shape half flag

    uint8_t note_style_hf : 1;
    uint8_t byte36 : 1;
    uint8_t byte37 : 1;
    uint8_t byte38 : 1;

    uint8_t lfo_depth;  // 23
    uint8_t lfo_speed;  // 24
    uint8_t lfo_value;  // 25
    uint8_t lfo_shape;  // 26

    uint8_t note_style; // 27

} sound_state,        *p_sound_state;     

#pragma pack(1)
typedef struct {
    uint8_t parm_id;
    
    uint8_t page2x : 1;
    uint8_t mixer : 1;
    uint8_t record : 1;
    uint8_t key14 : 1;  
    
    uint8_t key15 : 1;
    uint8_t key16 : 1;
    uint8_t key17 : 1;
    uint8_t key18 : 1;
    
    uint8_t key21 : 1;
    uint8_t key22 : 1;
    uint8_t key23 : 1;
    uint8_t key24 : 1;
    
    uint8_t key25 : 1;
    uint8_t key26 : 1;
    uint8_t page : 1;
    uint8_t key28 : 1;
    
    uint8_t step : 5;

    uint8_t shift : 1;
    uint8_t tempo : 1;
    uint8_t key38 : 1;
    
    uint8_t key41 : 1;
    uint8_t key42 : 1;
    uint8_t key43 : 1;
    uint8_t key44 : 1;
    
    uint8_t key45 : 1;
    uint8_t screen : 1;
    uint8_t stop : 1;
    uint8_t key48 : 1; 
    
    uint8_t track : 1;
    uint8_t project : 1;
    uint8_t key53 : 1;
    uint8_t key54 : 1;
    
    uint8_t key55 : 1;
    uint8_t key56 : 1;
    uint8_t key57 : 1;
    uint8_t key58 : 1;
    
} key_state, *p_key_state;

class OPZ {
public:
    OPZ();

    bool            connect();
    void            disconnect();

    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);
    static std::string& toString( key_id _id );
    static std::string& toString( track  _id );
    static std::string& toString( page   _id );

    void            update();

    void            setKeyCallback(std::function<void(key_id, int)> _callback) { m_key = _callback; m_key_enable = true; }

    static const std::vector<unsigned char>* getInitMsg();
    static const std::vector<unsigned char>* getHeartBeat();

    track           getSelectedTrack() const { return m_track; }
    page            getSelectedPage() const { return m_page; }

    float           getVolume() const { return m_volume; }
    float           getSoundProperty(track _track, sound_prop _prop);

    size_t          verbose;

protected:
    void            process_sysex(std::vector<unsigned char>* _message);
    void            process_event(std::vector<unsigned char>* _message);

    track_state     m_track_state[16];
    sound_state     m_sound_state[16];
    key_state       m_key_state;
    key_state       m_key_prev_state;

    float           m_volume;

    track           m_track;
    page            m_page;

    unsigned char   m_microphone_mode;
    bool            m_play;

    std::function<void(key_id, int)> m_key;
    bool            m_key_enable;
};

}