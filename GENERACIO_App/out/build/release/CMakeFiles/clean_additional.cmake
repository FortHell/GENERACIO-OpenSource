# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\GENERACIO_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\GENERACIO_autogen.dir\\ParseCache.txt"
  "GENERACIO_autogen"
  )
endif()
