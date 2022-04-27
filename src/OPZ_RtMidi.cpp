
#include "opz_rtmidi.h"

namespace T3 {

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

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
m_last_heartbeat(0.0), m_last_time(0.0),
m_connected(false) {
    m_packet_recived_enabled = true;
    m_packet_recived = [&](uint8_t _cmd, uint8_t *_data, size_t _lenght) {
            if (verbose > 2)
                printf("Recieved package %02X %02X %02X %02X \n", _data[0], _data[1], _data[2], _data[3] );

            //              cmd     0            1  2  3  4 
            // OPZ_HEADER   09  00 10           00 56 1F 00   00 78
            // OPZ_HEADER   0B  00 09 00 00     00 56 1F 00   00 00 F7

            std::vector<unsigned char> cmd = {
                SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, 
                0x0B, 0x00, 0x09, 0x00, 0x00, _data[0], _data[1], _data[2], _data[3], 0x00, 0x00, SYSEX_END
                // 0x0B, 0x00, 0x09, 0x00, 0x00, 0x00,     _data[1], _data[2], _data[3], 0x00, 0x00, SYSEX_END
                // 0x0B, 0x00, 0x09, 0x00, 0x00, 0x00,     0x00,     _data[2], _data[3], 0x00, 0x00, SYSEX_END
                // 0x0B, 0x00, 0x09, 0x00, 0x00, 0x00,     0x00,     0x00,     _data[3], 0x00, 0x00, SYSEX_END
            };
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

void opz_rtmidi::keepawake(){
    double now = getTimeSec(time_start);; 
    double delta = now - m_last_time;
    m_last_time = now;

    m_last_heartbeat += delta;

    if (m_last_heartbeat > 1.0) {

        if (m_connected)
            m_out->sendMessage( opz_heartbeat() );
        else
            m_event(NO_CONNECTION, 0);

        m_last_heartbeat = 0.0;
    }

    usleep( 16700 );
}

void opz_rtmidi::midiConfigCmd() {
    if (m_connected)
        m_out->sendMessage( opz_config_cmd() );
}

void opz_rtmidi::sendCmd(unsigned char _cmd) {
    if (!m_connected)
        return;

    std::vector<unsigned char> cmd = {
        SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, _cmd, SYSEX_END
    };
    m_out->sendMessage( &cmd );
}

void opz_rtmidi::sendCmd(unsigned char* _cmd, size_t _length) {
    if (!m_connected)
        return;

    std::vector<unsigned char> cmd = { SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION };
    for (size_t i = 0; i < _length; i++)
        cmd.push_back(_cmd[i]);
    cmd.push_back(SYSEX_END);
    m_out->sendMessage( &cmd );
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
