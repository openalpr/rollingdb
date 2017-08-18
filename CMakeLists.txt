project(rollingdb)

set(CMAKE_BUILD_TYPE RelWithDebugInfo)
cmake_minimum_required (VERSION 2.6)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

option(BUILD_TESTS "Use CUDA support" OFF)

FIND_PACKAGE( log4cplus REQUIRED )


ADD_DEFINITIONS(
    -std=c++11 
)
        
include_directories(
  ${CMAKE_SOURCE_DIR}/src
)

add_library(rollingdb 	SHARED 
  
    src/rollingdb.cpp 
    src/rollingdb_impl.cpp 
    src/chunkcollection.cpp 
    src/lmdbchunk.cpp 
    src/system_utils.cpp
)

TARGET_LINK_LIBRARIES(rollingdb
    lmdb
    re2
    pthread
    ${log4cplus_LIBRARIES}
)


set_target_properties(rollingdb PROPERTIES SOVERSION 2)

install (FILES   src/rollingdb.h     DESTINATION    ${CMAKE_INSTALL_PREFIX}/include)
install (TARGETS rollingdb     DESTINATION    ${CMAKE_INSTALL_PREFIX}/lib)


IF (WIN32)
  SET(WINDOWS_LIBS ws2_32)
ENDIF()

IF (BUILD_TESTS)
  add_subdirectory(tests)
ENDIF()
