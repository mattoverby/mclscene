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

#ifndef MCLSCENE_TETMESH_H
#define MCLSCENE_TETMESH_H 1

#include "MCL/Object.hpp"
#include "MCL/VertexSort.hpp"

namespace mcl {

//
//	Tetrahedral Mesh
//
//	TODO Surface mesh type with collapsed vertices
//	instead of creating normals for every vertex.
//
class TetMesh : public BaseObject {
public:
	struct tet {
		tet(){}
		tet( int a, int b, int c, int d ){ v[0]=a; v[1]=b; v[2]=c; v[3]=d; }
		int v[4];
	};

	std::vector< trimesh::point > vertices; // all vertices in the tet
	std::vector< tet > tets; // all elements
	std::vector< trimesh::vec > normals; // zero length for all non-surface normals
	std::vector< trimesh::TriMesh::Face > faces; // surface triangles

	TetMesh( std::string mat="" ) : temp_trimesh(NULL), material(mat) {}

	std::string get_type() const { return "tetmesh"; }

	// Filename is the first part of a tetmesh which must contain an .ele and .node file.
	// Returns true on success
	bool load( std::string filename );

	//bool save( std::string filename ); // TODO

	// Compute the normals for surface vertices. The inner normals are length zero.
	void need_normals( bool recompute=true );

	// Transform the mesh by the given matrix
	void apply_xform( const trimesh::xform &xf );

	void init( const std::vector< Param > &params );

	// Creates a new trimesh object from ALL vertices and stuff
	const std::shared_ptr<trimesh::TriMesh> get_TriMesh();

	std::string get_material() const { return material; }

	void get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ){}//TODO

private:
	std::string material;

	// Created on first call to as_TriMesh()
	std::shared_ptr<trimesh::TriMesh> temp_trimesh;

	bool load_node( std::string filename );

	bool load_ele( std::string filename );

	// Computes a surface mesh, called by load
	bool need_surface();

}; // end class TetMesh

} // end namespace mcl

#endif
