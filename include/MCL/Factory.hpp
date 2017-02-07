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
// By Matt Overby (http://www.mattoverby.net)


#ifndef MCLSCENE_FACTORY_H
#define MCLSCENE_FACTORY_H 1

#include "MCL/TetMesh.hpp"
#include "MCL/TriangleMesh.hpp"
#include "MCL/SceneManager.hpp"

namespace mcl {

//	
//	Factory functions for creating objects/materials/lights/cameras
//	TODO Remove the DefaultBuilders and replace with these functions
//
//	If a pointer to SceneManager is passed, the obj/mat/light/cam is added.
//
//	Most of these functions come from Trimesh2 by Szymon Rusinkiewicz (GNU GPL, version 2)
//
namespace factory {

	// A basic sphere
	std::shared_ptr<TriangleMesh> make_sphere( Vec3f center, float radius, int tess, SceneManager *scene=nullptr );

	// A non-symmetric cube
	std::shared_ptr<TriangleMesh> make_cube( int tess, SceneManager *scene=nullptr );

	// A beam is one or more connected cubes (chunks)
	std::shared_ptr<TriangleMesh> make_beam( int chunks, int tess, SceneManager *scene=nullptr );

	// plane

	// cylinder

	// torus

	// Helpers
	static inline void mkpoint(std::shared_ptr<TriangleMesh> mesh, float x, float y, float z);
	static inline void mkface(std::shared_ptr<TriangleMesh> mesh, int v1, int v2, int v3);
	static inline void mkquad(std::shared_ptr<TriangleMesh> mesh, int ll, int lr, int ul, int ur);
	static inline void mkquad_sym(std::shared_ptr<TriangleMesh> mesh, int ll, int lr, int ul, int ur, int cent);
	static inline void remove_vertices(TriangleMesh *mesh, const std::vector<bool> &toremove);
	static inline void remove_unused_vertices(TriangleMesh *mesh);
	static inline void remove_faces(TriangleMesh *mesh, const std::vector<bool> &toremove);
	static inline void remap_verts(TriangleMesh *mesh, const std::vector<int> &remap_table);
	static inline void reorder_verts(TriangleMesh *mesh);
	template <typename T> static inline T sqr( T val ){ return val*val; }
	template <typename T> static inline std::string t_to_str( T val ); // scalar to string

}; // end namespace factory

//
//	Implementation
//

std::shared_ptr<TriangleMesh> factory::make_sphere( Vec3f center, float radius, int tess, SceneManager *scene ){

	std::shared_ptr<TriangleMesh> mesh( new TriangleMesh() );

	mesh->vertices.reserve(2+tess*(tess-1));
	mkpoint(mesh, 0, 0, -1);
	for (int j = 1; j < tess; j++) {
		float th = M_PIf * j / tess;
		float z = -cos(th);
		float r = sin(th);
		for (int i = 0; i < tess; i++) {
			float ph = M_TWOPIf * i / tess;
			mkpoint(mesh, r*cos(ph), r*sin(ph), z);
		}
	}
	mkpoint(mesh, 0, 0, 1);

	mesh->faces.reserve(2*tess*tess - 2*tess);

	for (int i = 0; i < tess; i++)
		mkface(mesh, 0, ((i+1)%tess)+1, i+1);

	for (int j = 0; j < tess-2; j++) {
		int base = 1 + j * tess;
		for (int i = 0; i < tess; i++) {
			int i1 = (i+1)%tess;
			mkquad(mesh, base + i, base + i1,
				base+tess+i, base+tess+i1);
		}
	}

	int base = 1 + (tess-2)*tess;
	for (int i = 0; i < tess; i++){ mkface(mesh, base+i, base+((i+1)%tess), base+tess); }

	// Now scale it by the radius
	trimesh::xform s_xf = trimesh::xform::scale(radius,radius,radius);
	mesh->apply_xform( s_xf );

	// Translate so center is correct
	trimesh::xform t_xf = trimesh::xform::trans(center[0],center[1],center[2]);
	mesh->apply_xform( t_xf );

	mesh->update();

	// Add to SceneManager
	if( scene != nullptr ){
		std::vector< Param > params;
		params.push_back( Param( "center", to_str(center) ) );
		params.push_back( Param( "radius", t_to_str(radius) ) );
		params.push_back( Param( "tess", t_to_str(tess) ) );
		scene->object_params.push_back( params );
		scene->objects.push_back( mesh );
	}

	return mesh;

} // end make sphere


std::shared_ptr<TriangleMesh> factory::make_beam( int chunks, int tess, SceneManager *scene ){

	std::shared_ptr<TriangleMesh> mesh( new TriangleMesh() );

	for( int b=0; b<chunks; ++b ){

		std::shared_ptr<TriangleMesh> box = make_cube( tess );

		// Now translate box1 and box2
		trimesh::xform xf = trimesh::xform::trans( b*2.f, 0, 0 );
		box->apply_xform( xf );
		box->need_normals();

		// Remove faces on the -x and +x
		std::vector<bool> toremove( box->faces.size(), false );
		for( int f=0; f<box->faces.size(); ++f ){
			if( b > 0 ){ // remove -x
				if( box->trinorm(f).dot( Vec3f(-1.f,0.f,0.f) ) > 0.f ){ toremove[f]=true; }
			}
			if( b < chunks-1 ){ // remove +x
				if( box->trinorm(f).dot( Vec3f(1.f,0.f,0.f) ) > 0.f ){ toremove[f]=true; }
			}
		}

		remove_faces( box.get(), toremove );
		remove_unused_vertices( box.get() );

		int prev_verts = mesh->vertices.size();

		for( int i=0; i<box->vertices.size(); ++i ){ mesh->vertices.push_back( box->vertices[i] ); }
		for( int i=0; i<box->faces.size(); ++i ){
			for( int j=0; j<3; ++j ){ box->faces[i][j] += prev_verts; }
			mesh->faces.push_back( box->faces[i] );
		}

	}

	mesh->update();

	// Add to SceneManager
	if( scene != nullptr ){
		mesh->app.flat_shading = true;
		std::vector< Param > params;
		params.push_back( Param( "radius", t_to_str(chunks) ) );
		params.push_back( Param( "tess", t_to_str(tess) ) );
		scene->object_params.push_back( params );
		scene->objects.push_back( mesh );
	}

	return mesh;

} // end make beam



std::shared_ptr<TriangleMesh> factory::make_cube( int tess, SceneManager *scene ){

	std::shared_ptr<TriangleMesh> mesh( new TriangleMesh() );

	if (tess < 1)
		tess = 1;

	mesh->vertices.reserve(6*sqr(tess)+2);
	for (int j = 0; j < tess+1; j++) {
		float y = 1.0f - 2.0f * j / tess;
		for (int i = 0; i < tess+1; i++) {
			float x = 1.0f - 2.0f * i / tess;
			mkpoint(mesh, x, y, -1);
		}
	}
	for (int j = 1; j < tess; j++) {
		float z = -1.0f + 2.0f * j / tess;
		for (int i = 0; i < tess; i++) {
			float x = -1.0f + 2.0f * i / tess;
			mkpoint(mesh, x, -1, z);
		}
		for (int i = 0; i < tess; i++) {
			float y = -1.0f + 2.0f * i / tess;
			mkpoint(mesh, 1, y, z);
		}
		for (int i = 0; i < tess; i++) {
			float x = 1.0f - 2.0f * i / tess;
			mkpoint(mesh, x, 1, z);
		}
		for (int i = 0; i < tess; i++) {
			float y = 1.0f - 2.0f * i / tess;
			mkpoint(mesh, -1, y, z);
		}
	}
	for (int j = 0; j < tess+1; j++) {
		float y = -1.0f + 2.0f * j / tess;
		for (int i = 0; i < tess+1; i++) {
			float x = -1.0f + 2.0f * i / tess;
			mkpoint(mesh, x, y, 1);
		}
	}

	mesh->faces.reserve(12*sqr(tess));
	for (int j = 0; j < tess; j++) {
		for (int i = 0; i < tess; i++) {
			int ind = i + j * (tess+1);
			mkquad(mesh, ind, ind+tess+1, ind+1, ind+tess+2);
		}
	}

	int topstart = sqr(tess+1) + 4*tess*(tess-1);
	for (int j = 0; j < tess; j++) {
		int next = sqr(tess+1) + 4*tess*(j-1);
		for (int i = 0; i < tess; i++) {
			int ll = next++;
			int lr = ll + 1;
			int ul = ll + 4*tess;
			int ur = ul + 1;
			if (j == 0) {
				ll = sqr(tess+1)-1 - i;
				lr = ll - 1;
			}
			mkquad(mesh, ll, lr, ul, ur);
		}
		for (int i = 0; i < tess; i++) {
			int ll = next++;
			int lr = ll + 1;
			int ul = ll + 4*tess;
			int ur = ul + 1;
			if (j == 0) {
				ll = tess*(tess+1) - i*(tess+1);
				lr = ll - (tess+1);
			}
			if (j == tess-1) {
				ul = topstart + tess + i*(tess+1);
				ur = ul + (tess+1);
			}
			mkquad(mesh, ll, lr, ul, ur);
		}
		for (int i = 0; i < tess; i++) {
			int ll = next++;
			int lr = ll + 1;
			int ul = ll + 4*tess;
			int ur = ul + 1;
			if (j == 0) {
				ll = i;
				lr = i + 1;
			}
			if (j == tess-1) {
				ul = topstart + sqr(tess+1)-1 - i;
				ur = ul - 1;
			}
			mkquad(mesh, ll, lr, ul, ur);
		}
		for (int i = 0; i < tess; i++) {
			int ll = next++;
			int lr = ll + 1;
			int ul = ll + 4*tess;
			int ur = ul + 1;
			if (j == 0) {
				ll = tess + i*(tess+1);
				lr = ll + (tess+1);
			}
			if (j == tess-1) {
				ul = topstart + tess*(tess+1) - i*(tess+1);
				ur = ul - (tess+1);
			}
			if (i == tess-1) {
				if (j != 0)
					lr -= 4*tess;
				if (j != tess-1)
					ur -= 4*tess;
			}
			mkquad(mesh, ll, lr, ul, ur);
		}
	}
	for (int j = 0; j < tess; j++) {
		for (int i = 0; i < tess; i++) {
			int ind = topstart + i + j * (tess+1);
			mkquad(mesh, ind, ind+1, ind+tess+1, ind+tess+2);
		}
	}

	mesh->update();

	if( scene != nullptr ){
		mesh->app.flat_shading = true;
		std::vector< Param > params;
		params.push_back( Param( "tess", t_to_str(tess) ) );
		params.push_back( Param( "flat_shading", "1" ) );
		scene->object_params.push_back( params );
		scene->objects.push_back( mesh );
	}

	return mesh;
}



//
//	Helper functions
//

static inline void factory::mkpoint(std::shared_ptr<TriangleMesh> mesh, float x, float y, float z){
	mesh->vertices.push_back( Vec3f(x,y,z) );
}

static inline void factory::mkface(std::shared_ptr<TriangleMesh> mesh, int v1, int v2, int v3){
	mesh->faces.push_back( Vec3i(v1, v2, v3) );
}

// non symmetric quad
//	*---*
//	|  /|
//	| / |
//	|/  |
//	*---*
static inline void factory::mkquad(std::shared_ptr<TriangleMesh> mesh, int ll, int lr, int ul, int ur){
	mkface(mesh, ll, lr, ur);
	mkface(mesh, ll, ur, ul);
}

// Symmetric quad
//	*---*
//	|\ /|
//	| * |
//	|/ \|
//	*---*
static inline void factory::mkquad_sym(std::shared_ptr<TriangleMesh> mesh, int ll, int lr, int ul, int ur, int cent){
	// Counter clockwise
	mkface(mesh, ll, lr, cent);
	mkface(mesh, lr, ur, cent);
	mkface(mesh, cent, ur, ul);
	mkface(mesh, ll, cent, ul);
}


// Remove the indicated vertices from the TriMesh.
static inline void factory::remove_vertices(TriangleMesh *mesh, const std::vector<bool> &toremove){

	int nv = mesh->vertices.size();

	// Build a table that tells how the vertices will be remapped
	if (!nv)
		return;

	std::vector<int> remap_table(nv);
	int next = 0;
	for (int i = 0; i < nv; i++) {
		if (toremove[i])
			remap_table[i] = -1;
		else
			remap_table[i] = next++;
	}

	// Nothing to delete?
	if (next == nv) {
		return;
	}

	remap_verts(mesh, remap_table);

}

static inline void factory::remove_unused_vertices(TriangleMesh *mesh){

	int nv = mesh->vertices.size();
	if (!nv) return;

	int nf = mesh->faces.size();
	std::vector<bool> unused(nv, true);
	for (int i = 0; i < nf; i++) {
		unused[mesh->faces[i][0]] = false;
		unused[mesh->faces[i][1]] = false;
		unused[mesh->faces[i][2]] = false;
	}

	remove_vertices(mesh, unused);
}

static inline void factory::remove_faces(TriangleMesh *mesh, const std::vector<bool> &toremove){

	int numfaces = mesh->faces.size();
	if (!numfaces) return;

	int next = 0;
	for (int i = 0; i < numfaces; i++) {
		if (toremove[i])
			continue;
		mesh->faces[next++] = mesh->faces[i];
	}
	if (next == numfaces) {
		return;
	}

	mesh->faces.erase(mesh->faces.begin() + next, mesh->faces.end());
}


template <typename T> static inline std::string factory::t_to_str( T val ){
	std::stringstream ss; ss << val; return ss.str();
}



// Remap vertices according to the given table
//
// Faces are renumbered to reflect the new numbering of vertices, and any
// faces that included a vertex that went away will also be removed.
//
// Any per-vertex properties are renumbered along with the vertices.
static inline void factory::remap_verts(TriangleMesh *mesh, const std::vector<int> &remap_table){

	if (remap_table.size() != mesh->vertices.size()) {
		printf("remap_verts called with wrong table size!\n");
		return;
	}

	// Check what we're doing
	bool removing_verts = false, any_left = false;
	int last = -1;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++) {
		if (remap_table[i] < 0) {
			removing_verts = true;
		} else {
			any_left = true;
			if (remap_table[i] > last)
				last = remap_table[i];
		}
	}

