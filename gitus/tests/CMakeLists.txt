add_executable(gittests 
    unittests.cpp
	catch.hpp
)


target_link_libraries(gittests
    PRIVATE
    git
)

target_include_directories(gittests
    PRIVATE
    git
)



