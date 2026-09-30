// Experimental tool to test whether the OP-Z's active parameter page (1-4)
// can be remotely forced by writing a 0x06 (Button States) SysEx message
// back to the device.
//
// There is no confirmed-safe documentation for the writable byte layout of
// 0x06 beyond one narrow proven gesture (a project+track long-hold that
// toggles autosave). Reading 0x06 is solid (libopz's opz_key_state bitfield
// already drives the whole companion UI), but WRITING it risks latching
// held-button/encoder state if done carelessly.
//
// To keep this bounded: every write here starts from the device's own last
// received opz_key_state (getKeyState()), copies it byte-for-byte, and only
// flips the `page` field before sending - every other bit (held buttons,
// step, shift, etc.) is left exactly as the device last reported it. If the
// device does something unexpected, physically pressing any button on the
// OP-Z will emit a fresh 0x06 that overwrites this tool's guess, so there's
// no way to "stick" a bad state.
#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>

#include "libopz/opz_rtmidi.h"
#include "libopz/tools.h"

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf); // auto-flush every print

    opz::opz_rtmidi device;
    if (!device.connect()) {
        std::cout << "Could not connect to an OP-Z. Is it plugged in and MIDI OUT enabled?\n";
        return 1;
    }
    std::cout << "Connected.\n";

    device.setEventCallback([&](opz::opz_event_id _id, int _value) {
        if (_id == opz::PAGE_CHANGE)
            std::cout << "[event] PAGE_CHANGE reported old page = " << _value << ", device.getActivePageId() now = " << (int)device.getActivePageId() << "\n";
    });

    std::atomic<bool> running(true);
    std::thread heartbeat([&]() {
        while (running.load())
            device.update();
    });

    std::cout <<
        "Commands:\n"
        "  1-4 <enter>   send a 0x06 write attempting to force that page\n"
        "  r <enter>     print the last raw key_state snapshot\n"
        "  q <enter>     quit\n"
        "Press shift + a dial physically on the OP-Z at any point to compare against a real page change.\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty())
            continue;
        char ch = line[0];

        if (ch == 'q')
            break;

        if (ch == 'r') {
            const opz::opz_key_state& ks = device.getKeyState();
            unsigned char raw[4];
            memcpy(raw, &ks, sizeof(raw));
            std::cout << "raw bytes: " << opz::printHex(raw, 4)
                      << " | page=" << (int)ks.page
                      << " shift=" << (int)ks.shift
                      << " track=" << (int)ks.track
                      << " project=" << (int)ks.project
                      << " mixer=" << (int)ks.mixer
                      << " tempo=" << (int)ks.tempo
                      << " step=" << (int)ks.step
                      << "\n";
        }

        if (ch >= '1' && ch <= '4') {
            int target_page = (ch - '1');

            opz::opz_key_state st = device.getKeyState();
            unsigned char before[4];
            memcpy(before, &st, sizeof(before));

            st.page = target_page;
            unsigned char raw[4];
            memcpy(raw, &st, sizeof(raw));
            std::vector<unsigned char> body(raw, raw + 4);

            std::cout << "sending 0x06 with page=" << target_page
                       << " | before: " << opz::printHex(before, 4)
                       << " | after:  " << opz::printHex(raw, 4) << "\n";

            std::vector<unsigned char> frame = device.buildSysex(0x06, body);
            bool ok = device.send(frame);
            std::cout << (ok ? "sent." : "send failed (not connected?).") << "\n";

            std::cout << "device.getActivePageId() now reads: " << (int)device.getActivePageId() << "\n";
        }
    }

    running.store(false);
    heartbeat.join();
    device.disconnect();
    return 0;
}
