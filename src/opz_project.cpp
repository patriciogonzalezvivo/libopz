#include "opz_project.h"

#include <iostream>
#include <fstream>
#include <iomanip>

namespace T3 {

// https://teenage.engineering/guides/op-z/tracks
std::string track_name[] = { 
    "KICK", "SNARE", "PERC", "SAMPLE", "BASS", "LEAD", "ARP", "CHORD", "FX1", "FX2", "TAPE", "MASTER", "PERFORM", "MODULE", "LIGHT", "MOTION", "UNKNOWN" 
};

std::string pattern_parameter_name[] = { 
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

std::string sound_parameter_name[] = { 
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


opz_project::opz_project() {
}

std::string& toString( opz_track_id _id ) { return track_name[_id]; }
std::string& toString( opz_note_style_id  _id ) { return note_style_name[_id]; }
std::string& toString( opz_sound_parameter_id _id ) { return sound_parameter_name[_id]; }
std::string& toString( opz_pattern_parameter_id _id ) { return pattern_parameter_name[_id]; } 

float opz_project::getSoundParameter(uint8_t _pattern, opz_track_id _track, opz_sound_parameter_id _prop) const {
    if (_prop == SOUND_PARAM1)
        return m_project.pattern[_pattern].sound_param[_track].param1 / 255.0f;
    else if (_prop == SOUND_PARAM2)
        return m_project.pattern[_pattern].sound_param[_track].param2 / 255.0f;
    else if (_prop == SOUND_FILTER)
        return m_project.pattern[_pattern].sound_param[_track].filter / 255.0f; 
    else if (_prop == SOUND_RESONANCE)
        return m_project.pattern[_pattern].sound_param[_track].resonance / 255.0f;

    // TODO
    //      - I got the names wrong
    else if (_prop == ENVELOPE_ATTACK) // S
        return m_project.pattern[_pattern].sound_param[_track].attack / 255.0f;
    else if (_prop == ENVELOPE_DECAY) // A
        return m_project.pattern[_pattern].sound_param[_track].decay  / 255.0f;
    else if (_prop == ENVELOPE_SUSTAIN) // H
        return m_project.pattern[_pattern].sound_param[_track].sustain / 255.0f; 
    else if (_prop == ENVELOPE_RELEASE) // D
        return m_project.pattern[_pattern].sound_param[_track].release / 255.0f; 

    else if (_prop == LFO_DEPTH)
        return m_project.pattern[_pattern].sound_param[_track].lfo_depth / 255.0f;
    else if (_prop == LFO_SPEED) // RATE
        return m_project.pattern[_pattern].sound_param[_track].lfo_speed / 255.0f;
    else if (_prop == LFO_VALUE) // DEST
        return m_project.pattern[_pattern].sound_param[_track].lfo_value / 255.0f; 
    else if (_prop == LFO_SHAPE)
        return m_project.pattern[_pattern].sound_param[_track].lfo_shape / 255.0f;

    else if (_prop == SOUND_FX1)
        return m_project.pattern[_pattern].sound_param[_track].fx1 / 255.0f;
    else if (_prop == SOUND_FX2)
        return (m_project.pattern[_pattern].sound_param[_track].fx2 / 255.0f) * 2.0 - 1.0f;
    else if (_prop == SOUND_PAN)
        return m_project.pattern[_pattern].sound_param[_track].pan / 255.0f; 
    else if (_prop == SOUND_LEVEL)
        return m_project.pattern[_pattern].sound_param[_track].level / 255.0f;

    else if (_prop == NOTE_LENGTH)
        return (float)m_project.pattern[_pattern].track_param[_track].note_length;

    else if (_prop == NOTE_STYLE)
        return m_project.pattern[_pattern].sound_param[_track].note_style / 255.0f;

    else if (_prop == QUANTIZE)
        return (float)m_project.pattern[_pattern].track_param[_track].quantize;

    else if (_prop == PORTAMENTO)
        return m_project.pattern[_pattern].sound_param[_track].portamento / 255.0f;

    else if (_prop == STEP_COUNT)
        return (float)m_project.pattern[_pattern].track_param[_track].step_count;

    else if (_prop == STEP_LENGTH)
        return (float)m_project.pattern[_pattern].track_param[_track].step_length;

    return 0.0f;
}

bool opz_project::saveAsTxt(const std::string& _filename) {
    std::ofstream myfile (_filename);
    if (myfile.is_open()) {
        myfile << "opz_pattern_chain pattern_chain[16]\n";
        for (size_t i = 0; i < 16; i++) {
            for (size_t t = 0; t < 32; t++)
                myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern_chain[i].pattern[t] ) << " ";
            myfile << "\n";
        }
        myfile << "\n";

        myfile << "uint8_t drum_level, synth_level, punch_level, master_level, tempo;\n";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.drum_level ) << " ";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.synth_level ) << " ";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.punch_level ) << " ";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.master_level ) << " ";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.tempo ) << "\n\n";

        myfile << "uint8_t unknown1[44]\n";
        for (size_t i = 0; i < 44; i++)
            myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.unknown1[i] ) << " ";
        myfile << "\n\n";

        myfile << "uint8_t swing, metronome_level, metronome_sound;\n";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.swing ) << " ";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.metronome_level ) << " ";
        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.metronome_sound ) << "\n\n";

        myfile << "uint32_t unknown2;\n";
        myfile << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << (0xFFFFFFFF & m_project.unknown2 ) << "\n\n";

        for (size_t i = 0; i < 16; i++) {
            myfile << "opz_pattern pattern[16]\n";
            myfile << "pattern[" << std::dec << i <<"]\n";

                myfile << "\topz_track_parameter track_param[16];\n";
                for (size_t j = 0; j < 16; j++) {
                    myfile << "\ttrack_param[" << std::dec << j <<"];\n";
                    myfile << "\t\tuint32_t plug;\n\t\t";
                    myfile << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << (0xFFFFFFFF & m_project.pattern[i].track_param[j].plug ) << "\n\n";

                    myfile << "\t\tuint8_t step_count, unknown1, step_length, quantize, note_style, note_length;\n\t\t";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].step_count ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].unknown1 ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].step_length ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].quantize ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].note_style ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].note_length ) << "\n\n";

                    myfile << "\t\tuint8_t unknown2[2]\n\t\t";
                    for (size_t k = 0; k < 2; k++)
                        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].track_param[j].unknown2[k] ) << " ";
                    myfile << "\n\n";
                }

                myfile << "\topz_note note[880];\n";
                for (size_t j = 0; j < 880; j++) {
                    myfile << "\tnote[" << std::setfill('0') << std::setw(3) << std::dec << j <<"] = { ";
                    myfile << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << (0xFFFFFFFF & m_project.pattern[i].note[j].duration ) << " ";

                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].note[j].note ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].note[j].velocity ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].note[j].micro_adjustment ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].note[j].age ) << " } \n";
                }

                myfile << "\topz_step step[256];\n";
                for (size_t j = 0; j < 256; j++) {
                    myfile << "\tstep[" << std::dec << j <<"]; = { ";

                    // myfile << "\t\tuint16_t components_bitmask;\n\t\t";
                    myfile << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << (0xFFFF & m_project.pattern[i].step[j].components_bitmask ) << " ";
                    myfile << "  ";

                    // myfile << "\t\tuint8_t components_parameters[18]\n\t\t";
                    for (size_t k = 0; k < 18; k++)
                        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].step[j].components_parameters[k] ) << " ";
                    myfile << " ";

                    // myfile << "\t\tuint8_t components_locked_values[18]\n\t\t";
                    for (size_t k = 0; k < 18; k++)
                        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].step[j].components_locked_values[k] ) << " ";
                    myfile << "  ";

                    // myfile << "\t\tuint8_t parameter_mask[18]\n\t\t";
                    for (size_t k = 0; k < 18; k++)
                        myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].step[j].parameter_mask[k] ) << " ";
                    myfile << "}\n";
                }

                myfile << "\topz_sound_parameter sound_param[16];\n";
                for (size_t j = 0; j < 16; j++) {
                    myfile << "\tsound_param[" << std::dec << j <<"];\n";
                    myfile << "\t\tuint8_t param1, param2, attack, decay, sustain, release, fx1, fx2, filter, resonance, pan, level, filter, resonance, pan, level, portamento, lfo_depth, lfo_speed, lfo_value, lfo_shape, note_style;\n\t\t";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].param1 ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].param2 ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].attack ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].decay ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].sustain ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].release ) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].fx1) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].fx2) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].filter) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].resonance) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].pan) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].level) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].portamento) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].lfo_depth) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].lfo_speed) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].lfo_value) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].lfo_shape) << " ";
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].sound_param[j].note_style ) << "\n\n";

                }

                myfile << "\tuint8_t mute[40]; \n\t";
                for (size_t j = 0; j < 44; j++)
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].mute[j] ) << " ";
                myfile << "\n\n";

                myfile << "\tuint16_t send_tape;\n\t";
                myfile << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << (0xFFFF & m_project.pattern[i].send_tape ) << "\n\n";

                myfile << "\tuint16_t send_master;\n\t";
                myfile << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << (0xFFFF & m_project.pattern[i].send_master ) << "\n\n";

                myfile << "\tuint8_t active_mute_group;\n\t";
                myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].active_mute_group ) << "\n\n";

                myfile << "\tuint8_t unknown[3]\n\t";
                for (size_t j = 0; j < 3; j++)
                    myfile << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & m_project.pattern[i].unknown[j]  ) << " ";
                myfile << "\n\n";

            myfile << "\n\n";
        }

        myfile.close();
    }

    return true;
}

}