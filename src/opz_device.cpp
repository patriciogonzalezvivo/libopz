#include <stdio.h>  /* sprintf() */
#include <stdlib.h> /* malloc() */
#include <string.h>
#include <iostream>

#include "opz_device.h"
#include "tools.h"

namespace T3 {

// TOOLS
#define BIT(x,b) ((x & (1<<b)))
#define CAST_MESSAGE(TYP, NAM) const TYP & NAM = (const TYP &)data[0]
#define BYTE_TO_BINARY_PATTERN "%c %c %c %c  %c %c %c %c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

// https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol
std::vector<unsigned char> initial_message = {
    SYSEX_HEAD, 0x7E, 0x7F, 0x06, 0x01, SYSEX_END
};

// 0x00 Master Heartbeat (https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#00-master-heartbeat)
std::vector<unsigned char> master_heartbeat = { 
    // SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x03, 0x2D, 0x0E, 0x05, SYSEX_END // prior to version 1.2.5
    SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x01, 0x4E, 0x2E, 0x06, SYSEX_END // version 1.2.5
    // SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x01, 0x7F, 0x75, 0x06, SYSEX_END // new version
    // SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x00, 0x2C, 0x54, 0x06, SYSEX_END // 
    // SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x02, 0x27, 0x3C, 0x06, SYSEX_END
}; 

std::vector<unsigned char> config_cmd = {
    SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x0F, SYSEX_END
};

std::string event_name[] = {
    "VOLUME_CHANGE",
    "KEY_PROJECT", "KEY_MIXER", "KEY_TEMPO", "KEY_SCREEN",
    "KEY_TRACK", "KEY_KICK", "KEY_SNARE", "KEY_PERC", "KEY_SAMPLE", "KEY_BASS", "KEY_LEAD", "KEY_ARP", "KEY_CHORD", "KEY_FX1", "KEY_FX2", "KEY_TAPE", "KEY_MASTER", "KEY_PERFORM", "KEY_MODULE", "KEY_LIGHT", "KEY_MOTION",
    "KEY_RECORD", "PLAY_CHANGE", "KEY_STOP", 
    "OCTAVE_CHANGE", "KEY_SHIFT",
    "PROJECT_CHANGE", "PATTERN_CHANGE", "TRACK_CHANGE", "PAGE_CHANGE", "SEQUENCE_CHANGE", "MUTE_CHANGE"
    "MICROPHONE_MODE_CHANGE", "MICROPHONE_LEVEL_CHANGE", "MICROPHONE_FX_CHANGE",
    "PARAMETER_CHANGE",
    "PATTERN_PACKAGE_RECIVED", "PATTERN_DOWNLOADED"
    "NO_CONNECTION"
};

// https://teenage.engineering/guides/op-z/parameter-pages
std::string page_name[] = {     "ONE", "TWO", "THREE", "FOUR" };
std::string mic_fx_name[] = {   "NONE", "FX1", "FX2", "FX1 & FX2"   };

const std::vector<unsigned char>* opz_init_msg() { return &initial_message; }
const std::vector<unsigned char>* opz_heartbeat() { return &master_heartbeat; }
const std::vector<unsigned char>* opz_config_cmd() { return &config_cmd; };
std::vector<unsigned char> opz_confirm_package_cmd(uint8_t *_data, size_t _length) { 
    // std::cout << "       DATA MSG                 | " << T3::printHex(_data, 6) << std::endl; 
    std::vector<unsigned char> msg = {  0x09, 0x00, 0x00, _data[1], _data[2], _data[3], _data[4], 0x00  };
    std::vector<unsigned char> sysex_out = { T3::SYSEX_HEAD, T3::OPZ_VENDOR_ID[0], T3::OPZ_VENDOR_ID[1], T3::OPZ_VENDOR_ID[2], T3::OPZ_MAX_PROTOCOL_VERSION, 0x0B };
    sysex_out.resize(100);
    size_t outdata_length = T3::encode(&msg[0], msg.size(), &sysex_out[6]);
    sysex_out.resize(6 + outdata_length);
    sysex_out.push_back( T3::SYSEX_END );
    // std::cout << "   OUT 7BIT MSG " << T3::printHex(&sysex_out[0], sysex_out.size()) << std::endl; 
    return sysex_out;
}

std::string& toString( opz_page_id  _id ) { return page_name[_id]; }
std::string& toString( opz_mic_fx_id _id ) { return mic_fx_name[_id]; }
std::string& toString( opz_event_id _id ) { return event_name[_id]; }
std::string toString( midi_id _id ) { 
    if (_id == CONTROLLER_CHANGE) return "CONTROLLER_CHANGE";
    else if (_id == NOTE_ON) return "NOTE_ON";
    else if (_id == NOTE_OFF) return "NOTE_OFF";
    else if (_id == KEY_PRESSURE) return "KEY_PRESSURE";
    else if (_id == PROGRAM_CHANGE) return "PROGRAM_CHANGE";
    else if (_id == CHANNEL_PRESSURE) return "CHANNEL_PRESSURE";
    else if (_id == PITCH_BEND) return "PITCH_BEND";
    else if (_id == SYSEX_HEAD) return "SYSEX_HEAD";
    else if (_id == SONG_POSITION) return "SONG_POSITION";
    else if (_id == SONG_SELECT) return "SONG_SELECT";
    else if (_id == TUNE_REQUEST) return "TUNE_REQUEST";
    else if (_id == SYSEX_END) return "SYSEX_END";
    else if (_id == TIMING_TICK) return "TIMING_TICK";
    else if (_id == START_SONG) return "START_SONG";
    else if (_id == CONTINUE_SONG) return "CONTINUE_SONG";
    else if (_id == STOP_SONG) return "STOP_SONG";
    else if (_id == ACTIVE_SENSING) return "ACTIVE_SENSING";
    else if (_id == SYSTEM_RESET) return "SYSTEM_RESET";
    return "UNKNOWN";
}

opz_device::opz_device():
verbose(0),
m_level(0.0f), 
m_active_project(0),
m_active_pattern(0),
m_active_track(KICK), 
m_active_page(PAGE_ONE),
m_active_step(0),
m_mic_mode(0),
m_play(false),
m_event_enable(false),
m_midi_enable(false) {
}

void opz_device::process_message(unsigned char* _message, size_t _length) {
    if (_message[0] == SYSEX_HEAD)
        process_sysex(_message, _length);
    else
        process_event(_message, _length);
}

void opz_device::process_sysex(unsigned char *_message, size_t _length){

    // if (verbose > 1 && verbose < 4)
    //     std::cout << printHex(&_message[0], _length) << "(" << _length << " bytes)" << std::endl;

    const opz_sysex_header &header = (const opz_sysex_header&)_message[0];
    if (memcmp(OPZ_VENDOR_ID, header.vendor_id, sizeof(OPZ_VENDOR_ID)) != 0){
        if (verbose)
            printf("Vendor ID %02X:%02X:%02X is not the expected ID %02X:%02X:%02X\n", header.vendor_id[0],header.vendor_id[1],header.vendor_id[2], OPZ_VENDOR_ID[0],OPZ_VENDOR_ID[1],OPZ_VENDOR_ID[2]);
        return;
    }

    if ((header.protocol_version == 0) || (header.protocol_version > OPZ_MAX_PROTOCOL_VERSION)){
        if (verbose)
            printf("Unexpected protocol version %02X, was expecting > 0 and <= %02X\n", header.protocol_version, OPZ_MAX_PROTOCOL_VERSION);
        return;
    }

    // TODO: check that first bit of data is 'F000207601'

    // substract the header and end of SYSEX message
    size_t data_length = _length-sizeof(opz_sysex_header)-1;
    unsigned char *data = new unsigned char[data_length];

    // decode 7bit into raw 8bit
    size_t length = decode(&_message[sizeof(opz_sysex_header)], data_length, data);
    
    switch (header.parm_id) {
        case 0x01: {
            // // Universal response ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#01-universal-response )
            // if (verbose)
            //     printf("Msg %02X (Universal response)\n", header.parm_id);

            // if (verbose > 1 && verbose < 4)
            //     std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
        } break;

        case 0x02: {
            // Track Setting ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#02-track-settings )
            if (verbose)
                printf("Msg %02X (Track Setting)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            if (data[2] == 0x00) {
                uint8_t project_id = address2project(data[4]);
                uint8_t pattern_id = address2pattern(data[4]);
                uint8_t track_id = data[7];
                uint8_t step_id = data[8];
                size_t note_id = getNoteIdOffset(track_id, step_id);

                if (length == 25) { 
                    // counter  ??  type ??  address ?? ?? ?? key ?? note 
                    // 0        1   2    3   4       5  6  7  8   9  10   11 12 13 14 15 16 17 18 19 20 21 22 23 24   
                    // 3B       00  00   00  50      03 05 00 01  00 3C   00 78 02 50 08 00 00 0A 00 00 36 64 00 00 
                    // 5C       00  00   00  50      03 05 02 01  00 5D   00 98 02 50 08 00 00 0A 00 00 3C 64 00 00

                    // TODO:
                    //      - properly cast a note and overide the place on the patterns note

                    if (verbose > 2) {
                        printf("    NOTE ON\n");
                        printf("    project: %i\n", project_id );
                        printf("    pattern: %i\n", pattern_id );
                        printf("    track:   %i\n", track_id );
                        printf("    step:    %i\n", step_id );
                        printf("    note:    %i\n", data[10] );
                    }

                    m_project.pattern[pattern_id].note[note_id].note = data[10];

                    if (m_event_enable)
                        m_event(SEQUENCE_CHANGE, data[4]);

                } else if (length == 10) {
                
                    // counter  ??  type ??  address  ??   ?? ?? key ??
                    // 0        1   2    3   4        5    6  7  8   9
                    // 3A       00  00   00  50       03   05 00 01  01 
                    if (verbose > 2) {
                        printf("    NOTE OFF\n");
                        printf("    project: %i\n", project_id );
                        printf("    pattern: %i\n", pattern_id );
                        printf("    track:   %i\n", track_id );
                        printf("    step:    %i\n", step_id );
                    }

                    m_project.pattern[pattern_id].note[note_id].note = 0xFF;

                    if (m_event_enable)
                        m_event(SEQUENCE_CHANGE, data[4]);

                }
            }
            else if (length == 11 && data[2] == 0x60) {
                // counter  ??  type    ??  address ?? ?? ?? ?? ?? ??
                // BA       10  60      53  11      04 00 80 00 00 00 
                // A3       14  60      53  20      04 00 08 00 00 00
                
                bool mutes[] = {
                    (data[ 7] & 0x02) ? true : false, (data[ 7] & 0x08) ? true : false, (data[ 7] & 0x20) ? true : false, (data[ 7] & 0x80) ? true : false,
                    (data[ 8] & 0x02) ? true : false, (data[ 8] & 0x08) ? true : false, (data[ 8] & 0x20) ? true : false, (data[ 8] & 0x80) ? true : false, 
                    (data[ 9] & 0x02) ? true : false, (data[ 9] & 0x08) ? true : false, (data[ 9] & 0x20) ? true : false, (data[ 9] & 0x80) ? true : false,
                    (data[10] & 0x02) ? true : false, (data[10] & 0x08) ? true : false, (data[10] & 0x20) ? true : false, (data[10] & 0x80) ? true : false
                };
                memcpy(&m_mutes, &mutes, sizeof(bool) * 16);

                if (verbose > 2) {
                    printf("    project: %i\n", address2project(data[4]) );
                    printf("    pattern: %i\n", address2pattern(data[4]) );
                    printf("    type:    0x%02X\n", data[2]);
                    printf("    mutes:   ");
                    for (size_t i = 0; i < 16; i++)
                        printf( "%c ", m_mutes[i] ? '1' : '0' );
                    printf("\n");
                }

                if (m_event_enable)
                    m_event(MUTE_CHANGE, 1);

            } else if (length == 8) {
                // counter  ??  type ??  address  ?? ?? value  
                // F1       0F  09   00  10       01 00 47
                // B8       10  09   00  11       01 00 3C

                if (verbose > 2) {
                    printf("    project: %i\n", address2project(data[4]) );
                    printf("    pattern: %i\n", address2pattern(data[4]) );
                    printf("    type:    %s\n", toString((opz_sound_parameter_id)data[2]).c_str() );
                    printf("    value:   %03i\n", data[7] );
                }

                if (m_event_enable)
                    m_event(PARAMETER_CHANGE, 1);

            }
            

        } break;

        case 0x03: {
            // Keyboard Setting ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#03-keyboard-setting )
            if (verbose)
                printf("Msg %02X (Keyboard Setting)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            //  Octave
            m_octave[(size_t)m_active_track] = (int8_t)data[0];
            if (m_event_enable)
                m_event(OCTAVE_CHANGE, m_octave[(size_t)m_active_track]);

            //  Active Track
            opz_track_id track = (opz_track_id)(data[1]%16);
            if (m_active_track != track)
                if (m_event_enable)
                    m_event(TRACK_CHANGE, (int)m_active_track);
            m_active_track = track;

            //  Play
            size_t play = data[1]/16;
            if (play)
                if (!m_play && m_event_enable) 
                    m_event(PLAY_CHANGE, 1);
            else 
                if (m_play && m_event_enable)
                    m_event(PLAY_CHANGE, 0);
            m_play = play;

        } break;

        case 0x04: {
            if (verbose)
                printf("Msg %02X (IN/OUT?)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            float   volume = data[1]/ 255.0f;
            float   mic_level = (data[2]%16) / 15.0f;
            opz_mic_fx_id mic_fx = (opz_mic_fx_id)(data[2]/16);
            if (verbose > 2) {
                printf(" ????????   %02x\n", data[0] );
                printf(" LEVEL      %f\n",  volume);
                printf(" Mic. LEVEL %f\n", mic_level );
                printf(" Mic. FX    %i\n", mic_fx );
                printf(" Mic. MODE  %02x\n", data[3] );
            }

            if (m_level != volume) {
                m_level = volume;
                if (m_event_enable)
                    m_event(VOLUME_CHANGE, m_level * 100);
            }

            if (m_mic_mode != data[3]) {
                m_mic_mode = data[3];
                if (m_event_enable)
                    m_event(MICROPHONE_MODE_CHANGE, m_mic_mode);
            }

            if (m_mic_level != mic_level) {
                m_mic_level = mic_level;
                if (m_event_enable)
                    m_event(MICROPHONE_LEVEL_CHANGE, m_mic_level * 100);
            }

            if (m_mic_fx != mic_fx) {
                m_mic_fx = mic_fx;
                if (m_event_enable)
                    m_event(MICROPHONE_FX_CHANGE, m_mic_fx);
            }


        } break;

        case 0x05: {
            // Unknown
            if (verbose)
                printf("Msg %02X (unknown)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            if (verbose > 2) {
                printf("    unknown:    %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X\n", 
                                        data[0], data[1], data[2], data[3],
                                        data[4], data[5], data[6], data[7],
                                        data[8], data[9], data[10], data[11],
                                        data[12], data[13], data[14], data[15]);
                // printf("    0:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[0]));
                // printf("    1:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[1]));
                // printf("    2:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[2]));
                // printf("    3:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[3]));
                printf("    4:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[4]));
                // printf("    5:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[5]));
                // printf("    6:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[6]));
                // printf("    7:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[7]));
                // printf("    8:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[8]));
                // printf("    9:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[9]));
                // printf("    A:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[10]));
                // printf("    B:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[11]));
                // printf("    C:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[12]));
                // printf("    D:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[13]));
                printf("    E:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[14]));
                // printf("    F:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[15]));
            }

        } break;

        case 0x06: {
            // Button States ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#06-button-states )
            if (verbose)
                printf("Msg %02X (Button States)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            CAST_MESSAGE(opz_key_state, ki);
            memcpy(&(m_key_prev_state), &m_key_state, sizeof(m_key_state));
            memcpy(&(m_key_state), &ki, sizeof(m_key_state));

            if (m_active_page != (opz_page_id)m_key_state.page)
                if (m_event_enable)
                    m_event(PAGE_CHANGE, (int)m_active_page);
        
            m_active_page = (opz_page_id)m_key_state.page;

            if (m_event_enable) {
                // if (verbose > 2) {
                //     printf( "   bit1 1-6:  %i %i %i %i %i %i\n", m_key_state.bit11, m_key_state.bit12, m_key_state.bit13, m_key_state.bit14, m_key_state.bit15, m_key_state.bit16);
                //     printf( "   page:      %i\n", m_key_state.page);
                //     printf( "   step:      %i\n", m_key_state.step);
                //     printf( "   shift:     %i\n", m_key_state.shift);
                //     printf( "   tempo:     %i\n", m_key_state.tempo);
                //     printf( "   mixer:     %i\n", m_key_state.mixer);
                //     printf( "   bit3 1-5:  %i %i %i %i %i\n", m_key_state.bit31, m_key_state.bit32, m_key_state.bit33, m_key_state.bit34, m_key_state.bit35);
                //     printf( "   screen:    %i\n", m_key_state.screen);
                //     printf( "   stop:      %i\n", m_key_state.stop);
                //     printf( "   record:    %i\n", m_key_state.record);
                //     printf( "   track:     %i\n", m_key_state.track);
                //     printf( "   project:   %i\n", m_key_state.project);
                //     printf( "   bit4 3-8:  %i %i %i %i %i %i\n", m_key_state.bit43, m_key_state.bit44, m_key_state.bit45, m_key_state.bit46, m_key_state.bit47, m_key_state.bit48);
                // }

                if (m_key_state.project) m_event( KEY_PROJECT, 1);
                if (m_key_prev_state.project && !m_key_state.project ) m_event( KEY_PROJECT, 0);
                
                if (m_key_state.mixer) m_event( KEY_MIXER, 1);
                if (m_key_prev_state.mixer && !m_key_state.mixer) m_event( KEY_MIXER, 0);

                if (m_key_state.tempo) m_event( KEY_TEMPO, 1);
                if (m_key_prev_state.tempo && !m_key_state.tempo) m_event( KEY_TEMPO, 0);
                
                if (m_key_state.screen) m_event( KEY_SCREEN, 1);
                if (m_key_prev_state.screen && !m_key_state.screen) m_event( KEY_SCREEN, 0);

                if (m_key_state.track) m_event( KEY_TRACK, 1);
                if (m_key_prev_state.track && !m_key_state.track) m_event( KEY_TRACK, 0);

                if (m_key_state.step < 16) m_event( opz_event_id( 6+(int)m_key_state.step ), 1);
                if (m_key_prev_state.step < 16 && m_key_state.step >= 16) m_event( opz_event_id( 6+(int)m_key_prev_state.step ), 0);

                if (m_key_state.record) m_event( KEY_RECORD, 1);
                if (m_key_prev_state.record && !m_key_state.record) m_event( KEY_RECORD, 0);

                if (m_key_state.stop) m_event( KEY_STOP, 1);
                if (m_key_prev_state.stop && !m_key_state.stop) m_event( KEY_STOP, 0);

                if (m_key_state.shift) m_event( KEY_SHIFT, 1);
                if (m_key_prev_state.shift && !m_key_state.shift) m_event( KEY_SHIFT, 0);
            }

        } break;

        case 0x07: {
            // Sequencer Settings ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#07-sequencer-settings )
            if (verbose)
                printf("Msg %02X (Sequencer Settings)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            uint8_t pattern = data[0];
            uint8_t address = data[17];
            uint8_t project = data[19];
            m_active_step = 0;

            if (m_active_project != project) {
                m_active_project = project%16;
                if (m_event_enable)
                    m_event(PROJECT_CHANGE, (int)m_active_project);
            }
            
            if (m_active_pattern != pattern) {
                m_active_pattern = pattern%16;
                if (m_event_enable)
                    m_event(PATTERN_CHANGE, (int)m_active_pattern); 
            }
            
            if (verbose > 2) {
                printf("    pattern:    %02X (pattern %i)\n", m_active_pattern, m_active_pattern + 1);
                printf("    address:    %02X (offset %i bytes)\n", address, address);
                printf("    project:    %02X (project %i)\n", m_active_project, m_active_project + 1);
            }

        } break;

        case 0x09: {
            uint8_t pattern_address = data[0];
            uint8_t project = address2project(data[0]);
            uint8_t pattern = address2pattern(data[0]);
            size_t encoded_data_start = 6;
            size_t encoded_data_length = length - encoded_data_start;

            // Pattern ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#09-pattern )
            if (verbose)
                printf("Msg %02X (Pattern %02X Package %02X)\n", header.parm_id, data[0], data[4]);

            if (data[4] == 0x00)
                m_packets.clear();

            m_packets.insert(m_packets.end(), &data[encoded_data_start], &data[encoded_data_start] + encoded_data_length);

            // Global data is compressed using zlib
            if (verbose > 1 && verbose < 4) {
                // std::cout << "      7BT " << T3::printHex(_message, 15) << " .. F7" << std::endl; 
                // std::cout << "      RAW F0 00 20 76 01 09 " << T3::printHex(data, length) << "F7" << std::endl;

                // if (data[encoded_data_start] == 0x78) {
                //     std::cout << "      ENCODED DATA                            " << T3::printHex(&data[encoded_data_start], encoded_data_length) << "(" << encoded_data_length << " bytes)" << std::endl;
                //     std::vector<unsigned char> data_decompressed = decompress(&data[encoded_data_start], encoded_data_length);
                //     std::cout << "      DECODED DATA                            " << printHex(&data_decompressed[0], data_decompressed.size()) << "(" << data_decompressed.size() << " bytes)" << std::endl;
                //     std::cout << std::endl;
                // }

                // std::vector<unsigned char> buffer_decompressed = decompress(&m_packets[0], m_packets.size());
                // std::cout << std::endl;                // std::vector<unsigned char> buffer_decompressed = decompress(&m_packets[0], m_packets.size());
                // std::cout << std::endl;
                // std::cout << "   BUFFER     " << printHex(&buffer_decompressed[0], buffer_decompressed.size()) << "(" << buffer_decompressed.size() << " bytes)" << std::endl;
            }

            if (m_packet_recived_enabled)
                m_packet_recived(header.parm_id, data, 6);

        } break;

        case 0x0a: {

            uint8_t pattern_address = data[0];
            uint8_t project = address2project(data[0]);
            uint8_t pattern = address2pattern(data[0]);
            size_t encoded_data_start = 6;
            size_t encoded_data_length = length - encoded_data_start;

            if (verbose)
                printf("Msg %02X (Pattern %02X Package END)\n", header.parm_id, data[0]);

            // if (data_length > 7)
            //     m_packets.insert(m_packets.end(), &data[encoded_data_start], &data[encoded_data_start] + encoded_data_length);

            std::vector<unsigned char> decompressed = decompress(&m_packets[0], m_packets.size());

            if (verbose > 1 && verbose < 4) {
                // std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                // std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
                // std::cout << "      7BT " << T3::printHex(_message, 13) << " .. " << std::endl; 
                std::cout << "   Downloaded total of "  << m_packets.size() << " bytes" << std::endl;
                std::cout << "   Decompressed into   "  << decompressed.size() << " bytes" << std::endl;
            }

            // const opz_pattern & pi = (const opz_pattern&)decompressed[0];
            memcpy(&m_project.pattern[0], &decompressed[0], std::min( sizeof(opz_pattern) * 16, sizeof(uint8_t) * decompressed.size()) );
            // memcpy(&m_project.pattern[0], &decompressed[0], sizeof(uint8_t) * decompressed.size() );

            // const opz_pattern & pi = (const opz_pattern&)decompressed[0];
            // memcpy(&m_project.pattern[0], &pi, sizeof(uint8_t) * decompressed.size() );
            // memcpy(&m_project.pattern[(size_t)pattern], &pi, std::min( sizeof(uint8_t) * decompressed.size(), sizeof(opz_pattern)));

            if (verbose > 2) {
                printf("   address:     0x%02X\n", data[0]);
                for (size_t i = 0; i < 7; i++) {
                    printf("   project %02i pattern %02i %6s: plug 0x%02X, step_count %03i, ?? 0x%02X, step_length %03i, quantize %03i, note_style %03i, note_length %03i, ?? 0x%02X 0x%02X\n", 
                        project, pattern, toString((opz_track_id)i).c_str(),
                        m_project.pattern[(size_t)pattern].track_param[i].plug,
                        m_project.pattern[(size_t)pattern].track_param[i].step_count,
                        m_project.pattern[(size_t)pattern].track_param[i].unknown1,
                        m_project.pattern[(size_t)pattern].track_param[i].step_length,
                        m_project.pattern[(size_t)pattern].track_param[i].quantize,
                        m_project.pattern[(size_t)pattern].track_param[i].note_style,
                        m_project.pattern[(size_t)pattern].track_param[i].note_length,
                        m_project.pattern[(size_t)pattern].track_param[i].unknown2[0], 
                        m_project.pattern[(size_t)pattern].track_param[i].unknown2[1]
                    );
                }
            }

            if (m_event_enable)
                m_event(PATTERN_DOWNLOADED, pattern);
            
        }break;

        case 0x0c: {
            // Global Data ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#0c-global-data )
            if (verbose)
                printf("Msg %02X (Global Data)\n", header.parm_id);

            // Global data is compressed using zlib
            std::vector<unsigned char> decompressed = decompress(data, length);

            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
            
            const opz_project_data &pi = (const opz_project_data &)decompressed[0];

            memcpy(&m_project, &pi, sizeof(uint8_t) * decompressed.size());// sizeof(pi));

            if (verbose > 2) {
                printf("    drum level:      %i\n", pi.drum_level);
                printf("    synth level:     %i\n", pi.synth_level);
                printf("    pinch level:     %i\n", pi.punch_level);
                printf("    master level:    %i\n", pi.master_level);
                printf("    project tempo:   %i\n", pi.tempo);
                printf("    swing:           %i\n", pi.swing);
                printf("    metronome_level: %i\n", pi.metronome_level);
                printf("    metronome_sound: %i\n", pi.metronome_sound);
            }

        } break;

        case 0x0e: {
            // Sound preset ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#0e-sound-preset )
            if (verbose)
                printf("Msg %02X (Track Parameter)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "    (" << length << " bytes)" << std::endl;

            if (m_active_track != (opz_track_id)(data[0]%16)) {
                m_active_track = (opz_track_id)(data[0]%16);
                if (m_event_enable)
                    m_event(TRACK_CHANGE, (int)m_active_track);
            }

            const opz_sound_parameter &si = (const opz_sound_parameter &)data[1];
            memcpy(&m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track], &si, sizeof(opz_sound_parameter));

            if (m_event_enable)
                m_event(PARAMETER_CHANGE, 1);
                
            if (verbose > 2) {
                printf( " Proj. %i, pattern %i, track %s\n", m_active_project, m_active_pattern, toString(m_active_track).c_str());
                printf( "   param1:     %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].param1);
                printf( "   param2:     %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].param2);
                printf( "   attack:     %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].attack);
                printf( "   decay:      %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].decay);
                printf( "   ustain:     %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].sustain);
                printf( "   release:    %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].release);
                printf( "   fx1:        %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].fx1);
                printf( "   fx2:        %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].fx2);
                printf( "   filter:     %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].filter);
                printf( "   resonance:  %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].resonance);
                printf( "   pan:        %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].pan);
                printf( "   level:      %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].level);
                printf( "   portamendo: %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].portamento);
                printf( "   lfo_depth:  %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].lfo_depth);
                printf( "   lfo_speed:  %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].lfo_speed);
                printf( "   lfo_value:  %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].lfo_value);
                printf( "   lfo_shape:  %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].lfo_shape);
                printf( "   note_style: %i\n", m_project.pattern[m_active_pattern].sound_param[(size_t)m_active_track].note_style);
            }
        } break;

        case 0x10: {
            // Compressed MIDI Config ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#10-zlib-compressed-midi-config )
            if (verbose)
                printf("Msg %02X (Compressed MIDI Config)\n", header.parm_id);

            std::vector<unsigned char> decompressed = decompress(&data[0], length);

            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
        } break;

        case 0x11: {
            // https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#11-
            if (verbose)
                printf("Msg %02X (unknown)\n", header.parm_id);

            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ASC " << printAscii(data, length) << std::endl;
            }
        
        } break;

        case 0x12: {
            // Sound State ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#12-sound-state )
            if (verbose)
                printf("Msg %02X (Sound State)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
        } break;

        case 0x16: {
            if (verbose)
                printf("Msg %02X (Sample Data?)\n", header.parm_id);

            size_t offset = 1;
            std::vector<unsigned char> decompressed = decompress(&data[offset], length-offset);
            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
        } break;

        case 0x18: {
            if (verbose)
                printf("Msg %02X (unknown after 0x15 0x00 0x00 0x00 0x00 0x00)\n", header.parm_id);

            size_t offset = 1;
            std::vector<unsigned char> decompressed = decompress(&data[offset], length-offset);
            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
        } break;

        default: {
            if (verbose)
                printf("Msg %02X (unknown)\n", header.parm_id);

            size_t offset = 0;
            std::vector<unsigned char> decompressed = decompress(&data[offset], length-offset);
            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
        }
    }

}

void opz_device::process_event(unsigned char* _message, size_t _length) {

    if (_message[0] == START_SONG)
        m_play = true;
    else if (_message[0] == STOP_SONG)
        m_play = false;

    int bytes = 0;
    unsigned char status = 0;
    unsigned char channel = 0;

    int j = 0;
    if ((_message[0] & 0xf0) != 0xf0) {
        channel = _message[0] & 0x0f;
        channel += 1;
        status = _message[0] & 0xf0;
    }
    else {
        channel = 0;
        status = _message[0];
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

    if (status == NOTE_ON && _message[2] == 0)
        status = NOTE_OFF;

    if (m_midi_enable) {
        if (bytes >= 2)
            m_midi((midi_id)status, (size_t)channel, (size_t)_message[1], (size_t)_message[2]);

        else 
            m_midi((midi_id)status, (size_t)channel, 0, 0);
    }
}

}