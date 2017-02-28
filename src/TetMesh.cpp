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

#include "MCL/TetMesh.hpp"
#include "MCL/HashKeys.hpp"
#include <chrono>
#include <map>
#ifdef MCLSCENE_ENABLE_TETGEN
	#include "tetgen.h"
#endif

using namespace mcl;

namespace tetmesh_helper {
	static std::string get_ext( std::string fname ){
		size_t pos = fname.find_last_of('.');
		return (std::string::npos == pos) ? "" : fname.substr(pos+1,fname.size());
	}
	static std::string to_lower( std::string s ){ std::transform( s.begin(), s.end(), s.begin(), ::tolower ); return s; }

	static char **make_argv( const std::vector<std::string> &s ){
		char **args;
		args = new char *[s.size()];
		for( int i = 0; i < s.size(); ++i ){
			args[i] = new char[s[i].length() + 1];
			strcpy( args[i], s[i].c_str());
		} 
		return args;
	}
} // end helper functions


bool TetMesh::load( std::string filename ){

	// Clear old data
	vertices.clear();
	tets.clear();
	normals.clear();
	faces.clear();

	// Get the extension
	std::string ext = tetmesh_helper::to_lower( tetmesh_helper::get_ext(filename) );

	if( ext=="tet" ){
		if( !load_tet( filename ) ){ return false; }
		if( !need_surface() ){ return false; }
	}

	// If it's a PLY we need to use tetgen
	else if( ext=="ply" ){

		#ifndef MCLSCENE_ENABLE_TETGEN

		std::cerr << "**TetMesh::Load Error: Tetgen not enabled, use CMake flag MCL_ENABLE_TETGEN" << std::endl;
		return false;

		#else

		std::string new_filename = make_tetmesh( filename );
		if( new_filename.size()==0 ){ return false; }
		if( !load_node( new_filename ) ){ return false; }
		if( !load_ele( new_filename ) ){ return false; }
		if( !need_surface() ){ return false; }

		#endif
	}

	else {
		// Load new data
		if( !load_node( filename ) ){ return false; }
		if( !load_ele( filename ) ){ return false; }
		if( !need_surface() ){ return false; }
	}

	update();

	return true;
}


void TetMesh::need_normals( bool recompute ){

	if( vertices.size() == normals.size() && !recompute ){ return; }

	if( normals.size() != vertices.size() ){ normals.resize( vertices.size() ); }
	const int nv = normals.size();

#pragma omp parallel for
	for( int i = 0; i < nv; ++i ){
		normals[i][0] = 0.f; normals[i][1] = 0.f; normals[i][2] = 0.f;
	}

	int nf = faces.size();
//#pragma omp parallel for
	for( int i = 0; i < nf; ++i ){
		const Vec3f &p0 = vertices[faces[i][0]];
		const Vec3f &p1 = vertices[faces[i][1]];
		const Vec3f &p2 = vertices[faces[i][2]];
		Vec3f a = p0-p1, b = p1-p2, c = p2-p0;
		float l2a = a.squaredNorm(), l2b = b.squaredNorm(), l2c = c.squaredNorm();
		if (!l2a || !l2b || !l2c)
			continue;
		Vec3f facenormal = a.cross( b );
		normals[faces[i][0]] += facenormal * (1.0f / (l2a * l2c));
		normals[faces[i][1]] += facenormal * (1.0f / (l2b * l2a));
		normals[faces[i][2]] += facenormal * (1.0f / (l2c * l2b));
	}

#pragma omp parallel for
	for (int i = 0; i < nv; i++){ normals[i].normalize(); }

} // end compute normals

void TetMesh::update(){

	need_normals(true); aabb.valid=false;

	// Update app data
	this->app.num_vertices = vertices.size();
	this->app.num_normals = normals.size();
	this->app.num_faces = faces.size();
	this->app.num_texcoords = texcoords.size();

	this->app.vertices = &vertices[0][0];
	this->app.normals = &normals[0][0];
	this->app.faces = &faces[0][0];
	this->app.texcoords = &texcoords[0][0];
}

