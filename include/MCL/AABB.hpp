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

#ifndef MCLSCENE_AABB_H
#define MCLSCENE_AABB_H 1

#include "Vec.h"
#include <memory>
#include <cassert>

namespace mcl {

class AABB { // axis aligned bounding box
public:
	AABB() : valid(false) {}
	AABB( trimesh::vec min_, trimesh::vec max_ ) : min(min_), max(max_), valid(false) {}
	trimesh::vec min, max;

	// Fills the vector with points that make the edge lines.
	// Used for visual debugging in OpenGL.
	void get_edges( std::vector<trimesh::vec> &edges ){
		using namespace trimesh;

		// Bottom quad
		point a = min;
		point b( max[0], min[1], min[2] );
		point c( max[0], min[1], max[2] );
		point d( min[0], min[1], max[2] );
		// Top quad
		point e( min[0], max[1], min[2] );
		point f( max[0], max[1], min[2] );
		point g = max;
		point h( min[0], max[1], max[2] );

		// make edges
		// bottom
		edges.push_back( a ); edges.push_back( b );
		edges.push_back( a ); edges.push_back( d );
		edges.push_back( c ); edges.push_back( b );
		edges.push_back( c ); edges.push_back( d );
		// top
		edges.push_back( e ); edges.push_back( f );
		edges.push_back( e ); edges.push_back( h );
		edges.push_back( g ); edges.push_back( f );
		edges.push_back( g ); edges.push_back( h );
		// columns
		edges.push_back( d ); edges.push_back( h );
		edges.push_back( min ); edges.push_back( e );
		edges.push_back( b ); edges.push_back( f );
		edges.push_back( c ); edges.push_back( max );

	} // end make edges

	trimesh::vec center(){ return (min+max)*0.5f; }

	AABB& operator+=(const AABB& aabb){
		if( valid ){ min.min( aabb.min ); max.max( aabb.max ); }
		else{ min = aabb.min; max = aabb.max; }
		valid = true;
	}

	AABB& operator+=(const trimesh::vec& p){
		if( valid ){ min.min(p); max.max(p); }
		else{ min = p; max = p; }
		valid = true;
	}

	bool valid;
};


} // end namespace mcl



#endif
