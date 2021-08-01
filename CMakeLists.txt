# Build Box Pushing Game
cmake_minimum_required (VERSION 3.6)

project (BoxPushingGame VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(BUILD_STATIC ON)
set(BUILD_DEMOS OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/chipmunk")
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/lua")

include_directories("${CMAKE_SOURCE_DIR}/extlibs/lua/include")
include_directories("${CMAKE_SOURCE_DIR}/extlibs/lua/src")
include_directories("${CMAKE_SOURCE_DIR}/extlibs/chipmunk/include")

set(BUILD_STATIC_LIBS TRUE)
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/glm")

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL_mixer_INCLUDE_DIRS})

file(GLOB GAME_SOURCES "src/*")

# resolve platform dependencies and names
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s USE_SDL=2 -s DEMANGLE_SUPPORT=1 --preload-file ${CMAKE_SOURCE_DIR}/assets_compiled@/")
    set(TARGET_NAME ${PROJECT_NAME})
    set(PLATFORM_SPECIFIC_INCLUDES
        GLESv2
    )
    add_executable(${TARGET_NAME} ${GAME_SOURCES})
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
    set(PLATFORM_SPECIFIC_INCLUDES
        opengl32
    )
    add_executable(${TARGET_NAME} WIN32 ${GAME_SOURCES} resource/resource.rc)
endif()


target_link_libraries(${TARGET_NAME}
	glm::glm_static
	lua_static
	chipmunk
    SDL2::Main
    SDL2::Mixer
	${PLATFORM_SPECIFIC_INCLUDES}
)

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