	if (!any_left) {
		mesh->clear();
		return;
	}

	// Figure out what we have sitting around, so we can remap/recompute
	bool have_faces = !mesh->faces.empty();
	bool have_normals = !mesh->normals.empty();

	// Remap the vertices and per-vertex properties
	TriangleMesh *oldmesh = new TriangleMesh;
	*oldmesh = *mesh;

#define REMAP(property) mesh->property[remap_table[i]] = oldmesh->property[i]

	for (int i = 0; i < nv; i++) {
		if (remap_table[i] < 0 || remap_table[i] == i)
			continue;
		REMAP(vertices);
		if (have_normals) REMAP(normals);
	}

#define ERASE(property) mesh->property.erase(mesh->property.begin()+last+1, \
					     mesh->property.end())
	ERASE(vertices);
	if (have_normals) ERASE(normals);

	// Renumber faces
	int nf = mesh->faces.size(), nextface = 0;
	for (int i = 0; i < nf; i++) {
		int n0 = (mesh->faces[nextface][0] = remap_table[oldmesh->faces[i][0]]);
		int n1 = (mesh->faces[nextface][1] = remap_table[oldmesh->faces[i][1]]);
		int n2 = (mesh->faces[nextface][2] = remap_table[oldmesh->faces[i][2]]);
		if ((n0 >= 0) && (n1 >= 0) && (n2 >= 0))
			nextface++;
	}
	mesh->faces.erase(mesh->faces.begin() + nextface, mesh->faces.end());

	delete oldmesh;

}


// Reorder vertices in a mesh according to the order in which
// they are referenced by the grid, tstrips, or faces.
static inline void factory::reorder_verts(TriangleMesh *mesh){

	int nv = mesh->vertices.size();
	std::vector<int> remap(nv, -1);
	int next = 0;

	for (size_t i = 0; i < mesh->faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			int v = mesh->faces[i][j];
			if (remap[v] == -1)
				remap[v] = next++;
		}
	}

	if (next != nv) {
		// Unreferenced vertices...  Just stick them at the end.
		for (int i = 0; i < nv; i++)
			if (remap[i] == -1)
				remap[i] = next++;
	}

	remap_verts(mesh, remap);
}


} // end namespace mcl

#endif

