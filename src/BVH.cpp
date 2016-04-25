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

#include "MCL/BVH.hpp"

using namespace mcl;

void AABB::get_edges( std::vector<trimesh::vec> &edges ){
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


BVHNode::BVHNode( const std::vector<trimesh::TriMesh::Face> &faces, const std::vector<trimesh::point> &vertices, int split_axis, int max_depth ) {
	using namespace trimesh;

	left_child = NULL;
	right_child = NULL;
	split_axis = (split_axis+1)%3;
	m_split = split_axis;
	max_depth--;

	// Create the aabb
	aabb = std::shared_ptr<AABB>( new AABB );
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
	else if( faces.size()==2 ){
		std::vector<TriMesh::Face> left_faces(1,faces[0]), right_faces(1,faces[1]);
		left_child = std::shared_ptr<BVHNode>( new BVHNode( left_faces, vertices, split_axis, max_depth ) );
		right_child = std::shared_ptr<BVHNode>( new BVHNode( right_faces, vertices, split_axis, max_depth ) );
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
	left_child = std::shared_ptr<BVHNode>( new BVHNode( left_faces, vertices, split_axis, max_depth ) );
	right_child = std::shared_ptr<BVHNode>( new BVHNode( right_faces, vertices, split_axis, max_depth ) );
}


BVHNode::BVHNode( std::vector< std::shared_ptr<BVHNode> > bvhnodes, int split_axis ){
	using namespace trimesh;

	left_child = NULL;
	right_child = NULL;
	split_axis = (split_axis+1)%3;
	m_split = split_axis;


	// Create the aabb
	aabb = std::shared_ptr<AABB>( new AABB );
	for( int i=0; i<bvhnodes.size(); ++i ){
		*aabb += *bvhnodes[i]->bounds();
	}
	point center = aabb->center();

	// Special cases for numbers
	if( bvhnodes.size()==0 ){ return; }
	else if( bvhnodes.size()==1 ){
		left_child = bvhnodes[0];
		return;
	}
	else if( bvhnodes.size()==2 ){
		left_child = bvhnodes[0];
		right_child = bvhnodes[1];
		return;
	}

	// Otherwise, sort and store
	std::vector< std::shared_ptr<BVHNode> > left_nodes, right_nodes;
	for( int i=0; i<bvhnodes.size(); ++i ){
		double fc = bvhnodes[i]->bounds()->center()[ split_axis ];
		if( fc <= center[ split_axis ] ){ left_nodes.push_back( bvhnodes[i] ); }
		else if( fc > center[ split_axis ] ){ right_nodes.push_back( bvhnodes[i] ); }
	}

	// Check to make sure things got sorted. Sometimes small meshes fail.
	if( left_nodes.size()==0 ){ left_nodes.push_back( right_nodes.back() ); right_nodes.pop_back(); }
	if( right_nodes.size()==0 ){ right_nodes.push_back( left_nodes.back() ); left_nodes.pop_back(); }

	// Create the children
	left_child = std::shared_ptr<BVHNode>( new BVHNode( left_nodes, split_axis ) );
	right_child = std::shared_ptr<BVHNode>( new BVHNode( right_nodes, split_axis ) );
}


void BVHNode::get_edges( std::vector<trimesh::vec> &edges ){
	aabb->get_edges( edges );
	if( left_child != NULL ){ left_child->get_edges( edges ); }
	if( right_child != NULL ){ right_child->get_edges( edges ); }
}

