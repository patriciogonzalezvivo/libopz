#include <stdio.h>  /* sprintf() */
#include <stdlib.h> /* malloc() */
#include <string.h>
#include <iostream>
#include <vector>

#include "OPZ.h"

namespace T3 {

// TOOLS
#define BIT(x,b) ((x & (1<<b)))
#define CAST_MESSAGE(TYP, NAM) const TYP & NAM = (const TYP &)buffer[sizeof(sysex_header)-1]

char *hex_full(unsigned char *cp, size_t n) {
    char *s = (char*)malloc(3*n + 1);

    if (s == NULL)
        return s;

    for (size_t k = 0; k < n; ++k)
        sprintf(s + 3*k, "%02X ", cp[k]);

    s[3*n] = '\0';
    return s;
}

char *hex_msg(unsigned char *cp, size_t n) {
    char *s = (char*)malloc(3*(n-6) + 1);

    if (s == NULL)
        return s;

    for (size_t k = 6; k < n-1; ++k)
        sprintf(s + 3*(k-6), "%02X ", cp[k]);

    s[3*(n-6)] = '\0';
    return s;
}

// https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol
std::vector<unsigned char> initial_message = {
    SYSEX_HEAD, 0x7E, 0x7F, 0x06, 0x01, SYSEX_END
};

// 0x00 Master Heartbeat (https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#00-master-heartbeat)
std::vector<unsigned char> master_heartbeat = { 
    // Midi::SYSTEM_EXCLUSIVE, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x03, 0x2D, 0x0E, 0x05, Midi::END_OF_SYSEX // prior to version 1.2.5
    SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x01, 0x4E, 0x2E, 0x06, SYSEX_END // version 1.2.5
}; 

std::string key_name[] = {
    "pager2x",      "mixer",        "record",       "KEY14",        "KEY15",        "KEY16",    "KEY17",    "KEY18",
    "KEY21",        "KEY22",        "KEY23",        "KEY24",        "KEY25",        "KEY26",    "page",     "KEY28",
    "track_0bit",   "track_1bit",   "track_2bit",   "track_3bit",   "track_4bit",   "shift",    "tempo",    "KEY38",
    "KEY41",        "KEY42",        "KEY43",        "KEY44",        "screen",       "stop",     "KEY47",    "KEY48",
    "track",        "project",      "KEY53",        "KEY54",        "KEY55",        "KEY56",    "KEY57",    "KEY58"
};

// https://teenage.engineering/guides/op-z/parameter-pages
std::string page_name[] = { 
    "ONE", "TWO", "THREE", "FOUR" 
};

// https://teenage.engineering/guides/op-z/tracks
std::string track_name[] = { 
    "KICK", "SNARE", "PERC", "SAMPLE", "BASS", "LEAD", "ARP", "CHORD", "FX1", "FX2", "TAPE", "MASTER", "PERFORM", "MODULE", "LIGHT", "MOTION", "NONE" 
};

OPZ::OPZ() :    
m_volume(0.0f), 
m_track(KICK), 
m_page(PAGE_ONE), 
m_microphone_mode(0),
m_play(false) {
}

const std::vector<unsigned char>* OPZ::getInitMsg() const { return &initial_message; }
const std::vector<unsigned char>* OPZ::getHeartBeat() const { return &master_heartbeat; }

void OPZ::process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData) {
    OPZ *device = static_cast<OPZ*>(_userData);

    if (_message->at(0) == SYSEX_HEAD)
        device->process_sysex(_message);
    else
        device->process_event(_message);
}

