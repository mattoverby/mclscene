// Copyright (c) 2016 University of Minnesota
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

#ifndef MCLSCENE_TRIANGLEMESH_H
#define MCLSCENE_TRIANGLEMESH_H 1

#include "Object.hpp"
#include "AABB.hpp"

namespace mcl {


//
//	Triangle (reference)
//	Contains pointers to vertices and normals of a TriangleMesh
//
class TriangleRef : public BaseObject {
public:
	TriangleRef( Vec3f *p0_, Vec3f *p1_, Vec3f *p2_,
		Vec3f *n0_, Vec3f *n1_, Vec3f *n2_ ) :
		p0(p0_), p1(p1_), p2(p2_), n0(n0_), n1(n1_), n2(n2_) {}

	Vec3f *p0, *p1, *p2, *n0, *n1, *n2;

	void get_bounds( Vec3f &bmin, Vec3f &bmax ){
		AABB aabb; aabb += *p0; aabb += *p1; aabb += *p2;
		bmin = aabb.min; bmax = aabb.max;
	}

	bool ray_intersect( const raycast::Ray *ray, raycast::Payload *payload ) const {
		bool hit = raycast::ray_triangle( ray, *p0, *p1, *p2, *n0, *n1, *n2, payload );
		if( hit ){ payload->material = this->app.material; }
		return hit;
	}

	Vec3f projection( const Vec3f &point ) const {
		return projection::point_triangle( point, *p0, *p1, *p2 );
	}
};


//
//	Just a convenient wrapper to plug into the system
//
class TriangleMesh : public BaseObject {
public:
	std::vector< Vec3f > vertices; // all vertices in the mesh
	std::vector< Vec3f > normals; // zero length for all non-surface normals
	std::vector< Vec3i > faces; // surface triangles
	std::vector< Vec2f > texcoords; // per vertex uv coords
	std::vector< Vec2i > edges;

	// Returns true on success
	bool load( std::string filename );

	// Saves the triangle mesh to a file (obj)
	void save( std::string filename );

	std::string get_xml( int mode=0 );

	void apply_xform( const trimesh::xform &xf );

	void get_bounds( Vec3f &bmin, Vec3f &bmax );

	void update();

	void need_normals( bool recompute=false );

	// Creates edges for rendering wireframe
	void need_edges( bool recompute=false );

	void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){
		if( tri_refs.size() != faces.size() ){ make_tri_refs(); }
		prims.insert( prims.end(), tri_refs.begin(), tri_refs.end() );
	}

	// Normal of face f
	Vec3f trinorm( unsigned int f );

	// Clear all mesh data
	void clear();

private:
	AABB aabb;

	// Triangle refs are used for BVH hook-in.
	void make_tri_refs();
	std::vector< std::shared_ptr<BaseObject> > tri_refs;

	bool load_obj( std::string filename );
};


} // end namespace mcl

#endif
