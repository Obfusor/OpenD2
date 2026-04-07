cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

project(OpenD2)

option(BUILD_GAME "Build Executable" ON)
option(BUILD_D2CLIENT "Build D2Client" ON)
option(BUILD_D2SERVER "Build D2Server" ON)
option(ENABLE_EDITOR "Enable integrated map editor" ON)

# Common options
if(WIN32)
	set(STATIC_LIBRARIES dbghelp.lib psapi.lib)
else()
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_COMPILER g++)
endif()

# Add shared files
file(GLOB_RECURSE SHARED_SRC Shared/*.h Shared/*.hpp Shared/*.c Shared/*.cpp)
source_group("Shared" FILES ${SHARED_SRC})

# Build game.exe
if(BUILD_GAME)
	message(STATUS "Including game.exe files")

	# Collect engine sources, excluding EditorCompat (built separately)
	file(GLOB ENGINE_SRC
		Engine/*.h Engine/*.hpp Engine/*.c Engine/*.cpp
	)

	# Remove SDL/GL-specific engine files (no longer used)
	list(REMOVE_ITEM ENGINE_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/Renderer_GL.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/Renderer_SDL.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/Window.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/Input.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/Audio_SDL.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/Threadpool.cpp
	)

	# Build EditorLib - the editor's C rendering code as a static library
	file(GLOB EDITORCOMPAT_SRC
		Engine/EditorCompat/*.c
		Engine/EditorCompat/*.h
	)
	file(GLOB EDITORCOMPAT_MPQ_SRC
		Engine/EditorCompat/mpq/*.c
		Engine/EditorCompat/mpq/*.h
	)

	# Exclude main.c (has its own entry point) from EditorLib
	list(REMOVE_ITEM EDITORCOMPAT_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/EditorCompat/main.c
	)

	add_library(EditorLib STATIC ${EDITORCOMPAT_SRC} ${EDITORCOMPAT_MPQ_SRC})
	set_target_properties(EditorLib PROPERTIES LINKER_LANGUAGE C)
	target_compile_definitions(EditorLib PRIVATE
		_CRT_SECURE_NO_WARNINGS
	)

	if(WIN32)
		list(REMOVE_ITEM ENGINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Platform_Linux.cpp)

		set(ALLEGRO5_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/include")
		if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
			set(ALLEGRO5_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/lib/win32")
			set(ALLEGRO5_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/bin/win32")
			set(ALLEGRO5_DEPS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/deps/lib/win32")
		else()
			# For 64-bit, use same path for now (only 32-bit libs available)
			set(ALLEGRO5_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/lib/win32")
			set(ALLEGRO5_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/bin/win32")
			set(ALLEGRO5_DEPS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/allegro5/deps/lib/win32")
		endif()

		set(ALLEGRO5_LIBRARIES
			"${ALLEGRO5_LIB_DIR}/allegro.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_image.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_font.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_ttf.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_primitives.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_audio.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_acodec.lib"
		)

		# Compression libraries (already in engine)
		file(GLOB_RECURSE ADPCM_SRC Libraries/adpcm/*)
		file(GLOB_RECURSE HUFFMAN_SRC Libraries/huffman/*)
		file(GLOB_RECURSE PKWARE_SRC Libraries/pkware/*)
		set(LIBRARY_SRC ${ADPCM_SRC} ${HUFFMAN_SRC} ${PKWARE_SRC})
	else()
		list(REMOVE_ITEM ENGINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Platform_Windows.cpp)

		find_package(Allegro5 REQUIRED)
		set(LIBRARY_SRC "")
	endif()

	# EditorLib uses Allegro 5 (same as the rest of the engine)
	target_include_directories(EditorLib PRIVATE
		${ALLEGRO5_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/EditorCompat
	)
	target_link_libraries(EditorLib ${ALLEGRO5_LIBRARIES})

	# Dear ImGui
	set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/imgui")
	set(IMGUI_SRC
		${IMGUI_DIR}/imgui.cpp
		${IMGUI_DIR}/imgui_demo.cpp
		${IMGUI_DIR}/imgui_draw.cpp
		${IMGUI_DIR}/imgui_tables.cpp
		${IMGUI_DIR}/imgui_widgets.cpp
		${IMGUI_DIR}/backends/imgui_impl_allegro5.cpp
	)
	source_group("ImGui" FILES ${IMGUI_SRC})

	source_group("Engine" FILES ${ENGINE_SRC})
	source_group("EditorCompat" FILES ${EDITORCOMPAT_SRC})
	source_group("EditorCompat\\mpq" FILES ${EDITORCOMPAT_MPQ_SRC})
	source_group("Shared" FILES ${SHARED_SRC})

	set(ENGINE_SRC ${ENGINE_SRC} ${SHARED_SRC} ${LIBRARY_SRC} ${IMGUI_SRC})

	add_executable(game ${ENGINE_SRC})
	set_target_properties(game PROPERTIES OUTPUT_NAME game)
	set_target_properties(game PROPERTIES LINKER_LANGUAGE CXX)

	target_compile_definitions(game PUBLIC D2EXE ALLEGRO_NO_MAGIC_MAIN)
	if(ENABLE_EDITOR)
		target_compile_definitions(game PUBLIC ENABLE_EDITOR)
		target_compile_definitions(EditorLib PRIVATE ENABLE_EDITOR)
	endif()

	target_link_libraries(game PRIVATE EditorLib ${ALLEGRO5_LIBRARIES})
	target_include_directories(game PRIVATE
		${ALLEGRO5_INCLUDE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/Engine/EditorCompat
		${IMGUI_DIR}
		${IMGUI_DIR}/backends
	)

	if(WIN32)
		set_target_properties(game PROPERTIES LINK_FLAGS /SUBSYSTEM:WINDOWS)

		# System libs needed by Allegro 5
		target_link_libraries(game PRIVATE
			user32 gdi32 winmm ole32 kernel32 advapi32
			shell32 comdlg32 uuid opengl32 Shlwapi psapi dsound
		)

		# Copy Allegro 5 DLLs to output directory
		add_custom_command(TARGET game POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${ALLEGRO5_BIN_DIR}"
			"$<TARGET_FILE_DIR:game>"
		)
	else()
		target_link_libraries(game PRIVATE m dl pthread)
	endif()
endif()

# Build D2Common.dll
if(BUILD_D2CLIENT OR BUILD_D2SERVER)
	message(STATUS "Including D2Common.dll files")

	file(GLOB_RECURSE D2COMMON_SRC Modcode/Common/*.h Modcode/Common/*hpp Modcode/Common/*.c Modcode/Common/*.cpp)

	source_group("Common" FILES ${D2COMMON_SRC})
	source_group("Shared" FILES ${SHARED_SRC})

	set(D2COMMON_SRC ${D2COMMON_SRC} ${SHARED_SRC})

	add_library(D2Common SHARED ${D2COMMON_SRC})
	set_target_properties(D2Common PROPERTIES OUTPUT_NAME D2Common)
	set_target_properties(D2Common PROPERTIES LINKER_LANGUAGE CXX)

	target_link_libraries(D2Common ${STATIC_LIBRARIES})
	target_compile_definitions(D2Common PRIVATE D2COMMON)

	link_directories(${PROJECT_BINARY_DIR})
endif()

# Build D2Client.dll
if(BUILD_D2CLIENT)
	message(STATUS "Including D2Client.dll files")

	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Modcode/Client")
endif()


# Build D2Server.dll
if(BUILD_D2SERVER)
	message(STATUS "Including D2Server.dll files")

	file(GLOB_RECURSE D2SERVER_SRC Modcode/Server/*.h Modcode/Server/*.hpp Modcode/Server/*.c Modcode/Server/*.cpp)

	source_group("Server" FILES ${D2SERVER_SRC})
	source_group("Shared" FILES ${SHARED_SRC})

	# D2Unit is shared code needed by both client and server
	set(D2SERVER_SRC ${D2SERVER_SRC} ${SHARED_SRC}
		Modcode/Client/Game/D2Unit.cpp
		Modcode/Client/Game/D2Unit.hpp)

	add_library(D2Server SHARED ${D2SERVER_SRC})
	set_target_properties(D2Server PROPERTIES OUTPUT_NAME D2Server)
	set_target_properties(D2Server PROPERTIES LINKER_LANGUAGE CXX)

	target_link_libraries(D2Server ${STATIC_LIBRARIES} D2Common)
	target_compile_definitions(D2Server PRIVATE D2SERVER)
endif()

# Build standalone tools (no engine dependencies)
option(BUILD_TOOLS "Build standalone tools" ON)
if(BUILD_TOOLS)
	message(STATUS "Including d2s_parser.exe")

	file(GLOB D2SPARSER_SRC Tools/D2SaveParser/*.cpp Tools/D2SaveParser/*.hpp)
	source_group("D2SaveParser" FILES ${D2SPARSER_SRC})

	add_executable(d2s_parser ${D2SPARSER_SRC})
	set_target_properties(d2s_parser PROPERTIES OUTPUT_NAME d2s_parser)
	set_target_properties(d2s_parser PROPERTIES LINKER_LANGUAGE CXX)

	if(WIN32)
		set_target_properties(d2s_parser PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
	endif()

	# DC6 to PNG converter
	message(STATUS "Including dc6_to_png.exe")

	add_executable(dc6_to_png Tools/DC6Converter/dc6_to_png.cpp)
	set_target_properties(dc6_to_png PROPERTIES OUTPUT_NAME dc6_to_png)
	set_target_properties(dc6_to_png PROPERTIES LINKER_LANGUAGE CXX)
	target_compile_definitions(dc6_to_png PRIVATE ALLEGRO_NO_MAGIC_MAIN _CRT_SECURE_NO_WARNINGS)

	if(WIN32)
		set_target_properties(dc6_to_png PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
		target_include_directories(dc6_to_png PRIVATE ${ALLEGRO5_INCLUDE_DIR})
		target_link_libraries(dc6_to_png
			"${ALLEGRO5_LIB_DIR}/allegro.lib"
			"${ALLEGRO5_LIB_DIR}/allegro_image.lib"
		)
		# Copy Allegro DLLs so the tool can run
		add_custom_command(TARGET dc6_to_png POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${ALLEGRO5_BIN_DIR}"
			"$<TARGET_FILE_DIR:dc6_to_png>"
		)
	else()
		find_package(Allegro5 REQUIRED)
		target_include_directories(dc6_to_png PRIVATE ${ALLEGRO5_INCLUDE_DIR})
		target_link_libraries(dc6_to_png ${ALLEGRO5_LIBRARIES})
	endif()
endif()