void OPZ::process_sysex(std::vector<unsigned char>* _message){
    // TODO: check that first bit of buffer is 'F000207601'

    unsigned char *buffer = &_message->at(0);
    size_t length = _message->size();
    const sysex_header &seh = (const sysex_header&)*buffer;
    
    if (memcmp(OPZ_VENDOR_ID, seh.vendor_id, sizeof(OPZ_VENDOR_ID)) != 0){
        printf("Vendor ID %02hhx:%02hhx:%02hhx is not the expected ID %02hhx:%02hhx:%02hhx\n",seh.vendor_id[0],seh.vendor_id[1],seh.vendor_id[2],OPZ_VENDOR_ID[0],OPZ_VENDOR_ID[1],OPZ_VENDOR_ID[2]);
        return;
    }
    if ((seh.protocol_version == 0) || (seh.protocol_version > OPZ_MAX_PROTOCOL_VERSION)){
        printf("Unexpected protocol version %02hhx, was expecting > 0 and <= %02hhx\n", seh.protocol_version, OPZ_MAX_PROTOCOL_VERSION);
        return;
    }
    
    switch (seh.parm_id) {
        case 0x01: {
            // Universal response ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#01-universal-response )
            // printf("Msg %02hhx: Universal response\n", seh.parm_id);
        } break;

        case 0x02: {
            // Track Setting ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#02-track-settings )
            printf("Msg %02hhx: Track Setting\n", seh.parm_id);

        } break;

        case 0x03: {
            // Keyboard Setting ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#03-keyboard-setting )
            printf("Msg %02hhx: Keyboard Setting:", seh.parm_id);
            // std::cout << " " << hex_msg(buffer, length) ;

            // const track_state &ti = (const track_state &)buffer[sizeof(sysex_header)-1];
            CAST_MESSAGE(track_state, ti);
            m_track = (track)ti.track;

            memcpy(&(m_track_state[m_track]), &ti, sizeof(track_state));
            std::cout << "update track " << track_name[m_track];
            printf(", data %02hhx %02hhx\n", m_track_state[m_track].value_p1, m_track_state[m_track].value_p2);
            
        } break;

        case 0x04: {
            printf("Msg %02hhx: ", seh.parm_id);
            std::cout << hex_msg(buffer, length) << std::endl;

            m_volume = ( ((int)buffer[6] == 0)? (int)buffer[8] : 127 + (int)buffer[8] )/254.0;
            m_microphone_mode = buffer[10];

            printf(" Volumen    %f\n", m_volume);
            printf(" ????????   %02x\n", buffer[9] );
            printf(" Microphone %02x\n", m_microphone_mode);
        } break;

        case 0x06: {
            // Button States ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#06-button-states )
            printf("Msg %02hhx (Button States)\n", seh.parm_id);
            // std::cout << hex_msg(buffer, length) << std::endl;

            CAST_MESSAGE(key_state, ki);
            memcpy(&(m_key_prev_state), &m_key_state, sizeof(m_key_state));
            memcpy(&(m_key_state), &ki, sizeof(m_key_state));

            if (m_key_state.step < 16) 
                m_key_track = (int)m_key_state.step;
            else 
                m_key_track = -1;

            m_page = (page)( (int)(m_key_state.page2x) * 2 + (int)(m_key_state.page) );
            std::cout << "  track: " << track_name[m_track] << std::endl;
            std::cout << "  page:  " << page_name[m_page] << std::endl;

            if (m_key_state.step < 16)
                std::cout << "  track_key: " << track_name[m_key_track] << std::endl;

            // printf( "page2x:    %i\n", m_key_state.page2x);
            // printf( "mixer:     %i\n", m_key_state.mixer);
            // printf( "record:    %i\n", m_key_state.record);
            // printf( "key14:     %i\n", m_key_state.key14);

            // printf( "key15:     %i\n", m_key_state.key15);
            // printf( "key16:     %i\n", m_key_state.key16);
            // printf( "key17:     %i\n", m_key_state.key17);
            // printf( "key18:     %i\n", m_key_state.key18);

            // printf( "key21:     %i\n", m_key_state.key21);
            // printf( "key22:     %i\n", m_key_state.key22);
            // printf( "key23:     %i\n", m_key_state.key23);
            // printf( "key24:     %i\n", m_key_state.key24);

            // printf( "key25:     %i\n", m_key_state.key25);
            // printf( "key26:     %i\n", m_key_state.key26);
            // printf( "page:      %i\n", m_key_state.page);
            // printf( "key28:     %i\n", m_key_state.key28);

            // printf( "step:      %i \n", m_key_state.step) ;
        
            // printf( "shift:     %i\n", m_key_state.shift);
            // printf( "tempo:     %i\n", m_key_state.tempo);
            // printf( "key38:     %i\n", m_key_state.key38);

            // printf( "key41:     %i\n", m_key_state.key41);
            // printf( "key42:     %i\n", m_key_state.key42);
            // printf( "key43:     %i\n", m_key_state.key43);
            // printf( "key44:     %i\n", m_key_state.key44);

            // printf( "key45:     %i\n", m_key_state.key45);
            // printf( "screen:    %i\n", m_key_state.screen);
            // printf( "stop:      %i\n", m_key_state.stop);
            // printf( "key48:     %i\n", m_key_state.key48);

            // printf( "track:     %i\n", m_key_state.track);
            // printf( "project:   %i\n", m_key_state.project);
            // printf( "key53:     %i\n", m_key_state.key53);
            // printf( "key54:     %i\n", m_key_state.key54);

            // printf( "key55:     %i\n", m_key_state.key55);
            // printf( "key56:     %i\n", m_key_state.key56);
            // printf( "key57:     %i\n", m_key_state.key57);
            // printf( "key58:     %i\n", m_key_state.key58);
            
        } break;

        case 0x07: {
            // Sequencer Settings ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#07-sequencer-settings )
            printf("Msg %02hhx (Sequencer Settings) ", seh.parm_id);
            std::cout << hex_msg(buffer, length) << std::endl;
        } break;

        case 0x09: {
            // Pattern ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#09-pattern )
            printf("Msg %02hhx (Pattern) ", seh.parm_id);
            std::cout << hex_msg(buffer, length) << std::endl;
        } break;

        case 0x0c: {
            // Global Data ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#0c-global-data )
            printf("Msg %02hhx (Global Data) ", seh.parm_id);
            std::cout << hex_msg(buffer, length) << std::endl;
        } break;

        case 0x0e: {
            // Sound preset ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#0e-sound-preset )
            printf("Msg %02hhx (Sound preset) ", seh.parm_id);
            m_key_track = buffer[7];

            // std::cout << hex_msg(buffer, length) << " " << std::endl;
            // std::cout << "track: " << track_name[m_track] << std::endl;
            // std::cout << "  active page:  " << page_name[m_page] << std::endl;

            CAST_MESSAGE(sound_state, si);
            memcpy(&(m_sound_state[m_track]), &si, sizeof(sound_state));

            // printf( "   byte1X:     %i %i %i %i  %i %i %i %i\n", si.byte11, si.param1_hf, si.param2_hf, si.attack_hf, si.decay_hf, si.sustain_hf, si.release_hf, si.byte18);
            // printf( "   param1:     %i\n", si.param1);
            // printf( "   param2:     %i\n", si.param2);

            // printf( "   attack:     %i\n", si.attack);
            // printf( "   decay:      %i\n", si.decay);
            // printf( "   sustain:    %i\n", si.sustain);
            // printf( "   release:    %i\n", si.release);

            // printf( "   byte2X:     %i %i %i %i  %i %i %i %i\n", si.fx1_hf, si.fx2_hf, si.filter_hf, si.resonance_hf, si.pan_hf, si.level_hf, si.portamento_hf, si.byte28);
            // printf( "   fx1:        %i\n", si.fx1);
            // printf( "   fx2:        %i\n", si.fx2);
            // printf( "   filter:     %i\n", si.filter);
            // printf( "   resonance:  %i\n", si.resonance);
            // printf( "   pan:        %i\n", si.pan);
            // printf( "   level:      %i\n", si.level);
            // printf( "   portamendo: %i\n", si.portamento);

            // printf( "   byte3X:     %i %i %i %i  %i %i %i %i\n", si.lfo_depth_hf, si.lfo_speed_hf, si.lfo_value_hf, si.lfo_shape_hf, si.note_style_hf, si.byte36, si.byte37, si.byte38);
            // printf( "   lfo_depth:  %i\n", si.lfo_depth);
            // printf( "   lfo_speed:  %i\n", si.lfo_speed);
            // printf( "   lfo_value:  %i\n", si.lfo_value);
            // printf( "   lfo_shape:  %i\n", si.lfo_shape);

            // printf( "   note_style:  %i\n", si.note_style_hf);

        } break;

        case 0x10: {
            // Compressed MIDI Config ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#10-zlib-compressed-midi-config )
            printf("Msg %02hhx: Compressed MIDI Config\n", seh.parm_id);
        } break;

        case 0x11: {
            printf("Msg %02hhx: ", seh.parm_id);
            std::cout << hex_msg(buffer, length) << std::endl;
        } break;

        case 0x12: {
            // Sound State ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#12-sound-state )
            printf("Msg %02hhx: Sound State\n", seh.parm_id);
        } break;

        default: {
            printf("Msg %02hhx: ", seh.parm_id);
            std::cout << hex_full(buffer, length) << std::endl;
        }
    }

}

