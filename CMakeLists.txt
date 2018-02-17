# Debug Releaseに限定
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

# プロジェクト名
project(OpenSoundMixer)

#
option(BUILD_TEST "Build test" OFF)
option(XAUDIO2 "Enabled EAudio2" OFF)

# CMake バージョンチェック
if (MSVC)
cmake_minimum_required(VERSION 2.8.11)
else()
cmake_minimum_required(VERSION 2.8)
endif()

# コンパイル設定
if (MSVC)
	# 特に何も
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC" ${CMAKE_CXX_FLAGS})
endif()

# ソースコード設定
set(common_src
	src/osm.Decorder.cpp
	src/osm.Manager.cpp
	src/osm.Manager_Impl.cpp
	src/osm.ReferenceObject.cpp
	src/osm.Sound_Impl.cpp
	src/Decorder/osm.OggDecorder.cpp
	src/Decorder/osm.WaveDecorder.cpp
	src/Filter/osm.Resampler.cpp
	src/Filter/osm.Panner.cpp

	src/3rdParty/libogg/src/bitwise.c
	src/3rdParty/libogg/src/framing.c
	src/3rdParty/libvorbis/src/analysis.c
	src/3rdParty/libvorbis/src/bitrate.c
	src/3rdParty/libvorbis/src/block.c
	src/3rdParty/libvorbis/src/codebook.c
	src/3rdParty/libvorbis/src/envelope.c
	src/3rdParty/libvorbis/src/floor0.c
	src/3rdParty/libvorbis/src/floor1.c
	src/3rdParty/libvorbis/src/info.c
	src/3rdParty/libvorbis/src/lookup.c
	src/3rdParty/libvorbis/src/lpc.c
	src/3rdParty/libvorbis/src/lsp.c
	src/3rdParty/libvorbis/src/mapping0.c
	src/3rdParty/libvorbis/src/mdct.c
	src/3rdParty/libvorbis/src/psy.c
	src/3rdParty/libvorbis/src/registry.c
	src/3rdParty/libvorbis/src/res0.c
	src/3rdParty/libvorbis/src/sharedbook.c
	src/3rdParty/libvorbis/src/smallft.c
	src/3rdParty/libvorbis/src/synthesis.c
	src/3rdParty/libvorbis/src/vorbisenc.c
	src/3rdParty/libvorbis/src/vorbisfile.c
	src/3rdParty/libvorbis/src/window.c

	)

set(xaudio2_src
	src/Manager/osm.Manager_Impl_XAudio2.cpp)

set(wasapi_src
	src/Manager/osm.Manager_Impl_WasApi.cpp)

set(pulse_audio_src
	src/Manager/osm.Manager_Impl_PulseAudio.cpp)

set(open_al_src
	src/Manager/osm.Manager_Impl_OpenAL.cpp)

if (BUILD_TEST)
	set(test_src
		src_test/main.cpp)
endif()

if(WIN32)

	if(XAUDIO2)
		list(APPEND common_src
			${xaudio2_src})
		add_definitions(-DOSM_OUTPUT_XAUDIO2)
	else()
		list(APPEND common_src
			${wasapi_src})
		add_definitions(-DOSM_OUTPUT_WASAPI)
	endif()

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	list(APPEND common_src
		${open_al_src})
else()
	list(APPEND common_src
		${pulse_audio_src})
endif()

if (BUILD_TEST)
	list(APPEND test_src
		${common_src})
endif()

# インクルードの設定
include_directories(
	src/
	src/3rdParty/libogg/include/
	src/3rdParty/libvorbis/include/
	src/3rdParty/libvorbis/src/)
# 

link_directories(
	${PROJECT_SOURCE_DIR}/lib/
	${PROJECT_BINARY_DIR}/lib/
	lib/)

# コンパイル先を追加
if (BUILD_TEST)
	add_executable( osmtest ${test_src} )
endif()

add_library( osmlib STATIC ${common_src} )

if (MSVC)

else()
	list(APPEND common_lib dl )
	list(APPEND common_lib OpenSoundMixer )

	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		FIND_PACKAGE(OpenAL REQUIRED)
				
		if (BUILD_TEST)
			list(APPEND osmtest ${OpenAL_LIBRARY})
			target_link_libraries(osmtest "-framework OpenAL")
		endif()
	else()
		FIND_PACKAGE(PulseAudio REQUIRED)

		if (BUILD_TEST)
			list(APPEND osmtest ${PulseAudio_LIBRARY})
		endif()
	endif()

	find_package(Threads REQUIRED)
	list(APPEND common_lib ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_TEST)
	target_link_libraries(osmtest ${common_lib}) 
endif()

# フラグ処理
if (MSVC)
	if (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
		foreach (flag CMAKE_C_FLAGS
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
			CMAKE_CXX_FLAGS
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_RELEASE)

			if (${flag} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
			endif()
			if (${flag} MATCHES "/MDd")
				string(REGEX REPLACE "/MDd" "/MTd" ${flag} "${${flag}}")
			endif()
		endforeach()
	endif()
endif()

# 出力先の設定
if (MSVC)
set_target_properties(osmlib PROPERTIES OUTPUT_NAME_DEBUG "OpenSoundMixer" )
set_target_properties(osmlib PROPERTIES OUTPUT_NAME_RELEASE "OpenSoundMixer" )

if (BUILD_TEST)
	set_target_properties(osmtest PROPERTIES OUTPUT_NAME_DEBUG "OpenSoundMixerTest" )
	set_target_properties(osmtest PROPERTIES OUTPUT_NAME_RELEASE "OpenSoundMixerTest" )
endif()

else()
set_target_properties(osmlib PROPERTIES OUTPUT_NAME "OpenSoundMixer" )

if (BUILD_TEST)
	set_target_properties(osmtest PROPERTIES OUTPUT_NAME "OpenSoundMixerTest" )
endif()

set_target_properties(osmlib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "lib/" )
set_target_properties(osmlib PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lib/" )
endif()

install(
	TARGETS osmlib
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib)

if (BUILD_TEST)
	install(
		TARGETS osmtest
		RUNTIME DESTINATION bin
		ARCHIVE DESTINATION bin
		LIBRARY DESTINATION bin)
endif()
