#include <iostream>
#include <fstream>

#include "OPZ_RtMidi.h"

std::string version = "0.1";
std::string name = "opz_dump";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

int main(int argc, char** argv) {
    T3::OPZ_RtMidi  opz;
    opz.connect();
    // opz.verbose = 1; // message type
    // opz.verbose = 2; // hex values
    opz.verbose = 3; // interpreted message

    // Listen to key events (no cc, neighter notes)
    opz.setEventCallback( [](T3::event_id _id, int _value) {
        std::cout << T3::OPZ::toString(_id) << " " << _value << std::endl;
    } );

    opz.setMidiCallback( [](T3::midi_id _id, size_t channel, size_t _key, size_t _value) {
        std::cout << T3::OPZ::toString(_id) << ", chanel " << channel << ", key " << _key << ", value " << _value << std::endl;
    } );

    while (true)
        opz.keepawake();
    
    opz.disconnect();

    exit(0);
}
