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

#ifdef MCLSCENE_ENABLE_TETGEN
	static char **make_argv( const std::vector<std::string> &s ){
		char **args;
		args = new char *[s.size()];
		for( size_t i = 0; i < s.size(); ++i ){
			args[i] = new char[s[i].length() + 1];
			strcpy( args[i], s[i].c_str());
		} 
		return args;
	}
#endif
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

	else if( ext=="mesh" ){
		if( !load_mesh( filename ) ){ return false; }
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

	return true;
}


void TetMesh::need_normals( bool recompute ){

	if( vertices.size() == normals.size() && !recompute ){ return; }

	if( normals.size() != vertices.size() ){ normals.resize( vertices.size() ); }
	const int nv = normals.size();

	std::fill( normals.begin(), normals.end(), Vec3f(0,0,0) );

	int nf = faces.size();
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

	for (int i = 0; i < nv; i++){ normals[i].normalize(); }

} // end compute normals

bool TetMesh::get_vertices(
	float* &verts, int &num_vertices,
	float* &norms, int &num_normals,
	float* &tex, int &num_texcoords ){

	if( normals.size()==0 ){ need_normals(); }

	// Update app data
	num_vertices = vertices.size();
	num_normals = normals.size();
	num_texcoords = texcoords.size();
//	this->app.num_edges = edges.size();

	verts = &vertices[0][0];
	norms = &normals[0][0];
	tex = &texcoords[0][0];
//	this->app.edges = &edges[0][0];

	return true;
}

bool TetMesh::get_primitives( const Prim &type, int* &indices, int &num_prims ){
	if( type==Prim::Edge ){ indices = &edges[0][0]; num_prims = edges.size(); return true; }
	if( type==Prim::Tet ){ indices = &tets[0][0]; num_prims = tets.size(); return true; }
	if( type==Prim::Tri ){ indices = &faces[0][0]; num_prims = faces.size(); return true; }
	return false;
}


