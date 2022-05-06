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
Highly recomend trying the terminal base companion app 

```
examples/./opz_companion
```

![gif](.github/000.gif)

## Acknowledgment

This work is based on:

- [z-po project](https://github.com/lrk/z-po-project)
- [opz docs wiki](https://github.com/hyphz/opzdoc/wiki/)
- [opz.js](https://github.com/nbw/opz)
- [OPZ WebApp](https://github.com/Karmagrinder/OPZ_MIDI)
- [MidiJack](https://github.com/teenageengineering/MidiJack)
- [TE's OP-Z guide](https://teenage.engineering/guides/op-z)
