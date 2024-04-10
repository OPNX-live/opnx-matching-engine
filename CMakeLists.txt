cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

set(VERSION dev)
set(SUBMODULE_VERSION dev)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

project(matching_engine)


if(APPLE)
    link_directories(/usr/lib/ /usr/local/lib/ /usr/local/openssl/lib )
    message(STATUS "Now is Mac OS")
elseif(UNIX)
    link_directories(/usr/lib/ /usr/lib64/ /usr/local/lib/ /usr/local/lib64 /usr/local/openssl/lib)
    message(STATUS "Now is Linux OS")
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)

include_directories(${PROJECT_SOURCE_DIR}/include /usr/include /usr/local/include)

add_subdirectory(manager)
#add_subdirectory(test)