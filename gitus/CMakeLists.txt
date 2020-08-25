cmake_minimum_required(VERSION 3.12)
project(GitUS
    VERSION 1.0
    LANGUAGES C CXX
    DESCRIPTION "Client git")

set(Boost_USE_STATIC_LIBS on)
set(Boost_USE_MULTITHREADED ON)

if (UNIX)
find_package(ZLIB REQUIRED) 
find_package(Boost 1.71.0 REQUIRED COMPONENTS iostreams filesystem)
else ()
find_package(Boost 1.71.0 REQUIRED COMPONENTS iostreams filesystem zlib)
endif ()
		
set(CXX_STANDARD 17)

add_subdirectory(git)
add_subdirectory(tests)

add_executable(gitus gitus.cpp)

target_link_libraries(gitus
    PRIVATE
        git
)
