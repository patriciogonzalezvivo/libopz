# LibOPZ: a non official API library for the OP-Z
[![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/patriciogonzalezvivo)


First you need to clone and compile the examples. It requires the following dev libraries installed on your system:

- `libz`  
- `ncurses` (only for the c++ examples) 

```
mkdir build
cd build
cmake ..
make
```

After running these commands you can run any of the programs inside the `build/example` folder. 
Highly recomend trying the terminal base companion app. 
**Note** you need to have the OP-Z connected and turned on before running the app 

```
examples/./opz_companion
```

![gif](.github/000.gif)

# File structure and use

The files have the following hiearachy so they can be used for `.opz` files alone, connected to a devices through SYSEX with the MIDI library of your choosing or through RtMidi library.

- `opz` namespace
    - `opz_project`: general project class responsable of loading, holding and saving OP-Z project related data
        - `opz_device`: extended `opz_project` class that can sync with a device, retrive all the project data and trigger callbacks events when the devices it's uses (this also will include send change on the memory state of the device, see roadmap)
            - `opz_rtmidi`: extended `opz_device` class that uses RtMidi library to send/recived MIDI events.

# Roadmap

- [x] **Load/Save** `.opz` files
- [x] **Syncing with device** through MIDI SYSEX, recived and parse telemetry together with project data
- [ ] **Read and parse project data** into something meaningful and usable (from `.opz` file or device)
    - [x] Project parametters: drum_level, synth_level, punch_level, master_level, tempo, swing, metronome_level, metronome_sound.
    - [x] Pattern data
        - [x] muted tracks
        - [x] tracks parametters: plug, step_count, unknown1, step_length, quantize, note_style, note_length.
        - [x] tracks notes: duration, note, velocity, micro_adjustment, age.
        - [x] track sound parameters: param1, param2, attack, decay, sustain, release, fx1, fx2, filter, resonance, pan, level, portamento, lfo_depth, lfo_speed, lfo_value, lfo_shape, note_style.
        - [ ] track steps: components_bitmask, components_parameters, components_locked_values and parameter_mask are parsed but there is no funccionality associated with it (methods and maps to extract step components is missing).
        - [ ] send to tape
        - [ ] send to master
        - [ ] active group
    - [ ] Chained patterns

- [ ] **Parse all telemetry data** (streamed data from device while interacting with it). (Note: For it I'm going through the process of cloning all the functionality of the official companion app into a terminal only version).
    - [x] Main screen
    - [x] Pages
    - [x] Tempo
    - [ ] Projects (and chains)
    - [ ] Mixer
    - [ ] FX
    - [ ] Tape
    - [ ] Module
    - [ ] Master
    - [ ] Perform
    - [ ] Lights
    - [ ] Motion
    - [ ] Step Components
- [ ] **Exporting MIDI** from individual tracks patterns.
- [ ] Add the hability to **send** SYSEX messages that make **changes** on the OP-Z memory state.
    - [ ] Project parametters: drum_level, synth_level, punch_level, master_level, tempo, swing, metronome_level, metronome_sound.
    - [ ] Pattern data
        - [ ] muted tracks
        - [ ] tracks parametters: plug, step_count, unknown1, step_length, quantize, note_style, note_length.
        - [ ] tracks notes: duration, note, velocity, micro_adjustment, age.
        - [ ] track sound parameters: param1, param2, attack, decay, sustain, release, fx1, fx2, filter, resonance, pan, level, portamento, lfo_depth, lfo_speed, lfo_value, lfo_shape, note_style.
        - [ ] track steps: components_bitmask, components_parameters, components_locked_values and parameter_mask are parsed but there is no funccionality associated with it (methods and maps to extract step components is missing).
        - [ ] send to tape
        - [ ] send to master
        - [ ] active group
    - [ ] Chained patterns

Independent goals:
    - [ ] Optimize
    - [ ] Better error handling, robustness and reliability
    - [ ] Compile a **Python3** binding module using SWIG (probably with no RtMidi support, so raw midi messages will needs to be handled by a Python library)
    - [ ] Compile a **JavaScript** module using WASM (probably width no RtMidi support, raw midi messages will need to be handled by a JS library)

## Acknowledgment

The effords to reverse engineer the MIDI SYSEX messages from/to the OP-Z are based on:

- [Cengis's](https://github.com/lrk) [z-po project](https://github.com/lrk/z-po-project)
- [Mark Green's](https://github.com/hyphz) [opz docs wiki](https://github.com/hyphz/opzdoc/wiki/)
- [TE's OP-Z guide](https://teenage.engineering/guides/op-z)


## Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


## License 

LibOPZ is dual-licensed under [the Prosperity License](https://prosperitylicense.com/versions/3.0.0) and the [Patron License](https://patronlicense.com/versions/1.0.0.html) for [sponsors](https://github.com/sponsors/patriciogonzalezvivo) and [contributors](https://github.com/patriciogonzalezvivo/lygia/graphs/contributors).

[Sponsors](https://github.com/sponsors/patriciogonzalezvivo) and [contributors](https://github.com/patriciogonzalezvivo/libopz/graphs/contributors) become "Participating Contributors" to the [Patron License](https://patronlicense.com/versions/1.0.0.html) and can ignore any non-commercial limitation of [the Prosperity Licensed](https://prosperitylicense.com/versions/3.0.0) software.

