# - Try to find AlprNeural library
# Once done, this will define
#
#  AlprSupport_FOUND - system has AlprOcr
#  AlprSupport_INCLUDE_DIR - the AlprOcr include directories
#  AlprSupport_LIBRARIES - link these to use AlprOcr



# Include dir
find_path(AlprSupport_INC
  NAMES alprsupport/platform.h
  HINTS "/usr/include"
        "/usr/local/include"
        ${CMAKE_SOURCE_DIR}/../alprsupport/
)

# Finally the library itself
find_library(AlprSupport_LIB
  NAMES alprsupport
  HINTS "/usr/lib"
        "/usr/local/lib"
        "/opt/local/lib"
		${CMAKE_SOURCE_DIR}/../alprsupport/build
)


# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.

set( AlprSupport_LIBRARIES      ${AlprSupport_LIB} )
set( AlprSupport_INCLUDE_DIR   ${AlprSupport_INC} )


