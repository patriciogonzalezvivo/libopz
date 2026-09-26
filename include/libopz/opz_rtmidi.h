#pragma once

#include <mutex>

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

    // --- Outbound ---
    // Send a fully-formed MIDI message (e.g. a complete F0..F7 SysEx frame) verbatim.
    bool            send(const std::vector<unsigned char>& _msg);
    bool            send(unsigned char* _data, size_t _length);

    // Wrap a *raw* (8-bit, un-encoded) body in an OP-Z SysEx frame:
    //   F0 00 20 76 01 <parm_id> <7bit-encoded body> F7
    // Returns the complete frame ready to send().
    std::vector<unsigned char> buildSysex(uint8_t _parm_id, const std::vector<unsigned char>& _body);

    // Tell the device to switch to a different track (and optionally octave).
    // Sends a writable 0x03 (Keyboard Setting) SysEx message.
    bool            sendTrackSelect(opz_track_id _track, int8_t _octave = 0);

    // Tell the device to switch to a different project. Requires a 0x07
    // (Sequencer Settings) payload to already have been seen at least once
    // (hasChainPayload()) - the write starts from that last raw 20-byte chain
    // state and only flips the project byte, since the rest of the payload
    // (chain sequence/length) isn't independently reconstructed here. Returns
    // false without sending if no baseline has been received yet.
    bool            sendProjectSelect(uint8_t _project);

    // Per-track mixer level/mute (0x12, MixerState). Both require a baseline
    // 0x12 to already have been received (hasMixerState()) - the write starts
    // from that last-known 16-level + mute-mask snapshot and only flips the
    // target track's field, leaving every other track's level/mute untouched.
    // Return false without sending if no baseline has been received yet.
    bool            sendMixerTrackLevel(opz_track_id _track, uint8_t _level);
    bool            sendMixerToggleMute(opz_track_id _track);

    // Push a full 16-pattern bank to the device as a 0x09/0x0a stream (a live write).
    // Compresses + packetizes, waits for each 0x0b ACK. address/id come from the last
    // received dump (getPatternAddress()/getPatternId()). Returns packets ACK'd.
    int             sendPattern(const opz_pattern* _bank16, uint8_t _address, uint16_t _id);

    // Request a fresh full-bank dump and block (keeping heartbeats flowing) until it
    // lands in m_project or the timeout elapses. Call right before sendPattern so the
    // pushed bank reflects current device state (avoids reverting on-device edits).
    // Returns true if a new dump arrived within the timeout.
    bool            requestPatternSync(double _timeout_sec = 2.0);

private:
    static void     process_message(double _deltatime, std::vector<unsigned char>* _message, void* _userData);

    RtMidiIn*       m_in;
    RtMidiOut*      m_out;

    // RtMidiOut::sendMessage() is called both from the RtMidi input callback
    // thread (per-package ACKs during a pattern dump) and from the main/UI
    // thread (heartbeat, sendCmd, sendPattern); RtMidi does not guarantee
    // sendMessage() is safe to call concurrently from multiple threads on the
    // same port, so every call is serialized through this mutex.
    std::mutex      m_out_mutex;

    double          m_last_heartbeat;
    double          m_last_time;
    double          m_last_step;

    bool            m_connected;
};

}
