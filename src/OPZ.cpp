#include <stdio.h>  /* sprintf() */
#include <stdlib.h> /* malloc() */
#include <string.h>
#include <iostream>
#include <vector>

#include <zlib.h>

#include "OPZ.h"

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
    // Midi::SYSTEM_EXCLUSIVE, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x03, 0x2D, 0x0E, 0x05, Midi::END_OF_SYSEX // prior to version 1.2.5
    SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 0x00, 0x01, 0x4E, 0x2E, 0x06, SYSEX_END // version 1.2.5
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
    "PROJECT_CHANGE", "PATTERN_CHANGE", "TRACK_CHANGE", "PAGE_CHANGE",
    "MICROPHONE_MODE_CHANGE", "MICROPHONE_LEVEL_CHANGE", "MICROPHONE_FX_CHANGE",
    "PARAMETER_CHANGE"
};

// https://teenage.engineering/guides/op-z/parameter-pages
std::string page_name[] = { 
    "ONE", "TWO", "THREE", "FOUR" 
};

 std::string mic_fx_name[] = {
    "NONE", "FX1", "FX2", "FX1 & FX2"
};

// https://teenage.engineering/guides/op-z/tracks
std::string track_name[] = { 
    "KICK", "SNARE", "PERC", "SAMPLE", "BASS", "LEAD", "ARP", "CHORD", "FX1", "FX2", "TAPE", "MASTER", "PERFORM", "MODULE", "LIGHT", "MOTION", "UNKNOWN" 
};