void OPZ::process_event(std::vector<unsigned char>* _message) {

    if (_message->at(0) == START_SONG)
        m_play = true;
    else if (_message->at(0) == STOP_SONG)
        m_play = false;

    int bytes = 0;
    unsigned char status = 0;
    unsigned char channel = 0;

    int j = 0;
    if ((_message->at(0) & 0xf0) != 0xf0) {
        channel = _message->at(0) & 0x0f;
        channel += 1;
        status = _message->at(0) & 0xf0;
    }
    else {
        channel = 0;
        status = _message->at(0);
    }

    switch (status) {
        case CONTROLLER_CHANGE:
            bytes = 2;
            break;

        case NOTE_ON:
            bytes = 2;
            break;

        case NOTE_OFF:
            bytes = 2;
            break;

        case KEY_PRESSURE:
            bytes = 2;
            break;

        case PROGRAM_CHANGE:
            bytes = 1;
            break;

        case CHANNEL_PRESSURE:
            bytes = 1;
            break;

        case PITCH_BEND:
            bytes = 2;
            break;

        case SONG_POSITION:
            bytes = 2;
            break;

        case SONG_SELECT:
            bytes = 2;
            break;

        case TUNE_REQUEST:
            bytes = 2;
            break;

        case TIMING_TICK:
            bytes = 0;
            break;

        case START_SONG:
            bytes = 0;
            break;

        case CONTINUE_SONG:
            bytes = 0;
            break;

        case STOP_SONG:
            bytes = 0;
            break;

        default:
            bytes = 0;
            break;
    }

    if (status == NOTE_ON && _message->at(2) == 0)
        status = NOTE_OFF;

    // if (bytes >= 2) {
    //     std::cout << ", channel: " << channel;

    //     size_t key = _message->at(1);
    //     size_t target_value = _message->at(2);

    //     std::cout << ", key: " << _message->at(1);
    //     std::cout << ", value: " << _message->at(2);
    // }

    // std::cout << std::endl;
}

