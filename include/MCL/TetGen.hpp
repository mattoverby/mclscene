// Copyright (c) 2017 University of Minnesota
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
//	NOTE: Requires Tetgen 1.5.0
//	This file is just a wrapper for Tetgen (wias-berlin.de/software/tetgen)
//	by Hang Si. It's assumed you've already linked the appropriate libraries.
//
//	A copy of tetgen version 1.5.0 is in the src/tetgen directory. For an
//	example on how to build and link it, see the CMakeLists.txt in mclscene
//	root directory and the src/tests/test_tetgen test file.
//

#ifndef MCL_TETGEN_H
#define MCL_TETGEN_H

#include "Vec.hpp"
#define TETLIBRARY
#include "tetgen.h"

namespace mcl {

namespace tetgen {

	struct Settings {
		bool verbose;
		float quality;
		float maxvol;
		float maxvol_percent; // compute max vol as a fraction (0-1) of the total
		Settings() : verbose(false), quality(-1), maxvol(-1), maxvol_percent(-1) {}
		void print() const;
	};

	// Tetrahedralizes a triangle mesh, returns true on success.
	static bool make_tetmesh( std::vector<Vec4i> &tets, std::vector<Vec3f> &tet_verts,
		const std::vector<Vec3i> &tris, const std::vector<Vec3f> &tri_verts,
		const Settings &settings = Settings() );

} // end ns tetgen

//
//  Implementation
//

static bool tetgen::make_tetmesh( std::vector<Vec4i> &tets, std::vector<Vec3f> &tet_verts,
		const std::vector<Vec3i> &tris, const std::vector<Vec3f> &tri_verts, const Settings &settings ){

	// TetGen uses double for vertices. Kind of annoying, but it's too much
	// work to change it to allow floats. Easier to cast input to double...
	int n_triverts = tri_verts.size();
	std::vector<Vec3d> triverts( n_triverts );
	Eigen::AlignedBox<float,3> aabb;
	for( int i=0; i<n_triverts; ++i ){
		aabb.extend( tri_verts[i] );
		triverts[i] = tri_verts[i].cast<double>();
	}

	// We'll make a copy of tris too, since we want to keep the input const
	int n_tris = tris.size();
	std::vector<Vec3i> faces = tris;

	tetgenio in;
	in.mesh_dim = 3;
	in.pointlist = &triverts[0][0];
	in.numberofpoints = n_triverts;
	in.trifacelist = &faces[0][0];
	in.numberoftrifaces = n_tris;

	// Compute maxvol if we want it
	float maxvol = settings.maxvol;
	if( settings.maxvol_percent > 0.f ){
		float mvp = std::min( settings.maxvol_percent, 1.f );
		maxvol = mvp * aabb.volume();
	}

	// Set up the switches
	std::stringstream switches;
	if( settings.verbose ){ switches << "V"; }
	else{ switches << "Q"; }
	if( settings.quality > 0 ){ switches << "q" << settings.quality; }
	if( maxvol > 0 ){ switches << "a" << maxvol; }

	char *c_switches = new char[switches.str().length()+1];
	std::strcpy(c_switches,switches.str().c_str());

	tetgenio out;
	tetrahedralize(c_switches, &in, &out);

	// Set tetgenio input data back to NULL so
	// it doesn't try to deallocate.
	in.pointlist = NULL;
	in.trifacelist = NULL;
	delete c_switches;

	// Make sure we had success
	if( out.numberoftetrahedra == 0 || out.numberofpoints == 0 ){
		std::cerr << "**TetGen Error: Failed to tetrahedralize with settings:" << std::flush;
		settings.print();
		return false;
	}

	tets.clear(); // Copy tets
	tets.reserve( out.numberoftetrahedra );
	for( int i=0; i<out.numberoftetrahedra; ++i ){
		tets.emplace_back(
			out.tetrahedronlist[i*4+0],
			out.tetrahedronlist[i*4+1],
			out.tetrahedronlist[i*4+2],
			out.tetrahedronlist[i*4+3]
		);
	}

	tet_verts.clear(); // Copy verts
	tet_verts.reserve( out.numberofpoints );
	for( int i=0; i<out.numberofpoints; ++i ){
		tet_verts.emplace_back(
			out.pointlist[i*3+0],
			out.pointlist[i*3+1],
			out.pointlist[i*3+2]
		);
	}

	return true;

} // end tetrahedralize


void tetgen::Settings::print() const {
	std::cout <<
		"\n\t verbose: " << verbose <<
		"\n\t quality: " << quality <<
		"\n\t maxvol: " << maxvol <<
		"\n\t maxvol_percent: " << maxvol_percent <<
	std::endl;
} // end print settings


} // ns mcl

#endif