std::string pattern_page_parameter_name[] = { 
    "KICK_PLUG",    "KICK_PLUG1",   "KICK_PLUG2",   "KICK_PLUG3",   "KICK_STEP_COUNT",  "KICK_UNKNOWN", "KICK_STEP_LENGTH",     "KICK_QUANTIZE",    "KICK_NOTE_STYLE",  "KICK_NOTE_LENGTH",     "KICK_BYTE1",   "KICK_BYTE2",
    "SNARE_PLUG",   "SNARE_PLUG1",  "SNARE_PLUG2",  "SNARE_PLUG3",  "SNARE_STEP_COUNT", "SNARE_UNKNOWN","SNARE_STEP_LENGTH",    "SNARE_QUANTIZE",   "SNARE_NOTE_STYLE", "SNARE_NOTE_LENGTH",        "SNARE_BYTE1", "SNARE_BYTE2",
    "PERC_PLUG",    "PERC_PLUG1",   "PERC_PLUG2",   "PERC_PLUG3",   "PERC_STEP_COUNT",  "PERC_UNKNOWN", "PERC_STEP_LENGTH",     "PERC_QUANTIZE",    "PERC_NOTE_STYLE",  "PERC_NOTE_LENGTH","PERC_BYTE1",    "PERC_BYTE2",
    "SAMPLE_PLUG",  "SAMPLE_PLUG1", "SAMPLE_PLUG2", "SAMPLE_PLUG3", "SAMPLE_STEP_COUNT","SAMPLE_UNKNOWN","SAMPLE_STEP_LENGTH",  "SAMPLE_QUANTIZE", "SAMPLE_NOTE_STYLE", "SAMPLE_NOTE_LENGTH", "SAMPLE_BYTE1",   "SAMPLE_BYTE2",
    "BASS_PLUG",    "BASS_PLUG1",   "BASS_PLUG2",   "BASS_PLUG3",   "BASS_STEP_COUNT",  "BASS_UNKNOWN", "BASS_STEP_LENGTH",     "BASS_QUANTIZE",  "BASS_NOTE_STYLE",    "BASS_NOTE_LENGTH",   "BASS_BYTE1",     "BASS_BYTE2",
    "LEAD_PLUG",    "LEAD_PLUG1",   "LEAD_PLUG2",   "LEAD_PLUG3",   "LEAD_STEP_COUNT",  "LEAD_UNKNOWN", "LEAD_STEP_LENGTH",   "LEAD_QUANTIZE",  "LEAD_NOTE_STYLE",    "LEAD_NOTE_LENGTH",   "LEAD_BYTE1",     "LEAD_BYTE2",
    "ARC_PLUG",     "ARC_PLUG1",    "ARC_PLUG2",    "ARC_PLUG3",    "ARC_STEP_COUNT",   "ARC_UNKNOWN",  "ARC_STEP_LENGTH",    "ARC_QUANTIZE",   "ARC_NOTE_STYLE",     "ARC_NOTE_LENGTH",    "ARC_BYTE1",      "ARC_BYTE2",
    "CHORD_PLUG",   "CHORD_PLUG1",  "CHORD_PLUG2",  "CHORD_PLUG3",  "CHORD_STEP_COUNT", "CHORD_UNKNOWN",  "CHORD_STEP_LENGTH",  "CHORD_QUANTIZE", "CHORD_NOTE_STYLE",   "CHORD_NOTE_LENGTH",  "CHORD_BYTE1",    "CHORD_BYTE2",
    "FX1_PLUG",     "FX1_PLUG1",    "FX1_PLUG2",    "FX1_PLUG3",    "FX1_STEP_COUNT",   "FX1_UNKNOWN",    "FX1_STEP_LENGTH",    "FX1_QUANTIZE",   "FX1_NOTE_STYLE",     "FX1_NOTE_LENGTH",    "FX1_BYTE1",      "FX1_BYTE2",
    "FX2_PLUG",     "FX2_PLUG1",    "FX2_PLUG2",    "FX2_PLUG3",    "FX2_STEP_COUNT",   "FX2_UNKNOWN",    "FX2_STEP_LENGTH",    "FX2_QUANTIZE",   "FX2_NOTE_STYLE",     "FX2_NOTE_LENGTH",    "FX2_BYTE1",      "FX2_BYTE2",
    "TAPE_PLUG",    "TAPE_PLUG1",   "TAPE_PLUG2",   "TAPE_PLUG3",   "TAPE_STEP_COUNT",  "TAPE_UNKNOWN",   "TAPE_STEP_LENGTH",   "TAPE_QUANTIZE",  "TAPE_NOTE_STYLE",    "TAPE_NOTE_LENGTH",   "TAPE_BYTE1",     "TAPE_BYTE2",
    "MASTER_PLUG",  "MASTER_PLUG1", "MASTER_PLUG2", "MASTER_PLUG3", "MASTER_STEP_COUNT","MASTER_UNKNOWN", "MASTER_STEP_LENGTH", "MASTER_QUANTIZE","MASTER_NOTE_STYLE",  "MASTER_NOTE_LENGTH", "MASTER_BYTE1",   "MASTER_BYTE2",
    "PERFORM_PLUG", "PERFORM_PLUG1","PERFORM_PLUG2","PERFORM_PLUG3","PERFORM_STEP_COUNT","PERFORM_UNKNOWN","PERFORM_STEP_LENGTH","PERFORM_QUANTIZE","PERFORM_NOTE_STYLE","PERFORM_NOTE_LENGTH","PERFORM_BYTE1",  "PERFORM_BYTE2",
    "MODULE_PLUG",  "MODULE_PLUG1", "MODULE_PLUG2", "MODULE_PLUG3", "MODULE_STEP_COUNT","MODULE_UNKNOWN", "MODULE_STEP_LENGTH", "MODULE_QUANTIZE","MODULE_NOTE_STYLE",  "MODULE_NOTE_LENGTH", "MODULE_BYTE1",   "MODULE_BYTE2",
    "LIGHT_PLUG",   "LIGHT_PLUG1",  "LIGHT_PLUG2",  "LIGHT_PLUG3",  "LIGHT_STEP_COUNT", "LIGHT_UNKNOWN",  "LIGHT_STEP_LENGTH",  "LIGHT_QUANTIZE", "LIGHT_NOTE_STYLE",   "LIGHT_NOTE_LENGTH",  "LIGHT_BYTE1",    "LIGHT_BYTE2",
    "MOTION_PLUG",  "MOTION_PLUG1", "MOTION_PLUG2", "MOTION_PLUG3", "MOTION_STEP_COUNT","MOTION_UNKNOWN", "MOTION_STEP_LENGTH", "MOTION_QUANTIZE","MOTION_NOTE_STYLE",  "MOTION_NOTE_LENGTH", "MOTION_BYTE1",   "MOTION_BYTE2"
};

