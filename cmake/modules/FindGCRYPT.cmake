# - Find gcrypt
# Find the native GCRYPT includes and library
#
#  GCRYPT_INCLUDE_DIRS - where to find gcrypt.h, etc.
#  GCRYPT_LIBRARIES    - List of libraries when using gcrypt.
#  GCRYPT_FOUND        - True if gcrypt found.


IF (GCRYPT_INCLUDE_DIRS)
  # Already in cache, be silent
  SET(GCRYPT_FIND_QUIETLY TRUE)
ENDIF (GCRYPT_INCLUDE_DIRS)

FIND_PATH(GCRYPT_INCLUDE_DIR gcrypt.h)

SET(GCRYPT_NAMES gcrypt)
FIND_LIBRARY(GCRYPT_LIBRARY NAMES ${GCRYPT_NAMES} )
FIND_LIBRARY(GCRYPT_ERROR_LIBRARY NAMES gpg-error )

# handle the QUIETLY and REQUIRED arguments and set GCRYPT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GCRYPT DEFAULT_MSG GCRYPT_LIBRARY GCRYPT_INCLUDE_DIR)

IF(GCRYPT_FOUND)
  SET( GCRYPT_LIBRARIES ${GCRYPT_LIBRARY} ${GCRYPT_ERROR_LIBRARY})
ELSE(GCRYPT_FOUND)
  SET( GCRYPT_LIBRARIES )
ENDIF(GCRYPT_FOUND)

MARK_AS_ADVANCED( GCRYPT_LIBRARIES GCRYPT_INCLUDE_DIRS )
