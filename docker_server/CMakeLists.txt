cmake_minimum_required(VERSION 3.10)
project(VoiceServer)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

include(FindPkgConfig)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(server server.cpp)

target_link_libraries(server 
    PRIVATE 
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    ${PORTAUDIO_LIBRARIES}
)

target_include_directories(server PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
target_link_directories(server PRIVATE ${PORTAUDIO_LIBRARY_DIRS})
