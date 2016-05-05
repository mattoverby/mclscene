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

/*
void TriangleMesh::make_bvh( bool recompute ){

	using namespace trimesh;

	if( tri_refs.size()>0 && !recompute ){ return; }
	tri_refs.clear();

	// Create the triangle reference objects
	tris->need_faces();
	tris->need_normals();

	for( int i=0; i<faces.size(); ++i ){
		TriMesh::Face f = faces[i];
		std::shared_ptr<BaseObject> tri(
			new TriangleRef( &vertices[f[0]], &vertices[f[1]], &vertices[f[2]], &normals[f[0]], &normals[f[1]], &normals[f[2]], material )
		);
		tri_refs.push_back( tri );
	} // end loop faces

	// Now create a BVH with the triangle refs
	bvh->make_tree( tri_refs );

} // end make triangle refs
*/

void TriangleMesh::get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ){
	if( !aabb->valid ){
		for( int f=0; f<faces.size(); ++f ){
			(*aabb) += vertices[ faces[f][0] ];
			(*aabb) += vertices[ faces[f][1] ];
			(*aabb) += vertices[ faces[f][2] ];
		}
	}
	bmin = aabb->min; bmax = aabb->max;
//	make_bvh();
}


void TriangleMesh::make_tri_refs(){

	using namespace trimesh;

	tri_refs.clear();

	// Create the triangle reference objects
	tris->need_faces();
	tris->need_normals();

	for( int i=0; i<faces.size(); ++i ){
		TriMesh::Face f = faces[i];
		std::shared_ptr<BaseObject> tri(
			new TriangleRef( &vertices[f[0]], &vertices[f[1]], &vertices[f[2]], &normals[f[0]], &normals[f[1]], &normals[f[2]], material )
		);
		tri_refs.push_back( tri );
	} // end loop faces

} // end make triangle references


