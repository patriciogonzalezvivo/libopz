# LibOPZ non official OP-Z API library

This is on very early stages. All tested on firmware version 1.2.39

```
mkdir build
cd build
cmake ..
make
```

Then you can run any of the example programs.


## opz_dump

Prints out everything it recives and process from the OPZ

```
examples/./opz_dump
```

## opz_memory

A way of exploring OP-Z project memory

```
examples/./opz_memory
```

## opz_companion

Trying to replicate the OP-Z companion app in the terminal

```
examples/./opz_companion
```



## Acknowledgment

This work is based on:

- [z-op project](https://github.com/lrk/z-po-project)
- [opz docs wiki](https://github.com/hyphz/opzdoc/wiki/)
- [opz.js](https://github.com/nbw/opz)
- [OPZ WebApp](https://github.com/Karmagrinder/OPZ_MIDI)
- [MidiJack](https://github.com/teenageengineering/MidiJack)
- [TE's OP-Z guide](https://teenage.engineering/guides/op-z)