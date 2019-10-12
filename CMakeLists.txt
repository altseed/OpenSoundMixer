
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

project(OpenSoundMixer)

#
option(BUILD_TEST "Build test" OFF)

cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 11)

# download testdata
if (BUILD_TEST)
    find_package(PythonInterp 3)

    add_custom_target(TestData
        SOURCES TestData.dummy
    )

    add_custom_command(
        OUTPUT TestData.dummy
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup.py ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# sanitize for Debug
if(SANITIZE_ENABLED)
    set(CMAKE_CXX_FLAGS " -fsanitize=undefined,address" ${CMAKE_CXX_FLAGS})
endif()

# Add codes

file(GLOB_RECURSE osm_src
    thirdparty/*.c
    thirdparty/*.h
    src/Decorder/*.h
    src/Decorder/*.cpp
    src/Filter/*.h
    src/Filter/*.cpp)

file(GLOB osm_root_src
    src/*.h
    src/*.cpp)

list(APPEND osm_src ${osm_root_src})

set(wasapi_src
    src/Manager/osm.Manager_Impl_WasApi.cpp)

set(pulse_audio_src
    src/Manager/osm.Manager_Impl_PulseAudio.cpp)

set(open_al_src
    src/Manager/osm.Manager_Impl_OpenAL.cpp)

if (BUILD_TEST)
    set(test_src
        test/main.cpp)
endif()

if(WIN32)

    list(APPEND osm_src
        ${wasapi_src})

elseif(APPLE)
    list(APPEND osm_src
        ${open_al_src})
else()
    list(APPEND osm_src
        ${pulse_audio_src})
endif()

set (files ${osm_src} ${test_src})

foreach(FILE ${files}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")
    
    # Group into "Source Files" and "Header Files"
    if ("${FILE}" MATCHES ".*\\.cpp")
       set(GROUP "${GROUP}")
    elseif("${FILE}" MATCHES ".*\\.c")
       set(GROUP "${GROUP}")
    elseif("${FILE}" MATCHES ".*\\.cxx")
       set(GROUP "${GROUP}")
    elseif("${FILE}" MATCHES ".*\\.h")
       set(GROUP "${GROUP}")
    elseif("${FILE}" MATCHES ".*\\.mm")
       set(GROUP "${GROUP}")
    endif()

    source_group("${GROUP}" FILES "${FILE}")
endforeach()



include_directories(
    src/
    thirdparty/libogg/include/
    thirdparty/libvorbis/include/
    thirdparty/libvorbis/src/)
# 

link_directories(
    ${PROJECT_SOURCE_DIR}/lib/
    ${PROJECT_BINARY_DIR}/lib/
    lib/)

add_library( OpenSoundMixer STATIC ${osm_src} )

if (BUILD_TEST)
    add_executable( OpenSoundMixerTest ${test_src} )
    target_include_directories(OpenSoundMixerTest PRIVATE src)
    add_dependencies(OpenSoundMixerTest TestData)
endif()

if (MSVC)

else()
    list(APPEND common_lib dl )

    if(APPLE)
        FIND_PACKAGE(OpenAL REQUIRED)
                
        if (BUILD_TEST)
            list(APPEND common_lib ${OpenAL_LIBRARY})
            target_link_libraries(OpenSoundMixerTest PRIVATE "-framework OpenAL")
        endif()
    else()
        set_property(TARGET OpenSoundMixer PROPERTY POSITION_INDEPENDENT_CODE TRUE) 
        FIND_PACKAGE(PulseAudio REQUIRED)

        if (BUILD_TEST)
            list(APPEND common_lib ${PulseAudio_LIBRARY})
        endif()
    endif()

    find_package(Threads REQUIRED)
    list(APPEND common_lib ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_TEST)
    target_link_libraries(OpenSoundMixerTest PRIVATE ${common_lib}) 
    target_link_libraries(OpenSoundMixerTest PRIVATE OpenSoundMixer)

    if(NOT WIN32 AND NOT APPLE)
        target_link_libraries(OpenSoundMixerTest PRIVATE dl)
    endif()
endif()


if (MSVC)
    set_target_properties(OpenSoundMixerTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" )
    set_target_properties(OpenSoundMixer PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "lib/" )
    set_target_properties(OpenSoundMixer PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lib/" )
else()
    set_target_properties(OpenSoundMixerTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" )
    set_target_properties(OpenSoundMixer PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "lib/" )
    set_target_properties(OpenSoundMixer PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lib/" )
endif()

install(
    TARGETS OpenSoundMixer
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

if (BUILD_TEST)
    install(
        TARGETS OpenSoundMixerTest
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION bin
        LIBRARY DESTINATION bin)
endif()
