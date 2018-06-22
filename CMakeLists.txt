cmake_minimum_required(VERSION 3.7)
project(sound-net)

set(sound-net_VERSION_MAJOR 1)
set(sound-net_VERSION_MINOR 0)

# include portaudio lib
set(MSVS true)
set(PA_USE_ASIO true)
set(PA_USE_WASAPI true)
set(PA_USE_WDMKS true)
add_subdirectory(portaudio)
find_library(portaudio ./portaudio)

# include dspfilters lib
add_subdirectory(DSPFilters)
find_library(DSPFilters ./DSPFilters)
include_directories(./DSPFilters/)

message(STATUS portaudio libraries: ${PA_LIBRARIES})

#include fftw lib
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTS OFF)

#set(ENABLE_SSE ON)
#set(ENABLE_SSE2 ON)
#set(ENABLE_AVX ON)
set(ENABLE_AVX2 ON)
set(ENABLE_FLOAT ON)
add_subdirectory(fftw)
#find_library(fftw3 ./fftw)
find_library(fftw3f ./fftw)
include_directories(./fftw/api/)

#include sources
#include_directories(include)
file(GLOB SOURCES "src/*.cpp" "src/*.h" "include/*.h")

add_executable(snet ${SOURCES})
target_link_libraries(snet portaudio_static DSPFilters fftw3f)

