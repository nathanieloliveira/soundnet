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
include_directories(./DSPFilters/include)

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

# find boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.68.0)

#include sources
#include_directories(include)
file(GLOB SOURCES "src/*.cpp" "src/*.h" "include/*.h")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSNET_DEBUG")

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(snet ${SOURCES})
    target_link_libraries(snet portaudio_static DSPFilters fftw3f)
endif()

