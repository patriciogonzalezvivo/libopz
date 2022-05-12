#include <iostream>
#include <fstream>

#include "libopz/opz_rtmidi.h"

std::string version = "0.1";
std::string name = "opz_dump";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

int main(int argc, char** argv) {
    T3::opz_rtmidi  opz;
    opz.connect();
    // opz.verbose = 1; // message type
    // opz.verbose = 2; // hex values
    opz.verbose = 3; // interpreted message
    // opz.verbose = 4; // interpreted message w no hex values

    // // Listen to key events (no cc, neighter notes)
    // opz.setEventCallback( [&](T3::opz_event_id _id, int _value) {
    //     if (_id == T3::KEY_SCREEN && _value == 0) {
    //         std::vector<unsigned char> cmd = {
    //             // 0x15, 0x00, 0x00, 0x00, 0x00, 0x00
    //             0x0F
    //         };
    //         opz.sendCmd(&cmd[0], cmd.size());
    //     } 
    // } );

    // opz.setMidiCallback( [](T3::midi_id _id, size_t channel, size_t _key, size_t _value) {
    //     std::cout << T3::OPZ::toString(_id) << ", chanel " << channel << ", key " << _key << ", value " << _value << std::endl;
    // } );

    while (true)
        opz.update();
    
    opz.disconnect();

    exit(0);
}
