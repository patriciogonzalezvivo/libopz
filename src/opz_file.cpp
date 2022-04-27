#include "opz_file.h"

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


opz_file::opz_file() {

}

std::string& toString( opz_track_id _id ) { return track_name[_id]; }
std::string& toString( opz_note_style_id  _id ) { return note_style_name[_id]; }
std::string& toString( opz_page_parameter_id _id ) { return page_parameter_name[_id]; }
std::string& toString( opz_pattern_parameter_id _id ) { return pattern_parameter_name[_id]; } 

float opz_file::getTrackPageParameter(uint8_t _pattern, opz_track_id _track, opz_page_parameter_id _prop) const {
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