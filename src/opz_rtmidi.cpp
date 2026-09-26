
#include "libopz/opz_rtmidi.h"
#include "libopz/tools.h"
#include <algorithm>

namespace opz {

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

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
            if (m_connected) {
                std::lock_guard<std::mutex> lock(m_out_mutex);
                m_out->sendMessage( &cmd );
            }
    };

}

bool opz_rtmidi::connect() {

    bool in_connected = false;
    try {
        m_in = new RtMidiIn();
        unsigned int nPorts = m_in->getPortCount();
        for(unsigned int i = 0; i < nPorts; i++) {
            std::string name = m_in->getPortName(i);
            if (name.rfind("OP-Z", 0) == 0) {
                try {
                    RtMidiIn* in = new RtMidiIn(RtMidi::Api(0), "opz_dump");
                    in->openPort(i, name);
                    in->ignoreTypes(false, false, true);
                    in->setCallback(process_message, this);
                    delete m_in;
                    m_in = in;
                    in_connected = true;
                    break;
                } catch(RtMidiError &error) {
                    error.printMessage();
                }
            }
        }
    } catch(RtMidiError &error) {
        error.printMessage();
    }

    if (in_connected) {
        try {
            m_out = new RtMidiOut();
            unsigned int nPorts = m_out->getPortCount();
            for(unsigned int i = 0; i < nPorts; i++) {
                std::string name = m_out->getPortName(i);
                if (name.rfind("OP-Z", 0) == 0) {
                    try {
                        clock_gettime(CLOCK_MONOTONIC, &time_start);

                        RtMidiOut* out = new RtMidiOut(RtMidi::Api(0), "opz_dump");
                        out->openPort(i, name);
                        out->sendMessage( opz_init_msg() );
                        delete m_out;
                        m_out = out;

                        m_connected = true;
                        return true;
                    }
                    catch(RtMidiError &error) {
                        error.printMessage();
                    }
                }
            }
        } catch(RtMidiError &error) {
            error.printMessage();
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
        std::lock_guard<std::mutex> lock(m_out_mutex);
        m_out->sendMessage( opz_heartbeat() );
        m_last_heartbeat = 0.0;
    }

    if (m_play) {
        m_last_step += delta;

        if ( m_last_step >= getBeatPerStep() ) {
            m_last_step -= getBeatPerStep();

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

bool opz_rtmidi::send(const std::vector<unsigned char>& _msg) {
    if (!m_connected || m_out == NULL)
        return false;
    std::lock_guard<std::mutex> lock(m_out_mutex);
    m_out->sendMessage(&_msg);
    return true;
}

bool opz_rtmidi::send(unsigned char* _data, size_t _length) {
    std::vector<unsigned char> msg(_data, _data + _length);
    return send(msg);
}

bool opz_rtmidi::sendTrackSelect(opz_track_id _track, int8_t _octave) {
    uint8_t play_nibble = m_play ? 0x10 : 0x00;
    std::vector<unsigned char> body = {
        (unsigned char)_octave,
        (unsigned char)(play_nibble | ((uint8_t)_track & 0x0F))
    };
    return send(buildSysex(0x03, body));
}

std::vector<unsigned char> opz_rtmidi::buildSysex(uint8_t _parm_id, const std::vector<unsigned char>& _body) {
    std::vector<unsigned char> out = { SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2], OPZ_MAX_PROTOCOL_VERSION, _parm_id };
    // worst case 7-bit encoding grows the body by ~8/7; reserve generously
    out.resize(out.size() + _body.size() * 2 + 8);
    size_t enc_len = encode(&_body[0], _body.size(), &out[6]);
    out.resize(6 + enc_len);
    out.push_back(SYSEX_END);
    return out;
}

int opz_rtmidi::sendPattern(const opz_pattern* _bank16, uint8_t _address, uint16_t _id) {
    if (!m_connected || m_out == NULL)
        return 0;

    // 1) Compress the raw 16-pattern bank (single-member zlib), reusing the shared helper.
    const unsigned char* raw = (const unsigned char*)_bank16;
    size_t raw_len = sizeof(opz_pattern) * 16;
    std::vector<unsigned char> comp = compress(raw, raw_len);
    if (comp.empty())
        return 0;

    // 2) Packetize into 0x09 frames (final is 0x0a), header = 6 bytes:
    //    [address, 0x00, id_lo, id_hi, pkt_lo, pkt_hi] + zlib chunk.
    const size_t chunk = 178;
    size_t npkt = (comp.size() + chunk - 1) / chunk;
    int acked = 0;
    for (size_t i = 0; i < npkt; i++) {
        size_t off = i * chunk;
        size_t len = std::min(chunk, comp.size() - off);
        bool last = (i == npkt - 1);

        std::vector<unsigned char> body = {
            _address, 0x00,
            (unsigned char)(_id & 0xFF), (unsigned char)((_id >> 8) & 0xFF),
            (unsigned char)(i & 0xFF), (unsigned char)((i >> 8) & 0xFF)
        };
        body.insert(body.end(), comp.begin() + off, comp.begin() + off + len);

        std::vector<unsigned char> frame = buildSysex(last ? 0x0a : 0x09, body);

        uint32_t before = m_ack_count;
        {
            std::lock_guard<std::mutex> lock(m_out_mutex);
            m_out->sendMessage(&frame);
        }

        // Flow control: wait for this packet's 0x0b ACK (device applies on the last
        // data packets and may not ACK the 0x0a terminator, so don't block on it).
        if (!last) {
            for (int w = 0; w < 40 && m_ack_count <= before; w++)
                usleep(15000);
            if (m_ack_count > before) acked++;
        }
    }
    return acked;
}

bool opz_rtmidi::requestPatternSync(double _timeout_sec) {
    if (!m_connected || m_out == NULL)
        return false;

    uint32_t before = m_dump_count;

    // Also request config/global data (0x0F) so we get tempo, levels, etc.
    {
        std::lock_guard<std::mutex> lock(m_out_mutex);
        m_out->sendMessage(opz_config_cmd());
    }

    std::vector<unsigned char> req = {
        SYSEX_HEAD, OPZ_VENDOR_ID[0], OPZ_VENDOR_ID[1], OPZ_VENDOR_ID[2],
        OPZ_MAX_PROTOCOL_VERSION, 0x08, SYSEX_END };
    {
        std::lock_guard<std::mutex> lock(m_out_mutex);
        m_out->sendMessage(&req);
    }

    // Spin until a fresh dump lands, keeping the heartbeat alive via update().
    double waited = 0.0;
    while (waited < _timeout_sec && m_dump_count == before) {
        update();           // sends heartbeat as needed, sleeps ~16.7ms
        waited += 0.0167;
    }
    return m_dump_count != before;
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
