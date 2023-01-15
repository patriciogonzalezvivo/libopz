
#include "libopz/opz_rtmidi.h"

namespace opz {

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "libopz/tools.h"

#ifdef PLATFORM_WINDOWS
const int CLOCK_MONOTONIC = 0;
int clock_gettime(int, struct timespec* spec)      //C-file part
{
    __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime -= 116444736000000000i64;  //1jan1601 to 1jan1970
    spec->tv_sec = wintime / 10000000i64;           //seconds
    spec->tv_nsec = wintime % 10000000i64 * 100;      //nano-seconds
    return 0;
}
#endif

static timespec time_start;
double getTimeSec(const timespec &time_start) {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec temp;
    if ((now.tv_nsec-time_start.tv_nsec)<0) {
        temp.tv_sec = now.tv_sec-time_start.tv_sec-1;
        temp.tv_nsec = 1000000000+now.tv_nsec-time_start.tv_nsec;
    }
    else {
        temp.tv_sec = now.tv_sec-time_start.tv_sec;
        temp.tv_nsec = now.tv_nsec-time_start.tv_nsec;
    }
    return double(temp.tv_sec) + double(temp.tv_nsec/1000000000.);
}

opz_rtmidi::opz_rtmidi() : 
m_in(NULL), m_out(NULL),
m_last_heartbeat(0.0), m_last_time(0.0), m_last_step(0.0),
m_connected(false) {
    m_packet_recived_enabled = true;
    m_packet_recived = [&](uint8_t _cmd, uint8_t *_data, size_t _lenght) {
            std::vector<unsigned char> cmd = opz_confirm_package_cmd(_data, _lenght);
            if (m_connected)
                m_out->sendMessage( &cmd );
    };

}

bool opz_rtmidi::connect() {

    m_in = new RtMidiIn();
    unsigned int nPorts = m_in->getPortCount();
    bool in_connected = false;
    for(unsigned int i = 0; i < nPorts; i++) {
        std::string name = m_in->getPortName(i);
        if (name.rfind("OP-Z", 0) == 0) {
            try {
                m_in = new RtMidiIn(RtMidi::Api(0), "opz_dump");
                m_in->openPort(i, name);
                m_in->ignoreTypes(false, false, true);
                m_in->setCallback(process_message, this);
                in_connected = true;
                break;
            } catch(RtMidiError &error) {
                error.printMessage();
            }
        }
    }

    if (in_connected) {
        m_out = new RtMidiOut();
        nPorts = m_out->getPortCount();
        for(unsigned int i = 0; i < nPorts; i++) {
            std::string name = m_out->getPortName(i);
            if (name.rfind("OP-Z", 0) == 0) {
                try {
                    clock_gettime(CLOCK_MONOTONIC, &time_start);

                    m_out = new RtMidiOut(RtMidi::Api(0), "opz_dump");
                    m_out->openPort(i, name);
                    m_out->sendMessage( opz_init_msg() );

                    m_connected = true;
                    return true;
                }
                catch(RtMidiError &error) {
                    error.printMessage();
                }
            }
        }
    }

    disconnect();
    return false;
}

void opz_rtmidi::update(){
    if (!m_connected)
        return;

    double now = getTimeSec(time_start);; 
    double delta = now - m_last_time;
    m_last_time = now;

    // Keep the connecting with the opz alive
    m_last_heartbeat += delta;
    if (m_last_heartbeat > 1.0) {
        m_out->sendMessage( opz_heartbeat() );
        m_last_heartbeat = 0.0;
    }

    if (m_play) {
        m_last_step += delta;

        if ( m_last_step >= getBeatPerStep() ) {
            m_last_step = 0.0;

            size_t total_steps = getActiveTrackParameters().step_count * getActiveTrackParameters().step_length;
            // m_active_step = (m_active_step + 1) % total_steps;
            m_active_step++;
            if (m_event_enable)
                m_event(STEP_CHANGE, m_active_step);
        }


    }

    usleep( 16700 );
}

void opz_rtmidi::process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData) {
    opz_device *device = static_cast<opz_device*>(_userData);
    device->process_message(&_message->at(0), _message->size());
}

void opz_rtmidi::disconnect() {
    if (m_in) {
        m_in->cancelCallback();
        m_in->closePort();
        delete m_in;
        m_in = NULL;
    }

    if (m_out) {
        m_out->closePort();
        delete m_out;
        m_out = NULL;
    }

    m_connected = false;
}

}
