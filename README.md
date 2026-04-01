# OpenD2
A project to open-source Diablo 2, under the GNU General Public License.

![Diablo II Main Menu in OpenD2](https://i.imgur.com/RFNbRiT.png)

### Project Goals
Simply put, this project is a total rewrite of the game engine. It uses the original game files, and uses the original game's save files. Ideally it will also be compatible in TCP/IP games with the original client, but this may not be feasible.

Why would you want this? Well:
 * It will fix some bugs. However it will try to remain as close as possible to the original game experience.
 * It is a great base for building mods. In the past, mods relied on reverse engineering the game through hooking and memory patches to create advanced features.
 * It will run better than Blizzard's game, and won't require fiddling with Windows compatibility settings or running as Administrator to work.
 * It will run on Linux, Mac and Windows, without the need for emulators (ie, Wine)

It will not support Open or Closed Battle.net in order to minimize legal issues. Also, it will not support the cinematics because those use the proprietary BINK format.

### Project Status / Contributing
The majority of the gamecode is still being written. Currently, you can connect to a TCP/IP game and it will show up on the other end that you've connected, however it will stall on loading. Most of the main menu works outside of that.

If you would like to contribute to this project, please fork it and submit pull requests. 


### Compiling

#### Windows
To compile this project on Windows, all you will need is CMake and Visual Studio 2017 or later.

**Command Line Build:**
```cmd
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

**Using CMake GUI:**
Run cmake-gui and set the Source directory to this folder. Set the "Where to build the files" to be ./build. (This is so that the git repository doesn't pick this up as a source directory). Then, simply open the project file in whatever IDE you want.

**Dependencies:** The project includes all necessary libraries (SDL2, GLEW, GLM) and automatically copies required DLLs to the output directory during build.

#### Linux
Compilation on Linux requires the following prerequisites:
  * libglew-dev (or libglew-devel on Red Hat based systems)
  * libglm-dev (or libglm-devel on Red Hat based systems)
  * libsdl2-dev (or libsdl2-devel on Red Hat based systems)

After that, the following commands can be used to install:

```sh
mkdir build && cd build
cmake .. -DSDL2MAIN_LIBRARY=/usr/lib64/libSDL2.so
make
```

### Running
*Generally speaking* you will want to run the game from a separate directory from the main game, in order to not screw up your original installation.
The original game options are preserved:

* `-w` - Run in Windowed mode

And OpenD2 adds a few of its own, which start with `+` instead of `-`:

* `+basepath="..."` - Set the basepath (ie, where your game is installed to). Replace the ... with the path.
* `+homepath="..."` - Set the homepath (ie, where your game saves data to). Replace the ... with the path. Defaults to `<user>/My Documents/My Games/Diablo II`.
* `+modpath="..."` - Set the modpath (ie, where mods overwriting content will read from)
* `+sdlnoaccel` - Disables hardware acceleration
* `+borderless` - Run in borderless windowed mode
* `+logflags=...` - Set the priority for logging information. These are flags. 1 = Log Crashes, 2 = Log messages, 4 = Log debug info, 8 = Log system info, 16 = "prettify" the log

**Default Behavior:** The game now defaults to using the executable directory as the basepath, so you can run it directly without parameters in most cases.

**Example commands:**
```cmd
# Basic windowed mode (uses executable directory as basepath)
game.exe -w

# With specific Diablo II installation path
game.exe -w '+basepath="C:/Program Files (x86)/Diablo II"'

# With custom installation path
game.exe -w '+basepath="D:/Games/Diablo II"'

# Use the provided launcher scripts (for convenience)
launch_opend2.bat
# or
launch_opend2_advanced.bat "C:/Games/Diablo II"
```

**Requirements for running:**
1. Graphics drivers supporting OpenGL 3.3 or later
2. Diablo II (version 1.10) for game assets (optional - can run without)
3. To play multiplayer: host a TCP/IP game in vanilla Diablo 2 first, then join it through OpenD2

**Note:** OpenD2 now runs standalone and doesn't require an existing Diablo II installation to start. However, you'll need the original game files for full functionality.

### Architecture
Just as in the original game, there are several interlocking components driving the game. The difference is that all but the core can be swapped out by a mod.

#### Core (game.exe)
The core game engine communicates with all of the other components and drives everything. It is (or will be) responsible for the following:
- Window management
- Filesystem
- Memory management
- Log management
- Archive (.mpq) management
- Networking
- Sound
- Rendering

#### Common Code (D2Common.dll)
D2Common contains common routines needed by both the serverside and clientside. This includes things such as dungeon-building from random seeds, skill logic, .TXT -> .BIN compilation, and more.

#### Serverside (D2Game.dll)
The serverside is responsible for quest management, AI, and more. Ideally, this should be allowed to mismatch the client DLL and have custom game server logic.

#### Clientside (D2Client.dll)
The clientside is responsible for client logic, mostly with drawing the menus and sprites.
