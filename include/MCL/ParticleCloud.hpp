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

#ifndef MCLSCENE_PARTICLECLOUD_H
#define MCLSCENE_PARTICLECLOUD_H 1

#include "Object.hpp"

namespace mcl {

//
//	A particle cloud is just a collection of vertices
//	stored in a trimesh sans faces/normals.
//	It used for fluids, gasses, etc...
//
//	Eventually I will add support for ray tracing on its convex hull
//

class ParticleCloud : public BaseObject {
private: std::shared_ptr<trimesh::TriMesh> data;
public:
	ParticleCloud( std::string mat="" ) : data(new trimesh::TriMesh), vertices(data->vertices), aabb(new AABB) {}

	// Mesh data
	std::vector<trimesh::point> &vertices;

	// General getters
	std::string get_type() const { return "particlecloud"; }
	const std::shared_ptr<trimesh::TriMesh> get_TriMesh(){ return data; }
	std::string get_material() const { return material; }
	std::string get_xml( std::string obj_name, int mode=0 );
	void bounds( trimesh::vec &bmin, trimesh::vec &bmax );

	// Tells the cloud that some particle has moved, and the
	// bounding box/convex hull needs to be recomputed.
	void update();

	// A particle cloud can be initialized from a file or adding vertices manually.
	// Currently supported file types are
	//	.ply	(triangle mesh)
	//	.node	(tet mesh w/o elements)
	// It will strip out any face/normal information
	// Returns true on success.
	bool load( std::string file );

private:
	std::string material;
	std::shared_ptr<AABB> aabb;
};


} // end namespace mcl

#endif