// Transform the mesh by the given matrix
void TetMesh::apply_xform( const trimesh::xform &xf ){

	int nv = vertices.size();
	for (int i = 0; i < nv; i++){ vertices[i] = xf * vertices[i]; }
	need_normals(true);
	aabb.valid = false;
	for( size_t f=0; f<faces.size(); ++f ){
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
		size_t idx;
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

	for( size_t i=0; i<vertex_set.size(); ++i ){
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
		size_t idx;
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

	for( size_t i=0; i<tet_set.size(); ++i ){
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


bool TetMesh::load_mesh( std::string filename ){

	// Load the vertices of the tetmesh
	std::ifstream filestream;
	filestream.open( filename.c_str() );
	if( !filestream ){ std::cerr << "\n**TetMesh Error: Could not load " << filename << std::endl; return false; }
	vertices.clear();
	tets.clear();


	//
	//	Might segfault. Don't care.
	//
	bool parsing_verts = false;
	bool parsing_tets = false;
	while( !filestream.eof() ){
		std::string line="";
		std::getline(filestream, line);
		std::string linecpy = line;
		std::string::iterator end_pos = std::remove(linecpy.begin(), linecpy.end(), ' ');
		linecpy.erase(end_pos, linecpy.end());
		if( linecpy.size()==0 ){ continue; }

		std::stringstream ss1(line);
		std::string tok; ss1 >> tok;

		// Get num vertices
		if( tetmesh_helper::to_lower(tok) == "vertices" ){
			std::getline(filestream,line);
			std::stringstream ss2(line);
			int n_verts; ss2 >> n_verts;
			if( n_verts <= 0 ){
				std::cerr << "**TetMesh::load_mesh error: bad vertices: " << n_verts << std::endl;
				return false;
			}
			vertices.resize(n_verts);
			parsing_verts = true;
			parsing_tets = false;
			continue;
		}
		// Get num tets
		else if( tetmesh_helper::to_lower(tok) == "tetrahedra" ){
			std::getline(filestream,line);
			std::stringstream ss2(line);
			int n_tets; ss2 >> n_tets;
			if( n_tets <= 0 ){
				std::cerr << "**TetMesh::load_mesh error: bad tets: " << n_tets << std::endl;
				return false;
			}
			tets.resize(n_tets);
			parsing_verts = false;
			parsing_tets = true;
			continue;
		}

		else if( tetmesh_helper::to_lower(tok) == "end" ){
			break;
		}

		// Parsing verts
		if( parsing_verts ){
			std::stringstream ss(line);
			float x, y, z; int idx=-1;
			ss >> x >> y >> z >> idx;
			idx--;
			if( idx < 0 || idx >= (int)vertices.size() )
				vertices[idx] = Vec3f(x,y,z);
		} else if( parsing_tets ){
			std::stringstream ss(line);
			int x, y, z, w; int idx=-1;
			ss >> x >> y >> z >> w >> idx;
			idx--; x--; y--; z--; w--;
			if( idx < 0 || idx >= (int)tets.size() )
				tets[idx] = Vec4i(x,y,z,w);
		}

	} // end loop file

	filestream.close();

	return true;

} // end load ele file



bool TetMesh::need_surface(){

	if( faces.size()>0 ){ return true; }
	using namespace hashkey;

	// vertex ids -> number of faces using these indices
	std::unordered_map< int3, int > face_ids;

	// Loop over tets and store face information
	size_t n_tets = tets.size();
	for( size_t t=0; t<n_tets; ++t ){

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
	size_t n_faces = faces.size();
	for( size_t i=0; i<n_faces; ++i ){
		for( int j=0; j<3; ++j ){ vertlist[ faces[i][j] ] = true; }
	}

	// Now that we have a unique list of indices, add them to the buffer
	std::map<int, bool>::iterator it = vertlist.begin();
	for( ; it != vertlist.end(); ++it ){ indices->push_back( it->first ); }
	
	// Remove faces if it didn't already have them
	if( !has_faces ){ faces.clear(); }

} // end get surface verts

/*
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
*/

void TetMesh::need_edges(){

	if( edges.size()>0 ){ return; }
	if( faces.size()==0 ){ need_surface(); }

	edges.reserve( faces.size()*3 );
	size_t n_faces = faces.size();
	for( size_t f=0; f<n_faces; ++f ){
		edges.push_back( Vec2i(faces[f][0],faces[f][1]) );
		edges.push_back( Vec2i(faces[f][0],faces[f][2]) );
		edges.push_back( Vec2i(faces[f][1],faces[f][2]) );
	}
//	this->app.num_edges = edges.size();
//	this->app.edges = &edges[0][0];

}


void TetMesh::get_bounds( Vec3f &bmin, Vec3f &bmax ){
	if( !aabb.valid ){
		size_t n_faces = faces.size();
		for( size_t f=0; f<n_faces; ++f ){
			aabb += vertices[ faces[f][0] ];
			aabb += vertices[ faces[f][1] ];
			aabb += vertices[ faces[f][2] ];
		}
	}
	bmin = aabb.min; bmax = aabb.max;
}


void TetMesh::save( std::string filename ){


	std::string ext = tetmesh_helper::to_lower( tetmesh_helper::get_ext(filename) );
	if( ext == "tet" ){

		std::ofstream filestream;
		filestream.open( filename.c_str() );
		filestream << "tet " << vertices.size() << ' ' << tets.size();
		for( size_t i=0; i<vertices.size(); ++i ){
			filestream << "\n" << vertices[i][0] << ' ' << vertices[i][1] << ' ' << vertices[i][2];
		}
		for( size_t i=0; i<tets.size(); ++i ){
			filestream << "\n" << tets[i][0] << ' ' << tets[i][1] << ' ' << tets[i][2] << ' ' << tets[i][3];
		}

		filestream << "# Generated by mclscene (www.mattoverby.net)";
		filestream.close();

	} else {

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
			for( size_t i=0; i<tets.size(); ++i ){
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
			for( size_t i=0; i<vertices.size(); ++i ){
				filestream << "\t" << i << ' ' << to_str(vertices[i]) << "\n";
			}
			filestream << "# Generated by mclscene (www.mattoverby.net)";
			filestream.close();		

		} // end node file

	} // end save node/ele

} // end save

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

std::string TetMesh::get_xml( int mode ){

	std::string timestamp = get_timestamp_meshname();
	std::string filename = "";

	// Get mesh number by using mesh_exists
	// Not an elegant solution but fine for now.
	int mesh_num = 0;
	for( int i=0; i<10000; ++i ){
		std::stringstream fn; fn << timestamp << "-" << mesh_num << ".tet";
		if( !mesh_exists( fn.str() ) ){ filename = fn.str(); break; }
		mesh_num++;
	}

	if( !filename.size() ){
		std::cerr << "TetMesh::Error: Problem exporting file for scene save" << std::endl;
		return "";
	}

	// Save the mesh
	std::stringstream objfile;
	objfile << MCLSCENE_BUILD_DIR<< "/" << filename;
	save( objfile.str() );
	
	// mclscene
	if( mode == 0 ){
		std::stringstream xml;
		xml << "\t<Object type=\"TetMesh\" >\n";
		xml << "\t\t<File type=\"string\" value=\"" << objfile.str() << "\" />\n";
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

	(void)filename;
	return "";

	#endif
}



void TetMesh::collapse_points( float distance ){

	float dx = distance*distance;
	std::unordered_map< int, std::vector<int> > same_as;

	// Make a map of nodes that overlap
	for( size_t i=0; i<vertices.size(); ++i ){
		same_as[i] = std::vector<int>();
		for( size_t j=0; j<vertices.size(); ++j ){
			if( i==j ){ continue; }
			float dist = (vertices[i]-vertices[j]).squaredNorm();
			if( dist < dx ){
				same_as[i].push_back(j);
			}
		}
	}

	// Remove duplicates
	std::unordered_map< int, std::vector<int> >::iterator it = same_as.begin();
	for( ; it != same_as.end(); ++it ){
		for( size_t i=0; i<it->second.size(); ++i ){
			same_as.erase( it->second[i] );
		}
	}

	// Update vertices
	std::unordered_map< int, int > vertex_map;
	std::vector<Vec3f> old_verts = vertices;
	vertices.clear();
	it = same_as.begin();
	for( ; it != same_as.end(); ++it ){
		int orig_idx = it->first;
		int new_idx = vertices.size();
		vertex_map[ orig_idx ] = new_idx;
		if( same_as.count(orig_idx)>0 ){
			for( size_t i=0; i<same_as[orig_idx].size(); ++i ){
				vertex_map[ same_as[orig_idx][i] ] = new_idx;
			}
		}
		vertices.push_back( old_verts[ it->first ] );
	}

	// Update faces
	for( size_t i=0; i<faces.size(); ++i ){
		for( int j=0; j<3; ++j ){
			faces[i][j] = vertex_map[ faces[i][j] ];
		}
	}

	// Update tets
	for( size_t i=0; i<tets.size(); ++i ){
		for( int j=0; j<4; ++j ){
			tets[i][j] = vertex_map[ tets[i][j] ];
		}
	}

	// Remake normals
	if( normals.size() ){
		normals.clear();
		need_normals(true);
	}

} // end collapse points



