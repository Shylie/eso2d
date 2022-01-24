# Depenencies Setup

This project depends on [BearLibTerminal](http://foo.wyrd.name/en:bearlibterminal) and [dylib](https://github.com/martin-olivier/dylib).
Dylib is included as a git submodule, but BearLibTerminal should be downloaded manually.

Set up the dependencies folder as follows:
```
eso2d
└───dependencies
    ├───include
    │       BearLibTerminal.h
    │       
    ├───x64
    │   ├───bin
    │   │       BearLibTerminal.dll
    │   │       
    │   └───lib
    │           BearLibTerminal.lib
    │           
    └───x86
        ├───bin
        │       BearLibTerminal.dll
        │       
        └───lib
                BearLibTerminal.lib
```