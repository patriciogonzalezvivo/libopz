#pragma once

#include "rtmidi/RtMidi.h"
#include "opz_device.h"

namespace T3 {

class opz_rtmidi : public opz_device {
public:
    opz_rtmidi();

    bool connect();
    void disconnect();
    bool isConnected() const { return m_connected; }

    void keepawake();
    void midiConfigCmd();

    void sendCmd(unsigned char _cmd);
    void sendCmd(unsigned char* _cmd, size_t _length);

private:
    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);

    RtMidiIn*       m_in;
    RtMidiOut*      m_out;

    double          m_last_heartbeat;
    double          m_last_time;

    bool            m_connected;
};

}
