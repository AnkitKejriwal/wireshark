# - Find math
# Find the native MATH includes and library
#
#  MATH_INCLUDE_DIR - where to find math.h, etc.
#  MATH_LIBRARIES   - List of libraries when using math.
#  MATH_FOUND       - True if math found.


IF (MATH_INCLUDE_DIR)
  # Already in cache, be silent
  SET(MATH_FIND_QUIETLY TRUE)
ENDIF (MATH_INCLUDE_DIR)

FIND_PATH(MATH_INCLUDE_DIR math.h)

SET(MATH_NAMES m)
FIND_LIBRARY(MATH_LIBRARY NAMES ${MATH_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set MATH_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATH DEFAULT_MSG MATH_LIBRARY MATH_INCLUDE_DIR)

IF(MATH_FOUND)
  SET( MATH_LIBRARIES ${MATH_LIBRARY} )
ELSE(MATH_FOUND)
  SET( MATH_LIBRARIES )
ENDIF(MATH_FOUND)

MARK_AS_ADVANCED( MATH_LIBRARY MATH_INCLUDE_DIR )