std::string page_parameter_name[] = { 
    "SOUND_PARAM1",     "SOUND_PARAM2",     "SOUND_FILTER",     "SOUND_RESONANCE", 
    "ENVELOPE_ATTACK",  "ENVELOPE_DECAY",   "ENVELOPE_SUSTAIN", "ENVELOPE_RELEASE",
    "SOUND_FX1",        "SOUND_FX2",        "SOUND_PAN",        "SOUND_LEVEL",
    "LFO_DEPTH",        "LFO_SPEED",        "LFO_VALUE",        "LFO_SHAPE",
    "NOTE_LENGTH",      "NOTE_STYLE",       "QUANTIZE",         "PORTAMENTO",
    "STEP_COUNT",       "STEP_LENGTH"
};

std::string  note_style_name[] = {
    "DRUM_RETRIG",    "DRUM_MONO",  "DRUM_GATE",      "DRUM_LOOP",
    "SYNTH_POLY",     "SYNTH_MONO", "SYNTH_LEGATO"     
};

// print array of unsigend chars as HEX pairs
char *printHex(unsigned char *cp, size_t n) {
    char *s = (char*)malloc(3*n + 1);

    if (s == NULL)
        return s;

    for (size_t k = 0; k < n; ++k)
        sprintf(s + 3*(k), "%02X ", cp[k]);

    s[3*n] = '\0';
    return s;
}

// encode raw 8bit to 7bits  
size_t encode(const unsigned char* inData, unsigned inLength, unsigned char* outSysEx, bool inFlipHeaderBits = true) {
    size_t outLength    = 0;     // Num bytes in output array.
    unsigned char count = 0;     // Num 7bytes in a block.
    outSysEx[0]         = 0;

    for (unsigned i = 0; i < inLength; ++i) {
        const unsigned char data = inData[i];
        const unsigned char msb  = data >> 7;
        const unsigned char body = data & 0x7f;

        outSysEx[0] |= (msb << (inFlipHeaderBits ? count : (6 - count)));
        outSysEx[1 + count] = body;

        if (count++ == 6) {
            outSysEx   += 8;
            outLength  += 8;
            outSysEx[0] = 0;
            count       = 0;
        }
    }
    return outLength + count + (count != 0 ? 1 : 0);
}

// decode 7bit to raw 8bits
size_t decode(const unsigned char* inData, size_t inLength, unsigned char* outData, bool inFlipHeaderBits = true) {
    size_t count  = 0;
    unsigned char msbStorage = 0;
    unsigned char byteIndex  = 0;

    for (size_t i = 0; i < inLength; ++i) {
        if ((i % 8) == 0) {
            msbStorage = inData[i];
            byteIndex  = 6;
        }
        else {
            const unsigned char body     = inData[i];
            const unsigned char shift    = inFlipHeaderBits ? 6 - byteIndex : byteIndex;
            const unsigned char msb      = uint8_t(((msbStorage >> shift) & 1) << 7);
            byteIndex--;
            outData[count++] = msb | body;
        }
    }
    return count;
}

