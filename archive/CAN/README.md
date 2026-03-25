=======

# **TCU quickstart**

clone this repository (I recommend making this a subdirectory of a Solar Car directory)

```
git clone https://github.com/UOP-Solarcar/CAN.git
```

### _Setting up PlatformIO_

if you're using arduino IDE (not recommended, you can skip setting up PlatormIO)

- what is platformIO?
  -- PlatformIO is a cross-platform, cross-architecture, multi-framework professional IDE tool for embedded system and software engineers who write embedded applications.

# _installation_

-- you could follow the instructions provided, alternatively vscode users or people scared of the command line could follow along the documentation at (vscode is basically a glorified text editor so it shouldnt make too big of a difference)
<https://docs.platformio.org/en/latest/integration/ide/vscode.html>
-- PlatformIO also supports CLion and has it's own IDE, if for whatever reason you wish to go down that route, consult the official documentation
-- chose your preferred installation method from the ones below

# Mac

- I recommend installing with Homebrew. If you dont have homebrew installed, in a terminal run,

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

> [!IMPORTANT]
> If you're on Apple Silicon, (Apple M1, M2, etc.) you must install Rosetta to ensure that your builds will compile.

- You can install Rosetta by running:

```bash
softwareupdate --install-rosetta
```

This launches the installer, where you'll have to agree to the license before it installs. To skip that process and automatically agree to Rosetta terms, you can simply add an additional flag:

```bash
/usr/sbin/softwareupdate --install-rosetta --agree-to-license
```

- PlatformIO can thus be installed with

```bash
brew install platformio
```

# Python Installation

- if you have python installed, you can simply run
- this method is cross-platform

```bash
python3 -m pip install -U platformio
```

> [!IMPORTANT]
> If you have a Windows system, you will need to add `pio` to your `Path` after installation.

- Find the Python Scripts directory (e.g., `C:\Users\<Username>\.platformio\penv\Scripts\`).
- Add this directory to your PATH:
- Search for "Environment Variables" in the Windows Start Menu.
- Under "System Properties," click on Environment Variables.
- In "System Variables," find `Path`, click Edit, and add the Python `Scripts` path.

## Creating a new platformio project

Identify the board you're using, for example if you know you're using an arduino uno, you can run pio boards with the uno argument

```bash
pio boards uno
```

using the appropriate board, initialize a PlatformIO project. For example:

```bash
mkdir test
cd test
pio project init --board uno
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

`platformio.ini` is automatically generated based on boards, build flags, dependencies, and other specifications

Source code belongs in the `src` directory

```bash
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

## Building and running a platformio project

`pio run` Process (build) all environments specified in “platformio.ini” (Project Configuration File)
`pio run --target upload` Build project and upload firmware to the all devices specified in “platformio.ini” (Project Configuration File)
`pio run --target clear` Clean project (delete compiled objects)
`pio run -e uno`. Process only uno environment
`pio run -e uno -t upload`. Build project only for uno and upload firmware.

As with all things dont forget to actually read the documentation at <https://platformio.org/>

> > > > > > > old-can/master
