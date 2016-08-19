// Copyright 2016 Matthew Overby.
// 
// MCLSCENE Uses the BSD 2-Clause License (http://www.opensource.org/licenses/BSD-2-Clause)
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
// THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF MINNESOTA, DULUTH OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// By Matt Overby (http://www.mattoverby.net)

#ifndef MCLSCENE_SIMULATION_H
#define MCLSCENE_SIMULATION_H 1

#include "TriMesh.h"
#include "MCL/Param.hpp"

namespace mcl {

//
//	Simulation is the class a physics engine would derive from
//	for an easy plug-in to the renderer.
//	In the Gui class:
//	1) The scene is loaded into mclscene
//	2) TriMeshes are passed to the initialize function
//	3) Step is called by the gui to invoke a timestep
//	4) Update is called (after step) by the gui to update the mesh vertices
//
//	All functions should return true on success
//
//	It's also advisable to give your simulator access to mclscene
//	via constructor, but not required.
//
class Simulator  {
public:

	virtual bool initialize( const std::vector< trimesh::TriMesh* > &meshes,
		const std::vector< std::vector<mcl::Param> > &params ) = 0;
	virtual bool step( float screen_dt ) = 0;
	virtual bool update( std::vector< trimesh::TriMesh* > &meshes ) = 0;

}; // end class simulation


} // end namespace mcl

#endif
