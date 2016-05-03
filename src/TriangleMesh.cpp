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

#include "MCL/TriangleMesh.hpp"

using namespace mcl;


// Make a BVH tree from a triangle mesh
void MeshBVH::make_tree( const std::vector<trimesh::TriMesh::Face> &faces,
	const std::vector<trimesh::point> &vertices, int split_axis, int max_depth ) {
	using namespace trimesh;

	split_axis = (split_axis+1)%3;
	m_split = split_axis;
	max_depth--;

	// Create the aabb
	for( int i=0; i<faces.size(); ++i ){
		*aabb += vertices[ faces[i][0] ];
		*aabb += vertices[ faces[i][1] ];
		*aabb += vertices[ faces[i][2] ];
	}
	point center = aabb->center();

	// If num faces == 1, we're done
	if( faces.size()==0 ){ return; }
	else if( faces.size()==1 || max_depth <= 0 ){
		m_faces = faces;
		return;
	}
	
	// Split faces
	std::vector<TriMesh::Face> left_faces, right_faces;
	for( int i=0; i<faces.size(); ++i ){
		double fc = helper::face_center( faces[i], vertices )[ split_axis ];
		if( fc <= center[ split_axis ] ){ left_faces.push_back( faces[i] ); }
		else if( fc > center[ split_axis ] ){ right_faces.push_back( faces[i] ); }
	}

	// Check to make sure things got sorted. Sometimes small meshes fail.
	if( left_faces.size()==0 ){ left_faces.push_back( right_faces.back() ); right_faces.pop_back(); }
	if( right_faces.size()==0 ){ right_faces.push_back( left_faces.back() ); left_faces.pop_back(); }

	// Create the children
	left_child = std::shared_ptr<BVHNode>( new MeshBVH() );
	right_child = std::shared_ptr<BVHNode>( new MeshBVH() );
	std::static_pointer_cast<MeshBVH>(left_child)->make_tree( left_faces, vertices, split_axis, max_depth );
	std::static_pointer_cast<MeshBVH>(right_child)->make_tree( right_faces, vertices, split_axis, max_depth );

}

