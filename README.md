# LibOPZ: The non official API library for the OP-Z
(Tested on firmware version 1.2.39)

First you need to clone and compile the examples. It requires the following dev libraries installed on your system:

- `libz`
- `ncurses` (only for the examples) 

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

## Acknowledgment

The effords to reverse engineer the MIDI SYSEX messages from/to the OP-Z are based on:

- [Cengis's](https://github.com/lrk) [z-po project](https://github.com/lrk/z-po-project)
- [Mark Green's](https://github.com/hyphz) [opz docs wiki](https://github.com/hyphz/opzdoc/wiki/)
- [TE's OP-Z guide](https://teenage.engineering/guides/op-z)
