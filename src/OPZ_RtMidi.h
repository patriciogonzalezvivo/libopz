#pragma once

#include "OPZ.h"
#include "rtmidi/RtMidi.h"

namespace T3 {

class OPZ_RtMidi : public OPZ {
public:
    OPZ_RtMidi();

    bool connect();
    void disconnect();

    void keepawake();

private:

    RtMidiIn*       m_in;
    RtMidiOut*      m_out;

    double          m_last_heartbeat;
    double          m_last_time;

};

}