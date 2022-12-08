# Time Machine

A PSP utility which enables the user to load compatible firmware(s) from the Memory Stick, allowing her/him to experience homebrews that can no longer be played on the newest one.

Time Machine is also used in the more recent versions of [Despertar del Cementerio](https://github.com/mathieulh/Despertar-Del-Cementerio), which allows users to repair consoles with firmware problems.

The code is based on knowledge gained from reverse-engineering of the original Time Machine created by Dark AleX.

# State of the project

Unfortunately the project is nowhere near completion. Check the Issues page for a general idea what needs to be done.

# Requirements

The project requires Python 3 with packages: **pycryptodome** and **heatshrink2**. These utilities can be installed using pip:
```bash
pip3 install pycryptodome heatshrink2
```
The latest version of the [psp toolchain](https://github.com/pspdev/psptoolchain) is also required, along with a recent CMake (3.x+).

# Build

To build the project, issue the following CMake commands:
```bash
psp-cmake -S . -B build && cmake --build build
```

There is no install script (yet), so the build artifacts are all over the place. You might be interested in:
* `tm_msipl`, which is a multi-IPL loader that can be configured with `ms0:/TM/config.txt`
  * `tm_msipl.bin`, which is the universal IPL compatibile with all models (but no bootrom access)
  * `tm_msipl_legacy.bin`, which uses the old way of encrypting IPL blocks (compatibile with 01g and some 02g models)
* `tm_sloader`, which can load `payload.bin` and `nandipl.bin` from the current directory
* `tm_mloader`, which can load `payload_XXg.bin` and `nandipl_XXg.bin`, depending on your PSP model, from the current directory

# Memory Stick IPL Installation

To install the `tm_msipl.bin` to your Memory Stick you can use this Python 3 [script](https://github.com/draanPSP/msipl_installer).

# Credits

Dark AleX and the rest of the M33 team for the original TimeMachine,  
Davee for the [Infinity](https://github.com/DaveeFTW/Infinity), [iplsdk](https://github.com/DaveeFTW/iplsdk), dumping the PSP 3000 bootrom and general help,  
Proxima for the [MIPS emulator](https://github.com/ProximaV/allegrexemu) and general help,  
Mathieulh for the original [PSP_IPL_SDK](https://github.com/mathieulh/PSP_IPL_SDK) and [Devhook](https://github.com/mathieulh/Devhook),  
uOFW team for the firmware reverse-engineering efforts,  
John-K and artart78 for the IPL decryption code [pspdecrypt](https://github.com/John-K/pspdecrypt),  
Balika011 for 6.61 IPL patches [DC-M33](https://github.com/balika011/DC-M33/),  
neur0n and Rahim-US for [6.61 ME](https://github.com/PSP-Archive/minimum_edition).

# External libraries used
[Heatshrink](https://github.com/atomicobject/heatshrink) - ISC License  
[FatFS](http://elm-chan.org/fsw/ff/00index_e.html) - BSD-style license
