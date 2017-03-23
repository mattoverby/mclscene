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

if(NOT MCL_FORCE_CLONE)
find_package(Eigen3)
endif()

if(NOT EIGEN3_FOUND)
	set(EIGEN3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/eigen3 )
	IF(NOT EXISTS ${EIGEN3_DIR})
		message("Eigen3 not found, cloning into ${EIGEN3_DIR}")
#		execute_process(COMMAND git clone https://github.com/OPM/eigen3 ${EIGEN3_DIR})
		execute_process(COMMAND hg clone https://bitbucket.org/eigen/eigen/ ${EIGEN3_DIR})
	endif()
	set(EIGEN3_INCLUDE_DIR  ${EIGEN3_DIR})
	set(EIGEN3_FOUND true)
endif()

