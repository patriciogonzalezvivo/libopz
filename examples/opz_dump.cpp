#include <iostream>
#include <fstream>

#include "libopz/opz_rtmidi.h"

std::string version = "0.1";
std::string name = "opz_dump";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

int main(int argc, char** argv) {
    opz::opz_rtmidi device;
    device.connect();
    // device.verbose = 1; // message type
    // device.verbose = 2; // hex values
    device.verbose = 3; // interpreted message
    // device.verbose = 4; // interpreted message w no hex values

    // // Listen to key events (no cc, neighter notes)
    // device.setEventCallback( [&](opz::opz_event_id _id, int _value) {
    //     if (_id == opz::KEY_SCREEN && _value == 0) {
    //         std::vector<unsigned char> cmd = {
    //             // 0x15, 0x00, 0x00, 0x00, 0x00, 0x00
    //             0x0F
    //         };
    //         device.sendCmd(&cmd[0], cmd.size());
    //     } 
    // } );

    // device.setMidiCallback( [](opz::midi_id _id, size_t channel, size_t _key, size_t _value) {
    //     std::cout << opz::OPZ::toString(_id) << ", chanel " << channel << ", key " << _key << ", value " << _value << std::endl;
    // } );

    while (true)
        device.update();
    
    device.disconnect();

    exit(0);
}