const unsigned CHUNK_SIZE = 4096;
std::vector<unsigned char> compress(const unsigned char* inData, size_t inLength) {
    std::vector<unsigned char> output;

	z_stream stream;
	stream.zalloc = 0;
	stream.zfree = 0;
	stream.opaque = 0;

	stream.avail_in = inLength - 1;
    stream.next_in = const_cast<Byte *>(&inData[0]);

    int res = deflateInit(&stream, 9);
	if (res != Z_OK)
		return output;

    unsigned char ChunkOut[CHUNK_SIZE];
	do
	{
		stream.avail_out = sizeof(ChunkOut);
		stream.next_out = ChunkOut;
		res = deflate(&stream, Z_FINISH);
		unsigned compressed = sizeof(ChunkOut) - stream.avail_out;
		unsigned oldsize = output.size();
		output.resize(oldsize + compressed);
		memcpy(&output[oldsize], ChunkOut, compressed);
	}
	while (stream.avail_out == 0);
	deflateEnd(&stream);

	return output;
}

std::vector<unsigned char> decompress(const unsigned char* inData, size_t inLength) {
    std::vector<unsigned char> output;

    z_stream stream;
    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;

    stream.avail_in = inLength - 1;
    stream.next_in = const_cast<Byte *>(&inData[0]);
    
    int res = inflateInit(&stream);
    if (res != Z_OK)
        return output;

    unsigned char ChunkOut[CHUNK_SIZE];
    do {
        stream.avail_out = sizeof(ChunkOut);
        stream.next_out = ChunkOut;
        res = inflate(&stream, Z_FINISH);
        unsigned compressed = sizeof(ChunkOut) - stream.avail_out;
        unsigned oldsize = output.size();
        output.resize(oldsize + compressed);
        memcpy(&output[oldsize], ChunkOut, compressed);
    }
    while (stream.avail_out == 0);
    inflateEnd(&stream);

    return output;
}



OPZ::OPZ():
verbose(0),
m_level(0.0f), 
// m_active_address(0),
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

const std::vector<unsigned char>* OPZ::getInitMsg() { return &initial_message; }
const std::vector<unsigned char>* OPZ::getHeartBeat() { return &master_heartbeat; }
const std::vector<unsigned char>* OPZ::getConfigCmd() { return &config_cmd; };

std::string& OPZ::toString( pattern_page_parameter_id _id ) { return pattern_page_parameter_name[_id]; } 
std::string& OPZ::toString( page_parameter_id _id ) { return page_parameter_name[_id]; }
std::string& OPZ::toString( track_id _id ) { return track_name[_id]; }
std::string& OPZ::toString( page_id  _id ) { return page_name[_id]; }
std::string& OPZ::toString( mic_fx_id _id ) { return mic_fx_name[_id]; }
std::string& OPZ::toString( note_style_id  _id ) { return note_style_name[_id]; }
std::string& OPZ::toString( event_id _id ) { return event_name[_id]; }
std::string OPZ::toString( midi_id _id ) { 
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

void OPZ::process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData) {
    OPZ *device = static_cast<OPZ*>(_userData);

    if (_message->at(0) == SYSEX_HEAD)
        device->process_sysex(_message);
    else
        device->process_event(_message);
}

