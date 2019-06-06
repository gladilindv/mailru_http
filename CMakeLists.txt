cmake_minimum_required(VERSION 3.8)
project(mailru)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp session.cpp session.h uri.cpp uri.h)
add_executable(mailru ${SOURCE_FILES})