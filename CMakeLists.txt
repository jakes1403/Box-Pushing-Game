# Build Box Pushing Game
cmake_minimum_required (VERSION 3.6)

project (BoxPushingGame VERSION 1.0.0)

option(RENDERER_SDL2 "Use the SDL2 Renderer - must disable RENDERER_OPENGL3" ON)
option(RENDERER_OPENGL3 "Use the OpenGL3 Renderer - must disable RENDERER_SDL2" OFF)
option(RELEASE_MODE "Should compile in release mode?" ON)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(BUILD_STATIC ON)
set(BUILD_DEMOS OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/chipmunk")
if (NOT PLATFORM_PSP)
    add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/lua")

    include_directories("${CMAKE_SOURCE_DIR}/extlibs/lua/include")
    include_directories("${CMAKE_SOURCE_DIR}/extlibs/lua/src")
endif()
include_directories("${CMAKE_SOURCE_DIR}/extlibs/chipmunk/include")

set(BUILD_STATIC_LIBS TRUE)
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/glm")

if (RENDERER_OPENGL3)
    find_package(OpenGL REQUIRED)
endif()
if (NOT EMSCRIPTEN)
    if (NOT PLATFORM_PSP)
        find_package(SDL2 REQUIRED)
        find_package(SDL2_mixer REQUIRED)
        include_directories(${SDL2_INCLUDE_DIRS})
        include_directories(${SDL_mixer_INCLUDE_DIRS})
        set(SDL2_link
            SDL2::Main 
            SDL2::Mixer
        )
        set(LUA_link lua_static)
    endif()
endif()

file(GLOB GAME_SOURCES "src/*")

# resolve platform dependencies and names
if (PLATFORM_PSP)
    message(STATUS "Game set up for Sony PSP...")
    set(TARGET_NAME ${PROJECT_NAME})
    set(SDL2_link
        SDL2
        SDL2_image
    )
    add_executable(${TARGET_NAME} ${GAME_SOURCES} "src/psp/callback.cpp" "src/psp/callback.h" "src/psp/glib2d.c" "src/psp/glib2d.h")
    set(PLATFORM_SPECIFIC_INCLUDES
        lua
        m
        ${PSP_LIBRARIES}
        stdc++

        vorbis
        ogg
        png
        jpeg
        z

        pspvram

        GL
        pspgum
        pspgu
        pspge
        pspvfpu

        c

        pspuser
        pspkernel
        pspaudio
        pspdisplay
        pspdebug
        pspctrl
        pspnet
        pspnet_inet
        pspnet_resolver
        psphprm
        psprtc
    )
endif()
if (EMSCRIPTEN)
    set(LUA_link lua_static)
    set(TARGET_NAME ${PROJECT_NAME})
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_executable(${TARGET_NAME} ${GAME_SOURCES})
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s USE_SDL_MIXER=2 -s USE_SDL=2 -s DEMANGLE_SUPPORT=1 --preload-file ${CMAKE_SOURCE_DIR}/assets_web@/")
    include_directories("D:/NewSoftwareProjects/Libraries/SDL2/include")
    if (RENDERER_OPENGL3)
        set(PLATFORM_SPECIFIC_INCLUDES
            GLESv2
        )
    endif()
elseif(ANDROID)
    add_subdirectory(SDL)
    set(TARGET_NAME main)
    set(PLATFORM_SPECIFIC_INCLUDES
        GLESv2
        log
        android
    )
    add_library(${TARGET_NAME} SHARED)
	target_sources(${TARGET_NAME} PRIVATE ${GAME_SOURCES})
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    set(TARGET_NAME ${PROJECT_NAME})
    set(PLATFORM_SPECIFIC_INCLUDES
        dl
        ${GTK3_LIBRARIES}
    )
    add_executable(${TARGET_NAME} ${GAME_SOURCES})
elseif(WIN32)
    set(TARGET_NAME ${PROJECT_NAME})
    if (RENDERER_OPENGL3)
        set(PLATFORM_SPECIFIC_INCLUDES
            opengl32
        )
    endif()
    add_executable(${TARGET_NAME} WIN32 ${GAME_SOURCES} resource/resource.rc)
endif()

if (RENDERER_SDL2)
    target_compile_definitions(${TARGET_NAME} PUBLIC BUILD_GAME_SDL2)
endif()
if (RENDERER_OPENGL3)
    target_compile_definitions(${TARGET_NAME} PUBLIC BUILD_GAME_OPENGL3)
endif()

if (RELEASE_MODE)
    target_compile_definitions(${TARGET_NAME} PUBLIC BUILD_GAME_RELEASE)
else()
    target_compile_definitions(${TARGET_NAME} PUBLIC BUILD_GAME_DEBUG)
endif()

target_link_libraries(${TARGET_NAME}
	glm::glm_static
    ${LUA_link}
    chipmunk
	chipmunk_static
    ${SDL2_link}
	${PLATFORM_SPECIFIC_INCLUDES}
)
if (PLATFORM_PSP)
    create_pbp_file(TARGET ${TARGET_NAME} TITLE "Box Pushing Game" ICON_PATH "icon0.png" BACKGROUND_PATH "PIC1.png")
endif()

# Tell CPack to generate a .deb package
set(CPACK_GENERATOR "DEB")

# Set a Package Maintainer.
# This is required
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jacob Allen")

# Set a Package Version
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

set(CPACK_DEBIAN_PACKAGE_DEPENDS chipmunk SDL2 SDL2_mixer)

# Include CPack
include(CPack)