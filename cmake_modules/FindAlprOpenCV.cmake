# - Try to find AlprOpencv library
# Once done, this will define
#
#  AlprOpenCV_FOUND - system has AlprOcr
#  OpenCV_INCLUDE_DIRS - the AlprOcr include directories
#  OpenCV_LIBS - link these to use OpenCV



# Include dir
find_path(OpenCV_INCLUDE_DIRS
  NAMES opencv2/opencv.hpp
  HINTS ${CMAKE_SOURCE_DIR}/../libalpropencv/include
        "/usr/include/"
        "/usr/local/include/"
        
)

# Finally the library itself
find_library(OpenCV_LIBS
  NAMES alpropencv alpropencv310
  HINTS ${CMAKE_SOURCE_DIR}/../libalpropencv/build/lib/
        "/usr/lib"
        "/usr/local/lib"
        "/opt/local/lib"
		
)


# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.

SET(OpenCV_VERSION_MAJOR 3)
SET(OpenCV_VERSION_MINOR 1)
SET(OpenCV_VERSION_PATCH 0)
set(OpenCV_VERSION 3.1.0)


