cmake_minimum_required(VERSION 3.6)
project(PiXiuPlus)
set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES
        src/main.cpp
        src/common/MemPool.cpp
        src/data_struct/CritBitTree.cpp
        src/data_struct/SuffixTree.cpp
        src/proj/PiXiuCtrl.cpp
        src/proj/PiXiuStr.cpp
        )
add_executable(PiXiuPlus ${SOURCE_FILES})