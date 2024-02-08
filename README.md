 # **TCU quickstart**

if you're using windows, you're on your own lol xd

clone this repository (I recommend making this a subdirectory of a Solar Car directory)
```
git clone https://github.com/UOP-Solarcar/CAN.git
```

### *Setting up PlatformIO*

if you're using arduino IDE (not recommended, you can skip setting up PlatormIO)
- what is platformIO?
-- PlatformIO is a cross-platform, cross-architecture, multi-framework professional IDE tool for embedded system and software engineers who write embedded applications.


 # *installation*
-- you could follow the instructions provided, alternatively vscode users or people scared of the command line could follow along the documentation at (vscode is basically a glorified text editor so it shouldnt make too big of a difference)
https://docs.platformio.org/en/latest/integration/ide/vscode.html
-- PlatformIO also supports CLion and has it's own IDE, if for whatever reason you wish to go down that route, consult the official documentation
-- chose your preferred installation method from the ones below
#  Mac 
- I recmmend installing with Homebrew. If you dont have homebrew installed, in a terminal run,
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
- PlatformIO can thus be installed this 
```
brew install platformio
```

# Python Installation
- if you have python installed, you can sinply run 
-- this method is cross-platform
```
python3 -m pip install -U platformio
```

## Creating a new platformio project
Identify the board you're using 
```
pio boards uno
```

using the appropriate board, initialize a PlatformIO project. For example:
```
pio project init --board uno
cd test
```
This will create a project with the following structure: 
(dont forget to actually edit the README)
```
.
├── include
│   └── README
├── lib
│   └── README
├── platformio.ini
├── src
└── test
    └── README
```
``platformio.ini`` is automatically generated based on boards, build flags, dependencies, and other specifications

Source code belongs in the ``src`` directory
```
cd src
touch main.cpp
```
The file hierarchy should look like:
```
.
├── include
│   └── README
├── lib
│   └── README
├── platformio.ini
├── src
│   └── main.cpp
└── test
    └── README

```

## Building and  running a platformio project
``pio run``  Process (build) all environments specified in “platformio.ini” (Project Configuration File)
``pio run --target upload`` Build project and upload firmware to the all devices specified in “platformio.ini” (Project Configuration File)
``pio run --target clear``  Clean project (delete compiled objects) 
``pio run -e uno``. Process only uno environment
``pio run -e uno -t upload``. Build project only for uno and upload firmware.

As with all things dont forget to actually read the documentation at https://platformio.org/



