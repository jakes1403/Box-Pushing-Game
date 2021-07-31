# CMakeList.txt : CMake project for Editor, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.6)

project (BoxPushingGame)

#if (WINDOWS)
    set(SDL2_PATH "${CMAKE_SOURCE_DIR}/extlibs/SDL2")
    set(SDL2_mixer_PATH "${CMAKE_SOURCE_DIR}/extlibs/SDL2_mixer")
    set(IS_64_BIT ${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    if (IS_64_BIT)
        set(SDL2 "${SDL2_PATH}/lib/x64/SDL2.lib;${SDL2_PATH}/lib/x64/SDL2main.lib")
        set(SDL2_mixer "${SDL2_mixer_PATH}/lib/x64/SDL2_mixer.lib")
    else ()
        set(SDL2 "${SDL2_PATH}/lib/x86/SDL2.lib;${SDL2_PATH}/lib/x86/SDL2main.lib")
        set(SDL2_mixer "${SDL2_mixer_PATH}/lib/x86/SDL2_mixer.lib")
    endif ()
    set(SDL2_HEADERS "${SDL2_PATH}/include")
    set(SDL2_mixer_HEADERS "${SDL2_mixer_PATH}/include")
    include_directories(${SDL2_HEADERS})
    include_directories(${SDL2_mixer_HEADERS})
#endif()

set(BUILD_STATIC ON)
set(BUILD_DEMOS OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/chipmunk")
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/lua")

include_directories("${CMAKE_SOURCE_DIR}/extlibs/lua/include")
include_directories("${CMAKE_SOURCE_DIR}/extlibs/lua/src")
include_directories("${CMAKE_SOURCE_DIR}/extlibs/chipmunk/include")

set(BUILD_STATIC_LIBS TRUE)
add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/glm")

add_subdirectory("${CMAKE_SOURCE_DIR}/extlibs/flecs")

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
elseif(ANDROID)
    add_subdirectory(SDL)
endif()

if (EMSCRIPTEN)
    set(PLATFORM_SPECIFIC_INCLUDES
        GLESv2
    )
elseif(ANDROID)
    set(PLATFORM_SPECIFIC_INCLUDES
        GLESv2
        log
        android
    )
else()
    #find_package(OpenGL REQUIRED)
    set(PLATFORM_SPECIFIC_INCLUDES
        opengl32
    )
endif()

set(CMAKE_CXX_STANDARD 17)

find_library(SDL2 SDL2)

file(GLOB GAME_SOURCES "src/*")

if (ANDROID)
	set(TARGET_NAME main)
else()
	set(TARGET_NAME ${PROJECT_NAME})
endif()

if (ANDROID)
	add_library(${TARGET_NAME} SHARED)

	target_sources(${TARGET_NAME} PRIVATE ${GAME_SOURCES})
else()
	add_executable(${TARGET_NAME} WIN32 ${GAME_SOURCES} resource/resource.rc)
endif()

if (EMSCRIPTEN)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s WASM=1 -s USE_SDL=2 -s DEMANGLE_SUPPORT=1 --preload-file ${CMAKE_SOURCE_DIR}/assets_compiled@/")
endif()

#target_link_libraries(main
target_link_libraries(${TARGET_NAME}
	glm::glm_static
	flecs::flecs_static
	lua_static
	chipmunk
	${SDL2}
    ${SDL2_mixer}
	${PLATFORM_SPECIFIC_INCLUDES}
)

# TODO: Add tests and install targets if needed.
