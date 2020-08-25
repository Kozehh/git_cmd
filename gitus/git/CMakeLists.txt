add_library(git
    gituscommands.h 
    gituscommands.cpp)
if (UNIX)
target_link_libraries(git 
    PUBLIC 
		${Boost_LIBRARIES}
		${ZLIB_LIBRARIES}
)
else ()
target_link_libraries(git 
    PUBLIC 
		${Boost_LIBRARIES}
)
endif ()	
target_include_directories(git 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		${Boost_INCLUDE_DIRS}
)
