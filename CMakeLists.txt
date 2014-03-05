# Debug Releaseに限定
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

# プロジェクト名
project(OpenSoundMixer)

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
	src/Decorder/osm.WaveDecorder.cpp)

set(xaudio2_src
	src/Manager/osm.Manager_Impl_XAudio2.cpp)

set(pulse_audio_src
	src/Manager/osm.Manager_Impl_PulseAudio.cpp)

set(test_src
	src_test/main.cpp)

if(WIN32)
list(APPEND common_src
	${xaudio2_src})
else()
list(APPEND common_src
	${pulse_audio_src})
endif()

list(APPEND test_src
	${common_src})


# インクルードの設定
include_directories(
	src/
	)

# 
link_directories(${PROJECT_SOURCE_DIR}/lib/)

# コンパイル先を追加
add_executable( osmtest ${test_src} )
add_library( osmlib STATIC ${common_src} )

if (MSVC)

else()
	list(APPEND common_lib dl )
	list(APPEND common_lib ogg )
	list(APPEND common_lib vorbisfile )
	list(APPEND common_lib OpenSoundMixer )

	FIND_PACKAGE(PulseAudio REQUIRED)
	list(APPEND osmtest ${PulseAudio_LIBRARY})

	find_package(Threads REQUIRED)
	list(APPEND common_lib ${CMAKE_THREAD_LIBS_INIT})
endif()

target_link_libraries(osmtest ${common_lib}) 

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
set_target_properties(osmtest PROPERTIES OUTPUT_NAME_DEBUG "OpenSoundMixerTest" )
set_target_properties(osmtest PROPERTIES OUTPUT_NAME_RELEASE "OpenSoundMixerTest" )
else()
set_target_properties(osmlib PROPERTIES OUTPUT_NAME "OpenSoundMixer" )
set_target_properties(osmtest PROPERTIES OUTPUT_NAME "OpenSoundMixerTest" )

set_target_properties(osmlib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "../lib/" )
set_target_properties(osmlib PROPERTIES LIBRARY_OUTPUT_DIRECTORY "../lib/" )
endif()

install(
	TARGETS osmlib
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib)

install(
	TARGETS osmtest
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION bin
	LIBRARY DESTINATION bin)