void OPZ::process_sysex(std::vector<unsigned char>* _message){

    // if (verbose > 1 && verbose < 4)
    //     std::cout << printHex(&_message->at(0), _message->size()) << "(" << _message->size() << " bytes)" << std::endl;

    const sysex_header &header = (const sysex_header&)_message->at(0);
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
    size_t data_length = _message->size()-sizeof(sysex_header)-1;
    unsigned char *data = new unsigned char[data_length];

    // decode 7bit into raw 8bit
    size_t length = decode(&_message->at(sizeof(sysex_header)), data_length, data);
    
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

            CAST_MESSAGE(track_change, ti);
            // TODO

            if (verbose > 2) {
                std::cout << "   value type: " << toString((pattern_page_parameter_id)ti.value_type) << std::endl;
                std::cout << "   value:      " << ((int)ti.value / 255.0f) << std::endl;
            }

        } break;

        case 0x03: {
            // Keyboard Setting ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#03-keyboard-setting )
            if (verbose)
                printf("Msg %02X (Keyboard Setting)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
 
            if (data[1] > 15) {
                if (!m_play && m_event_enable) 
                    m_event(PLAY_CHANGE, 1);
                m_play = true;
                data[1] -= 16;
            }
            else {
                if (m_play && m_event_enable)
                    m_event(PLAY_CHANGE, 0);
                m_play = false;
            }

            track_id t =  (track_id)data[1];
            if (m_active_track != t)
                if (m_event_enable)
                    m_event(TRACK_CHANGE, (int)m_active_track);

            m_active_track = t;

            m_octave[(size_t)m_active_track] = (int8_t)data[0];
            if (m_event_enable)
                m_event(OCTAVE_CHANGE, m_octave[(size_t)m_active_track]);
            
        } break;

        case 0x04: {
            if (verbose)
                printf("Msg %02X (IN/OUT?)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            float   volume = data[1]/ 255.0f;
            float   mic_level = (data[2]%16) / 15.0f;
            mic_fx_id mic_fx = (mic_fx_id)(data[2]/16);
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
                // printf("    4:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[4]));
                // printf("    5:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[5]));
                // printf("    6:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[6]));
                // printf("    7:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[7]));
                // printf("    8:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[8]));
                // printf("    9:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[9]));
                // printf("    A:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[10]));
                // printf("    B:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[11]));
                // printf("    C:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[12]));
                // printf("    D:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[13]));
                // printf("    E:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[14]));
                // printf("    F:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(data[15]));
            }

        } break;

        case 0x06: {
            // Button States ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#06-button-states )
            if (verbose)
                printf("Msg %02X (Button States)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;

            CAST_MESSAGE(key_state, ki);
            memcpy(&(m_key_prev_state), &m_key_state, sizeof(m_key_state));
            memcpy(&(m_key_state), &ki, sizeof(m_key_state));

            if (m_active_page != (page_id)m_key_state.page)
                if (m_event_enable)
                    m_event(PAGE_CHANGE, (int)m_active_page);
        
            m_active_page = (page_id)m_key_state.page;

            if (m_event_enable) {
                if (verbose > 2) {
                    printf( "   bit1 1-6:  %i %i %i %i %i %i\n", m_key_state.bit11, m_key_state.bit12, m_key_state.bit13, m_key_state.bit14, m_key_state.bit15, m_key_state.bit16);
                    printf( "   page:      %i\n", m_key_state.page);
                    printf( "   step:      %i\n", m_key_state.step);
                    printf( "   shift:     %i\n", m_key_state.shift);
                    printf( "   tempo:     %i\n", m_key_state.tempo);
                    printf( "   mixer:     %i\n", m_key_state.mixer);
                    printf( "   bit3 1-5:  %i %i %i %i %i\n", m_key_state.bit31, m_key_state.bit32, m_key_state.bit33, m_key_state.bit34, m_key_state.bit35);
                    printf( "   screen:    %i\n", m_key_state.screen);
                    printf( "   stop:      %i\n", m_key_state.stop);
                    printf( "   record:    %i\n", m_key_state.record);
                    printf( "   track:     %i\n", m_key_state.track);
                    printf( "   project:   %i\n", m_key_state.project);
                    printf( "   bit4 3-8:  %i %i %i %i %i %i\n", m_key_state.bit43, m_key_state.bit44, m_key_state.bit45, m_key_state.bit46, m_key_state.bit47, m_key_state.bit48);
                }

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

                if (m_key_state.step < 16) m_event( event_id( 6+(int)m_key_state.step ), 1);
                if (m_key_prev_state.step < 16 && m_key_state.step >= 16) m_event( event_id( 6+(int)m_key_prev_state.step ), 0);

                if (m_key_state.record) m_event( KEY_RECORD, 1);
                if (m_key_prev_state.record && !m_key_state.record) m_event( KEY_RECORD, 0);

                // TODO: KEY_PLAY

                if (m_key_state.stop) m_event( KEY_STOP, 1);
                if (m_key_prev_state.stop && !m_key_state.stop) m_event( KEY_STOP, 0);

                // NOTE: MINUX / PLUS -> OCTAVE_CHANGE

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

            CAST_MESSAGE(sequence_change, si);

            if (m_active_project != si.project)
                if (m_event_enable)
                    m_event(PROJECT_CHANGE, (int)m_active_project);
            

            if (m_active_pattern)
                if (m_event_enable)
                    m_event(PATTERN_CHANGE, (int)m_active_pattern); 
            
            m_active_project = (si.project)%16;
            m_active_pattern = (si.pattern)%16;
            // m_active_address = si.address;  // project + pattern
            m_active_step = 0;

            if (verbose > 2) {
                printf("    pattern:    %02X (pattern %i)\n", m_active_pattern, m_active_pattern + 1);
                printf("    unknown:    %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X\n", 
                                        si.unknown1[0], si.unknown1[1], si.unknown1[2], si.unknown1[3],
                                        si.unknown1[4], si.unknown1[5], si.unknown1[6], si.unknown1[7],
                                        si.unknown1[8], si.unknown1[9], si.unknown1[10], si.unknown1[11],
                                        si.unknown1[12], si.unknown1[13], si.unknown1[14], si.unknown1[15]);
                // printf("    unknown10:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[0]));
                // printf("    unknown11:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[1]));
                // printf("    unknown12:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[2]));
                // printf("    unknown13:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[3]));
                // printf("    unknown14:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[4]));
                // printf("    unknown15:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[5]));
                // printf("    unknown16:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[6]));
                // printf("    unknown17:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[7]));
                // printf("    unknown18:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[8]));
                // printf("    unknown19:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[9]));
                // printf("    unknown1A:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[10]));
                // printf("    unknown1B:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[11]));
                // printf("    unknown1C:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[12]));
                // printf("    unknown1D:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[13]));
                // printf("    unknown1E:  %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[14]));
                // printf("    unknown1F:   %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown1[15]));
                printf("    address:    %02X (offset %i bytes)\n", si.address, si.address);
                printf("    unknown2:   %02X\n", si.unknown2);
                // printf("    unknown2:   %c %c %c %c  %c %c %c %c\n", BYTE_TO_BINARY(si.unknown2));
                printf("    project:    %02X (project %i)\n", m_active_project, m_active_project + 1);
            }

        } break;

        case 0x09: {
            // Pattern ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#09-pattern )
            if (verbose)
                printf("Msg %02X (Pattern)\n", header.parm_id);
            
            std::vector<unsigned char> decompressed = decompress(&data[6], length-6);

            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, 6) << "(" << 6 << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }

            const pattern_chunck & pi = (const pattern_chunck &)decompressed[0];
            memcpy(&m_project.pattern[(size_t)m_active_pattern].track_param[0], &pi.track_param[0], sizeof(uint8_t) * decompressed.size());

            if (verbose > 2) {
                printf("   ???????:     %02X %02X  %02X %02X  %02X\n", data[0], data[1], data[2], data[3], data[4]);
                for (size_t i = 0; i < 7; i++) {
                    printf("   %7s:     plug: 0x%02X, step_count: %03i, unknown1: 0x%02X, step_length: %03i, quantize: %03i, note_style: %03i, note_length: %03i, unknown2: 0x%02X 0x%02X \n", 
                        toString((track_id)i).c_str(),
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].plug,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].step_count,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].unknown1,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].step_length,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].quantize,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].note_style,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].note_length,
                        m_project.pattern[(size_t)m_active_pattern].track_param[i].unknown2[0], m_project.pattern[(size_t)m_active_pattern].track_param[i].unknown2[1]
                    );
                }
            }

        } break;

        case 0x0c: {
            // Global Data ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#0c-global-data )
            if (verbose)
                printf("Msg %02X (Global Data)\n", header.parm_id);

            // Global data is compressed using zlib
            std::vector<unsigned char> decompressed = decompress(data, length);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            
            const project & pi = (const project &)decompressed[0];

            memcpy(&m_project, &pi, sizeof(uint8_t) * decompressed.size());// sizeof(pi));

            if (verbose > 2) {
                printf("    drum level:     %i\n", pi.drum_level);
                printf("    synth level:    %i\n", pi.synth_level);
                printf("    pinch level:    %i\n", pi.punch_level);
                printf("    master level:   %i\n", pi.master_level);
                printf("    project tempo:  %i\n", pi.tempo);
                printf("    swing:          %i\n", pi.swing);
                printf("    metronome_level:%i\n", pi.metronome_level);
                printf("    metronome_sound:%i\n", pi.metronome_sound);
            }

        } break;

        case 0x0e: {
            // Sound preset ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#0e-sound-preset )
            if (verbose)
                printf("Msg %02X (Track Parameter)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "    (" << length << " bytes)" << std::endl;

            if (m_active_track != (track_id)(data[0]%16)) {
                m_active_track = (track_id)(data[0]%16);
                if (m_event_enable)
                    m_event(TRACK_CHANGE, (int)m_active_track);
            }

            const page_parameter &si = (const page_parameter &)data[1];
            memcpy(&m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track], &si, sizeof(page_parameter));

            if (m_event_enable)
                m_event(PARAMETER_CHANGE, 1);
                
            if (verbose > 2) {
                printf( " Proj. %i, pattern %i, track %s\n", m_active_project, m_active_pattern, toString(m_active_track).c_str());
                printf( "   param1:     %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].param1);
                printf( "   param2:     %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].param2);
                printf( "   attack:     %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].attack);
                printf( "   decay:      %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].decay);
                printf( "   ustain:     %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].sustain);
                printf( "   release:    %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].release);
                printf( "   fx1:        %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].fx1);
                printf( "   fx2:        %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].fx2);
                printf( "   filter:     %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].filter);
                printf( "   resonance:  %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].resonance);
                printf( "   pan:        %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].pan);
                printf( "   level:      %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].level);
                printf( "   portamendo: %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].portamento);
                printf( "   lfo_depth:  %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].lfo_depth);
                printf( "   lfo_speed:  %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].lfo_speed);
                printf( "   lfo_value:  %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].lfo_value);
                printf( "   lfo_shape:  %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].lfo_shape);
                printf( "   note_style: %i\n", m_project.pattern[m_active_pattern].page_param[(size_t)m_active_track].note_style);
            }
        } break;

        case 0x10: {
            // Compressed MIDI Config ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#10-zlib-compressed-midi-config )
            if (verbose)
                printf("Msg %02X (Compressed MIDI Config)\n", header.parm_id);

            std::vector<unsigned char> decompressed = decompress(&data[0], length);

            if (verbose > 1 && verbose < 4) {
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
        } break;

        case 0x12: {
            // Sound State ( https://github.com/hyphz/opzdoc/wiki/MIDI-Protocol#12-sound-state )
            if (verbose)
                printf("Msg %02X (Sound State)\n", header.parm_id);

            if (verbose > 1 && verbose < 4)
                std::cout << "       " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
        } break;

        case 0x16: {
            if (verbose)
                printf("Msg %02X (Sample Data?)\n", header.parm_id);

            size_t offset = 1;
            std::vector<unsigned char> decompressed = decompress(&data[offset], length-offset);
            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, 1) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
        }break;

        default: {
            if (verbose)
                printf("Msg %02X (unknown)\n", header.parm_id);

            size_t offset = 1;
            std::vector<unsigned char> decompressed = decompress(&data[offset], length-offset);
            if (verbose > 1 && verbose < 4) {
                std::cout << "   RAW " << printHex(data, length) << "(" << length << " bytes)" << std::endl;
                std::cout << "   ENC " << printHex(&decompressed[0], decompressed.size()) << "(" << decompressed.size() << " bytes)" << std::endl;
            }
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

    if (m_midi_enable) {
        if (bytes >= 2)
            m_midi((midi_id)status, (size_t)channel, (size_t)_message->at(1), (size_t)_message->at(2));

        else 
            m_midi((midi_id)status, (size_t)channel, 0, 0);
    }
}

float OPZ::getTrackPageParameter(uint8_t _pattern, track_id _track, page_parameter_id _prop) const {
    if (_prop == SOUND_PARAM1)
        return m_project.pattern[_pattern].page_param[_track].param1 / 255.0f;
    else if (_prop == SOUND_PARAM2)
        return m_project.pattern[_pattern].page_param[_track].param2 / 255.0f;
    else if (_prop == SOUND_FILTER)
        return m_project.pattern[_pattern].page_param[_track].filter / 255.0f; 
    else if (_prop == SOUND_RESONANCE)
        return m_project.pattern[_pattern].page_param[_track].resonance / 255.0f;

    // TODO
    //      - I got the names wrong
    else if (_prop == ENVELOPE_ATTACK) // S
        return m_project.pattern[_pattern].page_param[_track].attack / 255.0f;
    else if (_prop == ENVELOPE_DECAY) // A
        return m_project.pattern[_pattern].page_param[_track].decay  / 255.0f;
    else if (_prop == ENVELOPE_SUSTAIN) // H
        return m_project.pattern[_pattern].page_param[_track].sustain / 255.0f; 
    else if (_prop == ENVELOPE_RELEASE) // D
        return m_project.pattern[_pattern].page_param[_track].release / 255.0f; 

    else if (_prop == LFO_DEPTH)
        return m_project.pattern[_pattern].page_param[_track].lfo_depth / 255.0f;
    else if (_prop == LFO_SPEED) // RATE
        return m_project.pattern[_pattern].page_param[_track].lfo_speed / 255.0f;
    else if (_prop == LFO_VALUE) // DEST
        return m_project.pattern[_pattern].page_param[_track].lfo_value / 255.0f; 
    else if (_prop == LFO_SHAPE)
        return m_project.pattern[_pattern].page_param[_track].lfo_shape / 255.0f;

    else if (_prop == SOUND_FX1)
        return m_project.pattern[_pattern].page_param[_track].fx1 / 255.0f;
    else if (_prop == SOUND_FX2)
        return (m_project.pattern[_pattern].page_param[_track].fx2 / 255.0f) * 2.0 - 1.0f;
    else if (_prop == SOUND_PAN)
        return m_project.pattern[_pattern].page_param[_track].pan / 255.0f; 
    else if (_prop == SOUND_LEVEL)
        return m_project.pattern[_pattern].page_param[_track].level / 255.0f;

    else if (_prop == NOTE_LENGTH)
        return (float)m_project.pattern[_pattern].track_param[_track].note_length;

    else if (_prop == NOTE_STYLE)
        return m_project.pattern[_pattern].page_param[_track].note_style / 255.0f;

    else if (_prop == QUANTIZE)
        return (float)m_project.pattern[_pattern].track_param[_track].quantize;

    else if (_prop == PORTAMENTO)
        return m_project.pattern[_pattern].page_param[_track].portamento / 255.0f;

    else if (_prop == STEP_COUNT)
        return (float)m_project.pattern[_pattern].track_param[_track].step_count;

    else if (_prop == STEP_LENGTH)
        return (float)m_project.pattern[_pattern].track_param[_track].step_length;

    return 0.0f;
}

}