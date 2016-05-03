# Copyright 2014 Matthew Overby.
# 
# MCLSCENE Uses the BSD 2-Clause License (http://www.opensource.org/licenses/BSD-2-Clause)
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other materials
#    provided with the distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF MINNESOTA, DULUTH OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# By Matt Overby (http://www.mattoverby.net)

cmake_minimum_required(VERSION 2.6)
project(mclscene C CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set( CMAKE_BUILD_TYPE Debug )

#
#	compilation options
#
option(MCL_BUILD_SAMPLES "Build mcl library samples" ON)


#
#	dependencies
#

# trimesh, included in deps/
set( TRIMESH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/trimesh2" )
add_subdirectory( ${TRIMESH_DIR} ) # sets TRIMESH_INCLUDE and TRIMESH_SRCS
include_directories( ${TRIMESH_INCLUDE} )

# Opengl and SFML used by the gui, not required
find_package(OpenGL)
find_package(SFML 2 COMPONENTS system window graphics)

# OpenMP, not required
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Boost
set( Boost_USE_STATIC_LIBS ON )
find_package( Boost COMPONENTS filesystem system REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )

#
#	mclscene library
#

# Set up the library, add gui sources if opengl/sfml are found
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set( MCLSCENE_SRCS
	include/MCL/SceneManager.hpp	src/SceneManager.cpp
	include/MCL/TetMesh.hpp		src/TetMesh.cpp
	include/MCL/MeshDump.hpp	src/MeshDump.cpp
	include/MCL/Param.hpp		src/Param.cpp
	include/MCL/Object.hpp		src/Object.cpp
	include/MCL/BVH.hpp		src/BVH.cpp
	include/MCL/TriangleMesh.hpp	src/TriangleMesh.cpp
	include/MCL/VertexSort.hpp
	include/MCL/RenderUtils.hpp
	include/MCL/Camera.hpp
	include/MCL/Light.hpp
	include/MCL/AABB.hpp
	include/MCL/Material.hpp
	include/MCL/RayIntersect.hpp
	include/MCL/DefaultBuilders.hpp
)

# Create the library
if(SFML_FOUND AND OPENGL_FOUND)
	set( MCLSCENE_SRCS ${MCLSCENE_SRCS} include/MCL/Gui.hpp src/Gui.cpp )
	add_library( mclscene ${TRIMESH_SRCS} ${MCLSCENE_SRCS} )
	set( MCLSCENE_LIBRARIES mclscene ${OPENGL_LIBRARIES} ${SFML_LIBRARIES} ${Boost_LIBRARIES} png )
elseif(SFML_FOUND AND OPENGL_FOUND)
	add_library( mclscene ${TRIMESH_SRCS} ${MCLSCENE_SRCS} )
	set( MCLSCENE_LIBRARIES mclscene ${Boost_LIBRARIES} )
endif(SFML_FOUND AND OPENGL_FOUND)

# Set the libraries variable in parent scope if exists
get_directory_property(HasParent PARENT_DIRECTORY)
if(HasParent)
	set( MCLSCENE_LIBRARIES ${MCLSCENE_LIBRARIES} PARENT_SCOPE )
	set( MCLSCENE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${TRIMESH_INCLUDE} PARENT_SCOPE )
endif(HasParent)

#
#	samples
#

if(MCL_BUILD_SAMPLES)

	# sample loader
	add_executable( load_test samples/LoadTest.cpp )
	target_link_libraries( load_test ${MCLSCENE_LIBRARIES} )

	# viewer sample
	if(SFML_FOUND AND OPENGL_FOUND)
		include_directories( ${SFML_INCLUDE_DIRS} ${OpenGL_INCLUDE_DIRS} )
		link_directories( ${OpenGL_LIBRARY_DIRS} ${SFML_LIBRARY_DIRS} )
		add_definitions( ${OpenGL_DEFINITIONS} )
		add_executable( mesh_viewer samples/MeshViewer.cpp )
		target_link_libraries( mesh_viewer ${MCLSCENE_LIBRARIES} )
	endif(SFML_FOUND AND OPENGL_FOUND)

endif(MCL_BUILD_SAMPLES)
