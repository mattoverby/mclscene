# Set  the minimum  required version  of cmake  for a  project.
cmake_minimum_required(VERSION 2.6)

# Add an executable to the project using the specified source files.
#add_executable(tetgen tetgen.cxx predicates.cxx)

# Build as a library
add_definitions( -DTETLIBRARY )

#Add a library to the project using the specified source files. 
# In Linux/Unix, it will creates the libtet.a
#add_library(tet STATIC tetgen.cxx predicates.cxx)
set( TETGEN_INCLUDE
	${CMAKE_CURRENT_SOURCE_DIR}
PARENT_SCOPE )

set( TETGEN_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/tetgen.h
	${CMAKE_CURRENT_SOURCE_DIR}/tetgen.cxx
	${CMAKE_CURRENT_SOURCE_DIR}/predicates.cxx
PARENT_SCOPE )

#Set properties on a target. 
#We use this here to set -DTETLIBRARY for when compiling the
#library
#set_target_properties(tet PROPERTIES "COMPILE_DEFINITIONS" TETLIBRARY)