// Transform the mesh by the given matrix
void TetMesh::apply_xform( const trimesh::xform &xf ){

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


bool TetMesh::load_node( std::string filename ){

	// Load the vertices of the tetmesh
	std::stringstream node_file; node_file << filename << ".node";
	std::ifstream filestream;
	filestream.open( node_file.str().c_str() );
	if( !filestream ){ std::cerr << "\n**TetMesh Error: Could not load " << node_file.str() << std::endl; return false; }

	std::string header;
	getline( filestream, header );
	std::stringstream headerSS(header);
	int n_nodes = 0; headerSS >> n_nodes;

	vertices.resize( n_nodes );
	std::vector< int > vertex_set( n_nodes, 0 );
	bool starts_with_one = false;

	for( int i=0; i<n_nodes; ++i ){
		std::string line;
		getline( filestream, line );

		std::stringstream lineSS(line);
		double x, y, z;
		int idx;
		lineSS >> idx >> x >> y >> z;

		// Check for 1-indexed
		if( i==0 && idx==1 ){ starts_with_one = true; }
		if( starts_with_one ){ idx -= 1; }

		if( idx > vertices.size() ){
			std::cerr << "\n**TetMesh Error: Your indices are bad for file " << node_file.str() << std::endl; return false;
		}

		vertices[idx] = Vec3f( x, y, z );
		vertex_set[idx] = 1;
	}
	filestream.close();

	for( int i=0; i<vertex_set.size(); ++i ){
		if( vertex_set[i] == 0 ){ std::cerr << "\n**TetMesh Error: Your indices are bad for file " << node_file.str() << std::endl; return false; }
	}

	return true;

} // end load node file

bool TetMesh::load_ele( std::string filename ){

	// Load the vertices of the tetmesh
	std::stringstream ele_file; ele_file << filename << ".ele";
	std::ifstream filestream;
	filestream.open( ele_file.str().c_str() );
	if( !filestream ){ std::cerr << "\n**TetMesh Error: Could not load " << ele_file.str() << std::endl; return false; }

	std::string header;
	getline( filestream, header );
	std::stringstream headerSS(header);
	int n_tets = 0; headerSS >> n_tets;

	tets.resize( n_tets );
	std::vector< int > tet_set( n_tets, 0 );
	bool starts_with_one = false;

	for( int i=0; i<n_tets; ++i ){
		std::string line;
		getline( filestream, line );

		std::stringstream lineSS(line);
		int idx;
		int node_ids[4];
		lineSS >> idx >> node_ids[0] >> node_ids[1] >> node_ids[2] >> node_ids[3];
//		lineSS >> idx >> node_ids[0] >> node_ids[2] >> node_ids[1] >> node_ids[3];

		// Check for 1-indexed
		if( i==0 && idx==1 ){ starts_with_one = true; }
		if( starts_with_one ){
			idx -= 1;
			for( int j=0; j<4; ++j ){ node_ids[j]-=1; }
		}

		if( idx > tets.size() ){
			std::cerr << "\n**TetMesh Error: Your indices are bad for file " << ele_file.str() << std::endl; return false;
		}

		tets[idx] = Vec4i( node_ids[0], node_ids[1], node_ids[2], node_ids[3] );
		tet_set[idx] = 1;
	}
	filestream.close();

	for( int i=0; i<tet_set.size(); ++i ){
		if( tet_set[i] == 0 ){ std::cerr << "\n**TetMesh Error: Your indices are bad for file " << ele_file.str() << std::endl; return false; }
	}

	return true;

} // end load ele file


bool TetMesh::load_tet( std::string filename ){

	// Load the vertices of the tetmesh
	std::ifstream filestream;
	filestream.open( filename.c_str() );
	if( !filestream ){ std::cerr << "\n**TetMesh Error: Could not load " << filename << std::endl; return false; }

	std::string header;
	getline( filestream, header );
	std::stringstream headerSS(header);
	std::string tetstr;
	int n_tets = 0; int n_verts = 0; headerSS >> tetstr >> n_verts >> n_tets;

	//
	//	Load Vertices
	//
	vertices.reserve( n_verts );
	for( int i=0; i<n_verts; ++i ){
		std::string line;
		getline( filestream, line );
		std::stringstream lineSS(line);
		double x, y, z;
		lineSS >> x >> y >> z;
		vertices.push_back( Vec3f( x, y, z ) );
	}

	//
	//	Load Tets
	//
	tets.reserve( n_tets );
	for( int i=0; i<n_tets; ++i ){
		std::string line;
		getline( filestream, line );
		std::stringstream lineSS(line);
		int a, b, c, d;
		lineSS >> a >> b >> c >> d;
		tets.push_back( Vec4i( a, b, c, d ) );
	}

	filestream.close();

	return true;

} // end load ele file

bool TetMesh::need_surface(){

	// vertex ids -> number of faces using these indices
	std::unordered_map< int3, int > face_ids;

	// Loop over tets and store face information
	for( int t=0; t<tets.size(); ++t ){

		// Indices that make up the tetrahedra
		int p0 = tets[t][0];
		int p1 = tets[t][1];
		int p2 = tets[t][2];
		int p3 = tets[t][3];

		// Faces of each tetrahedra
		int3 curr_faces[4];
		curr_faces[0] = int3( p0, p1, p3 );
		curr_faces[1] = int3( p0, p2, p1 );
		curr_faces[2] = int3( p0, p3, p2 );
		curr_faces[3] = int3( p1, p2, p3 );

		// Store face count
		for( int f=0; f<4; ++f ){
			if( face_ids.count(curr_faces[f]) == 0 ){ face_ids[ curr_faces[f] ] = 1; }
			else{ face_ids[ curr_faces[f] ] += 1; }
		}
	}

	// Loop over face_ids and if a face only exists once (for all tets) its a boundary
	std::unordered_map< int3, int >::iterator faceIt = face_ids.begin();
	for( ; faceIt != face_ids.end(); ++faceIt ){
		if( faceIt->second == 1 ){
			int3 f = faceIt->first;
			faces.push_back( Vec3i( f.orig_v[0], f.orig_v[1], f.orig_v[2] ) );
		}
	}

	return true;

} // end create boundary mesh


void TetMesh::get_surface_vertices( std::vector<int> *indices ){

	bool has_faces = faces.size() > 0;

	// Use the compute faces function to get surface verts
	if( !has_faces ){ need_surface(); }

	std::map<int, bool> vertlist; // Sorted
	for( int i=0; i<faces.size(); ++i ){
		for( int j=0; j<3; ++j ){ vertlist[ faces[i][j] ] = true; }
	}

	// Now that we have a unique list of indices, add them to the buffer
	std::map<int, bool>::iterator it = vertlist.begin();
	for( ; it != vertlist.end(); ++it ){ indices->push_back( it->first ); }
	
	// Remove faces if it didn't already have them
	if( !has_faces ){ faces.clear(); }

} // end get surface verts


void TetMesh::make_tri_refs(){

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


void TetMesh::get_bounds( Vec3f &bmin, Vec3f &bmax ){
	if( !aabb.valid ){
		for( int f=0; f<faces.size(); ++f ){
			aabb += vertices[ faces[f][0] ];
			aabb += vertices[ faces[f][1] ];
			aabb += vertices[ faces[f][2] ];
		}
	}
	bmin = aabb.min; bmax = aabb.max;
}


void TetMesh::save( std::string filename ){

	//
	//	ele file
	//
	// http://wias-berlin.de/software/tetgen/fformats.ele.html
	// First line: <# of tetrahedra> <nodes per tetrahedron> <# of attributes>
	// Remaining lines list of # of tetrahedra:
	// <tetrahedron #> <node> <node> <node> <node> ... [attributes]
	{

		std::stringstream elefn;
		elefn << filename << ".ele";
		std::ofstream filestream;
		filestream.open( elefn.str().c_str() );
		filestream << tets.size() << " 4 0\n";
		for( int i=0; i<tets.size(); ++i ){
			filestream << "\t" << i << ' ' << tets[i][0] << ' ' << tets[i][1] << ' ' << tets[i][2] << ' ' << tets[i][3] << "\n";
		}
		filestream << "# Generated by mclscene (www.mattoverby.net)";
		filestream.close();

	} // end ele file

	//
	//	node file
	//
	// http://wias-berlin.de/software/tetgen/fformats.node.html
	// First line: <# of points> <dimension (must be 3)> <# of attributes> <# of boundary markers (0 or 1)>
	// Remaining lines list # of points:
	// <point #> <x> <y> <z> [attributes] [boundary marker]
	{

		std::stringstream nodefn;
		nodefn << filename << ".node";
		std::ofstream filestream;
		filestream.open( nodefn.str().c_str() );
		filestream << vertices.size() << " 3 0 0\n";
		for( int i=0; i<vertices.size(); ++i ){
			filestream << "\t" << i << ' ' << to_str(vertices[i]) << "\n";
		}
		filestream << "# Generated by mclscene (www.mattoverby.net)";
		filestream.close();		

	} // end node file

} // end save


std::string TetMesh::get_xml( int mode ){

	using namespace std::chrono;
	double timems = duration_cast< milliseconds >(
	    system_clock::now().time_since_epoch()
	).count();

	// Save to a NODE and ELE file
	std::stringstream nodeele;
	nodeele << MCLSCENE_BUILD_DIR<< "/" << timems;
	save( nodeele.str() );
	
	// mclscene
	if( mode == 0 ){
		std::stringstream xml;
		xml << "\t<Object type=\"TetMesh\" >\n";
		xml << "\t\t<File type=\"string\" value=\"" << nodeele.str() << "\" />\n";
		xml << "\t</Object>";
		return xml.str();
	}

	return "";
}


std::string TetMesh::make_tetmesh( std::string filename ){

	#ifdef MCLSCENE_ENABLE_TETGEN

	std::cout << "\n******************************\n* Tetrahedralizing surface mesh. \n* " <<
		"Warning: This is buggy and you're better\n* off doing it yourself!" <<
		"\n******************************\n" << std::endl;

	std::string args = "./tetgen -q " + filename;
	std::vector<std::string> args_s;
	args_s.push_back( "./tetgen" );
	args_s.push_back( filename );
	args_s.push_back( "-F" ); // suppress .faces
	args_s.push_back( "-q" ); // quality mesh
	args_s.push_back( "-Q" ); // quiet terminal output
	char** args_c = tetmesh_helper::make_argv( args_s );

	tetgenio in, addin, bgmin;
	tetgenbehavior b;

	if( !b.parse_commandline( args_s.size(),args_c ) ){
		std::cerr << "\n**TetMesh::tetrahedralize Error: Trouble starting tetgen." << std::endl;
		return "";
	}
	if (b.refine) {
		if (!in.load_tetmesh(b.infilename)) {
			std::cerr << "\n**TetMesh::tetrahedralize Error: Error loading " << filename << std::endl;
			return "";
		}
	} else {
		if (!in.load_plc(b.infilename, (int) b.object)) {
			std::cerr << "\n**TetMesh::tetrahedralize Error: Error loading " << filename << std::endl;
			return "";
		}
	}
	if (b.insertaddpoints) {
		if (!addin.load_node(b.addinfilename)) {
			addin.numberofpoints = 0l;
		}
	}
	if (b.metric) {
		if (!bgmin.load_tetmesh(b.bgmeshfilename)) {
			bgmin.numberoftetrahedra = 0l;
		}
	}

	if (bgmin.numberoftetrahedra > 0l) {
		tetrahedralize(&b, &in, NULL, &addin, &bgmin);
	} else {
		tetrahedralize(&b, &in, NULL, &addin, NULL);
	}

	// Remove last 4 characters (ply)
	for( int i=0; i<4; ++i ){ filename.pop_back(); }
	std::string new_filename = filename + ".1";

	std::cout << "Saving mesh files: " << new_filename << " (.node and .ele)" << std::endl;
	std::cout << "\n******************************\n* Done running tetgen." <<
		"\n******************************\n" << std::endl;
	return new_filename;

	#else

	return "";

	#endif
}







