# Copyright (c) 2017 University of Minnesota
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

find_package(GLFW 3)

if(NOT GLFW_FOUND)
	set( GLFW_CLONED true )

	set(glfw_checkout_Dir ${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw)
	make_directory(${glfw_checkout_Dir})
	message("GLFW3 not found, cloning into ${glfw_checkout_Dir}")

	set(glfw_PREFIX "${glfw_checkout_Dir}")
	set(glfw_INSTALL_DIR "${glfw_checkout_Dir}/install")

	set(glfw_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${glfw_INSTALL_DIR} -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF )
	set(glfw_DIR "${glfw_INSTALL_DIR}")

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(glfw_CMAKE_ARGS ${glfw_CMAKE_ARGS} -DCMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD="c++11" -DCMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY="libc++")
	endif()

	ExternalProject_add(glfw
		PREFIX ${glfw_PREFIX}
		GIT_REPOSITORY https://github.com/glfw/glfw
		INSTALL_DIR ${glfw_INSTALL_DIR}
		CMAKE_ARGS ${glfw_CMAKE_ARGS}
		UPDATE_COMMAND ""
	)
	 
	set_property(TARGET glfw PROPERTY FOLDER "Dependencies")
	set(GLFW_INCLUDE_DIR ${glfw_INSTALL_DIR}/include CACHE INTERNAL "Directory of GLFW header files")

	# Windows Section #
	if (MSVC)
		set(GLFW_LIBRARY ${glfw_INSTALL_DIR}/lib/glfw3.lib CACHE INTERNAL "GLFW lib file")
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		# Tell MSVC to use main instead of WinMain for Windows subsystem executables
		set_target_properties(${WINDOWS_BINARIES} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
	endif()

	# Apple section
	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(GLFW_LIBRARY ${glfw_INSTALL_DIR}/lib/libglfw3.a CACHE INTERNAL "GLFW lib file")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
		find_library(COCOA_LIB Cocoa)
		find_library(CARBON_LIB Carbon)
		find_library(IOKIT_LIB IOKit)
		find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
		find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
		set(LIBS_ALL ${LIBS_ALL} ${COCOA_LIB} ${CARBON_LIB} ${IOKIT_LIB} ${CORE_FOUNDATION_FRAMEWORK} ${CORE_VIDEO_FRAMEWORK})
		message(STATUS "${CORE_VIDEO_FRAMEWORK}")
	endif()

	# Linux section
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set(GLFW_LIBRARY ${glfw_INSTALL_DIR}/lib/libglfw3.a CACHE INTERNAL "GLFW lib file")
		find_package(Threads)
		find_package(X11)
		set(LIBS_ALL ${LIBS_ALL} ${CMAKE_THREAD_LIBS_INIT} rt Xrandr Xxf86vm Xi Xcursor Xinerama m dl ${X11_LIBRARIES})
	endif()

	set(GLFW_LIBRARIES ${GLFW_LIBRARY} ${LIBS_ALL})
endif()
