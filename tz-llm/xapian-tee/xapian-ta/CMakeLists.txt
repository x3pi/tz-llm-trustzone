cmake_minimum_required(VERSION 3.10)
project(XapianTA CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-fexceptions)
string(REPLACE "-fno-exceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# Include ChCore syscall headers
# (They should be automatically included by toolchain.cmake if we find them)
include_directories(/home/vectorxj/chcore/staros/.cpp/aarch64/include)
include_directories(/home/vectorxj/chcore/staros/.cpp/aarch64/include/aarch64-linux-musleabi)
include_directories(/home/vectorxj/chcore/staros/build/chcore-libc/install/include)

# Include Xapian headers
include_directories(/home/vectorxj/xapian-tee/install/include)

add_executable(xapian-ta main.cpp)

# Link against Xapian and Zlib
target_link_libraries(xapian-ta
    /home/vectorxj/xapian-tee/install/lib/libxapian.a
    /home/vectorxj/xapian-tee/install/lib/libz.a
)
