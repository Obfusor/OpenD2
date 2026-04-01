# OpenD2 Build Dependencies

## Overview
This document explains the library dependencies required for OpenD2 and how they are handled during the build process.

## Required Libraries

### SDL2 (Simple DirectMedia Layer)
OpenD2 uses SDL2 for:
- Window management
- Input handling  
- Audio playback
- Networking (SDL2_net)
- Audio mixing (SDL2_mixer)

**Required DLLs:**
- `SDL2.dll` - Core SDL2 library
- `SDL2_mixer.dll` - Audio mixing functionality
- `SDL2_net.dll` - Network functionality

**Audio Codec DLLs (for SDL2_mixer):**
- `libFLAC-8.dll` - FLAC audio support
- `libmodplug-1.dll` - Module audio support
- `libmpg123-0.dll` - MP3 audio support
- `libogg-0.dll` - OGG container support
- `libopus-0.dll` - Opus audio support
- `libopusfile-0.dll` - Opus file reading
- `libvorbis-0.dll` - Vorbis audio support
- `libvorbisfile-3.dll` - Vorbis file reading

### GLEW (OpenGL Extension Wrangler)
Used for OpenGL extension management and modern OpenGL functionality.

**Required DLLs:**
- `glew32.dll` - GLEW library

### GLM (OpenGL Mathematics)
Header-only math library for OpenGL (no DLLs required).

## Build Process

### Automatic DLL Handling
The CMakeLists.txt has been configured to automatically copy all required DLLs to the output directory during build:

```cmake
# Copy required DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/sdl/x86"
        "$<TARGET_FILE_DIR:game>"
    )
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/glew/glew32.dll"
        "$<TARGET_FILE_DIR:game>"
    )
endif()
```

### Manual DLL Copying (if needed)
If automatic copying fails, you can manually copy the DLLs:

```powershell
# Copy SDL2 DLLs
Copy-Item "Libraries/sdl/x86/*.dll" "build/Release/"

# Copy GLEW DLL  
Copy-Item "Libraries/glew/glew32.dll" "build/Release/"
```

## Troubleshooting

### Common Issues

1. **"The program can't start because SDL2.dll is missing"**
   - Solution: Ensure SDL2.dll is in the same directory as game.exe

2. **"The program can't start because glew32.dll is missing"**
   - Solution: Ensure glew32.dll is in the same directory as game.exe

3. **Audio not working**
   - Solution: Ensure all audio codec DLLs are present (libFLAC-8.dll, etc.)

4. **"Fatal Assertion Failure: Basepath is not set" (older versions)**
   - This has been fixed: The game now automatically uses the executable directory as the default basepath
   - If you still get this error, rebuild the project with the latest code

5. **"SDL_GL_CreateContext() failed: the specified window isn't an OpenGL window"**
   - This has been fixed: OpenGL attributes are now set before window creation
   - OpenGL is now enabled by default
   - If you still get this error, ensure your graphics drivers support OpenGL 3.3 or later

### Verification
To verify all dependencies are present, check that these files exist in your build/Release directory:
- game.exe
- D2Common.dll, D2Client.dll, D2Server.dll (built by the project)
- SDL2.dll, SDL2_mixer.dll, SDL2_net.dll
- glew32.dll
- All audio codec DLLs (lib*.dll files)

## Platform Notes

### Windows
- Uses 32-bit (x86) libraries by default
- Libraries are included in the repository under `Libraries/`
- Visual Studio 2017 or later required

### Linux  
- Uses system-installed packages (libsdl2-dev, libglew-dev, libglm-dev)
- No DLL copying needed (uses shared libraries)