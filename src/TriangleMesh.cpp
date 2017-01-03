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

#include "MCL/TriangleMesh.hpp"
#include <chrono>
#include "TriMesh_algo.h"

using namespace mcl;


void TriangleMesh::bounds( Vec3d &bmin, Vec3d &bmax ){
	if( !aabb.valid ){
		for( int f=0; f<faces.size(); ++f ){
			aabb += vertices[ faces[f][0] ];
			aabb += vertices[ faces[f][1] ];
			aabb += vertices[ faces[f][2] ];
		}
	}
	bmin = aabb.min; bmax = aabb.max;
}


void TriangleMesh::need_normals( bool recompute ){

	if( vertices.size() == normals.size() && !recompute ){ return; }

	if( normals.size() != vertices.size() ){ normals.resize( vertices.size() ); }
	const int nv = normals.size();

#pragma omp parallel for
	for( int i = 0; i < nv; ++i ){
		normals[i][0] = 0.f; normals[i][1] = 0.f; normals[i][2] = 0.f;
	}

	int nf = faces.size();
#pragma omp parallel for
	for( int i = 0; i < nf; ++i ){
		const Vec3d &p0 = vertices[faces[i][0]];
		const Vec3d &p1 = vertices[faces[i][1]];
		const Vec3d &p2 = vertices[faces[i][2]];
		Vec3d a = p0-p1, b = p1-p2, c = p2-p0;
		float l2a = a.squaredNorm(), l2b = b.squaredNorm(), l2c = c.squaredNorm();
		if (!l2a || !l2b || !l2c)
			continue;
		Vec3d facenormal = a.cross( b );
		normals[faces[i][0]] += facenormal * (1.0f / (l2a * l2c));
		normals[faces[i][1]] += facenormal * (1.0f / (l2b * l2a));
		normals[faces[i][2]] += facenormal * (1.0f / (l2c * l2b));
	}

#pragma omp parallel for
	for (int i = 0; i < nv; i++){ normals[i].normalize(); }

} // end compute normals



// Transform the mesh by the given matrix
void TriangleMesh::apply_xform( const trimesh::xform &xf ){

	int nv = vertices.size();
#pragma omp parallel for
	for (int i = 0; i < nv; i++){ vertices[i] = xf * vertices[i]; }

	need_normals(true);

	aabb.valid = false;
	for( int f=0; f<faces.size(); ++f ){
		aabb += vertices[ faces[f][0] ];
		aabb += vertices[ faces[f][1] ];
		aabb += vertices[ faces[f][2] ];
	}
}


void TriangleMesh::make_tri_refs(){

	tri_refs.clear();
	need_normals();

	for( int i=0; i<faces.size(); ++i ){
		Vec3i f = faces[i];
		std::shared_ptr<BaseObject> tri(
			new TriangleRef( &vertices[f[0]], &vertices[f[1]], &vertices[f[2]], &normals[f[0]], &normals[f[1]], &normals[f[2]] )
		);
		tri->app.material = app.material;
		tri_refs.push_back( tri );
	} // end loop faces

} // end make triangle references


bool TriangleMesh::load( std::string filename ){

	// Clear old data
	vertices.clear();
	normals.clear();
	faces.clear();
	tri_refs.clear();

	// Load the file with trimesh2, it has a bunch of nice i/o
	trimesh::TriMesh *newmesh = trimesh::TriMesh::read( filename.c_str() );
	if( newmesh == NULL ){ return false; }
	trimesh::remove_unused_vertices( newmesh );

	// Copy over data
	vertices.resize( newmesh->vertices.size() );
	colors.resize( newmesh->colors.size() );
	texcoords.resize( newmesh->texcoords.size() );
	faces.resize( newmesh->faces.size() );

#pragma omp parallel for
	for( int i=0; i<vertices.size(); ++i ){ vertices[i] = Vec3d( newmesh->vertices[i][0], newmesh->vertices[i][1], newmesh->vertices[i][2] ); }
#pragma omp parallel for
	for( int i=0; i<colors.size(); ++i ){ colors[i] = Vec3d( newmesh->colors[i][0], newmesh->colors[i][1], newmesh->colors[i][2] ); }
#pragma omp parallel for
	for( int i=0; i<texcoords.size(); ++i ){ texcoords[i] = Vec2d( newmesh->texcoords[i][0], newmesh->texcoords[i][1] ); }
#pragma omp parallel for
	for( int i=0; i<faces.size(); ++i ){ faces[i] = Vec3i( newmesh->faces[i][0], newmesh->faces[i][1], newmesh->faces[i][2] ); }

	delete newmesh;

	// Remake the triangle refs and aabb
	make_tri_refs();
	aabb.valid = false;
	for( int f=0; f<faces.size(); ++f ){
		aabb += vertices[ faces[f][0] ];
		aabb += vertices[ faces[f][1] ];
		aabb += vertices[ faces[f][2] ];
	}

	need_normals(true);
	update_appdata();

	return true;

} // end load file

void TriangleMesh::update_appdata(){

	// Update app data
	this->app.num_vertices = vertices.size();
	this->app.num_normals = normals.size();
	this->app.num_faces = faces.size();
	this->app.num_colors = colors.size();
	this->app.num_texcoords = texcoords.size();

	this->app.vertices = &vertices[0][0];
	this->app.normals = &normals[0][0];
	this->app.faces = &faces[0][0];
	this->app.colors = &colors[0][0];
	this->app.texcoords = &texcoords[0][0];

}

static std::string get_timestamp_meshname(){
	std::string MY_DATE_FORMAT = "h%Hm%M";
	const int MY_DATE_SIZE = 20;
	static char name[MY_DATE_SIZE];
	time_t now = time(0);
	strftime(name, sizeof(name), MY_DATE_FORMAT.c_str(), localtime(&now));
	return std::string(name);
}

static bool mesh_exists( const std::string& name ){
	std::ifstream f(name.c_str());
	return f.good();
}

void TriangleMesh::save( std::string filename ){
	std::cout << "\n\n**TODO: TriangleMesh::save\n\n" << std::endl;
}

std::string TriangleMesh::get_xml( int mode ){

	std::string timestamp = get_timestamp_meshname();
	std::string filename = "";

	// Get mesh number by using mesh_exists
	// Not an elegant solution but fine for now.
	int mesh_num = 0;
	for( int i=0; i<10000; ++i ){
		std::stringstream fn; fn << timestamp << "-" << mesh_num << ".obj";
		if( !mesh_exists( fn.str() ) ){ filename = fn.str(); break; }
		mesh_num++;
	}

	if( !filename.size() ){
		std::cerr << "TriangleMesh::Error: Problem exporting file for scene save" << std::endl;
		return "";
	}


	// Save the mesh
	std::stringstream objfile;
	objfile << MCLSCENE_BUILD_DIR<< "/" << filename;
	save( objfile.str() );

	// mclscene
	if( mode == 0 ){
		std::stringstream xml;
		xml << "\t<Object type=\"TriMesh\" >\n";
		xml << "\t\t<File value=\"" << objfile.str() << "\" />\n";
		xml << "\t</Object>";
		return xml.str();
	}

	return "";
}


