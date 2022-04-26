#pragma once

#include "rtmidi/RtMidi.h"
#include "OPZ.h"

namespace T3 {

class OPZ_RtMidi : public OPZ {
public:
    OPZ_RtMidi();

    bool connect();
    void disconnect();
    bool isConnected() const { return m_connected; }

    void keepawake();
    void midiConfigCmd();

    void sendCmd(unsigned char _cmd);

private:
    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);

    RtMidiIn*       m_in;
    RtMidiOut*      m_out;

    double          m_last_heartbeat;
    double          m_last_time;

    bool            m_connected;
};

}