float OPZ::getSoundProperty(track _track, sound_prop _prop) {
    if (_prop == SOUND_PARAM1)
        return (m_sound_state[m_track].param1 + m_sound_state[m_track].param1_hf * 127) / 255.0f;
    else if (_prop == SOUND_PARAM2)
        return (m_sound_state[m_track].param2 + m_sound_state[m_track].param2_hf * 127) / 255.0f;
    else if (_prop == SOUND_FILTER)
        return (m_sound_state[m_track].filter + m_sound_state[m_track].filter_hf * 127) / 255.0f; 
    else if (_prop == SOUND_RESONANCE)
        return (m_sound_state[m_track].resonance + m_sound_state[m_track].resonance_hf * 127) / 255.0f;

    // TODO
    //      - I got the names wrong
    else if (_prop == ENVELOPE_ATTACK) // S
        return (m_sound_state[m_track].attack + m_sound_state[m_track].attack_hf * 127) / 255.0f;
    else if (_prop == ENVELOPE_DECAY) // A
        return (m_sound_state[m_track].decay + m_sound_state[m_track].decay_hf * 127) / 255.0f;
    else if (_prop == ENVELOPE_SUSTAIN) // H
        return (m_sound_state[m_track].sustain + m_sound_state[m_track].sustain_hf * 127) / 255.0f; 
    else if (_prop == ENVELOPE_RELEASE) // D
        return (m_sound_state[m_track].release + m_sound_state[m_track].release_hf * 127) / 255.0f; 

    else if (_prop == LFO_DEPTH)
        return (m_sound_state[m_track].lfo_depth + m_sound_state[m_track].lfo_depth_hf * 127) / 255.0f;
    else if (_prop == LFO_SPEED) // RATE
        return (m_sound_state[m_track].lfo_speed + m_sound_state[m_track].lfo_speed_hf * 127) / 255.0f;
    else if (_prop == LFO_VALUE) // DEST
        return (m_sound_state[m_track].lfo_value + m_sound_state[m_track].lfo_value_hf * 127) / 255.0f; 
    else if (_prop == LFO_SHAPE)
        return (m_sound_state[m_track].lfo_shape + m_sound_state[m_track].lfo_shape_hf * 127) / 255.0f;

    else if (_prop == SOUND_FX1)
        return (m_sound_state[m_track].fx1 + m_sound_state[m_track].fx1_hf * 127) / 255.0f;
    else if (_prop == SOUND_FX2)
        return (m_sound_state[m_track].fx2 + m_sound_state[m_track].fx2_hf * 127) / 127.0f - 1.0f;
    else if (_prop == SOUND_PAN)
        return (m_sound_state[m_track].pan + m_sound_state[m_track].pan_hf * 127) / 255.0f; 
    else if (_prop == SOUND_LEVEL)
        return (m_sound_state[m_track].level + m_sound_state[m_track].level_hf * 127) / 255.0f;

    else if (_prop == PORTAMENTO)
        return (m_sound_state[m_track].portamento + m_sound_state[m_track].portamento_hf * 127) / 255.0f;

    return 0.0f;
}

}