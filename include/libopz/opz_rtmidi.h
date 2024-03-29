#pragma once

#include "rtmidi/RtMidi.h"
#include "opz_device.h"

namespace opz {

class opz_rtmidi : public opz_device {
public:
    opz_rtmidi();

    bool            connect();
    void            disconnect();
    bool            isConnected() const { return m_connected; }

    double          getTimeHeadPosition() const { return m_active_step + m_last_step / getBeatPerStep(); }

    void            update();

private:
    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);

    RtMidiIn*       m_in;
    RtMidiOut*      m_out;

    double          m_last_heartbeat;
    double          m_last_time;
    double          m_last_step;

    bool            m_connected;
};

}
