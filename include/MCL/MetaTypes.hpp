// Copyright 2014 Matthew Overby.
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

#ifndef MCLSCENE_METATYPES_H
#define MCLSCENE_METATYPES_H 1

#include "TetMesh.hpp"
#include "TriMesh.h"
#include "bsphere.h"
#include "XForm.h"
#include "TriMesh_algo.h"
#include "pugixml.hpp"
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <map>

///
///	Metadata structs loaded from the config file
///
namespace mcl {

// Nice utility functions for parsing
namespace parse {

	static std::string to_lower( std::string s ){ std::transform( s.begin(), s.end(), s.begin(), ::tolower ); return s; }

	// Returns directory to a file
	static std::string fileDir( std::string fname ){
		size_t pos = fname.find_last_of('/');
		return (std::string::npos == pos) ? "" : fname.substr(0, pos)+'/';
	}
};

//
//	Parameter list included in every meta type
//
//	See the check_params functions for required parameters
//
class Parameters {
public:
	// Returns true on success
	bool load_params( const pugi::xml_node &curr_node );

	// Parameter list
	//	Parameter label -> vector of value
	//	Order is preserved for names, but not across types.
	//
	std::unordered_map< std::string, std::vector< double > > dbl_vals;
	std::unordered_map< std::string, std::vector< char > > char_vals;
	std::unordered_map< std::string, std::vector< std::string > > str_vals;
	std::unordered_map< std::string, std::vector< int > > int_vals;
	std::unordered_map< std::string, std::vector< long > > long_vals;
	std::unordered_map< std::string, std::vector< bool > > bool_vals;
	std::unordered_map< std::string, std::vector< float > > float_vals;
	std::unordered_map< std::string, std::vector< trimesh::vec > > vec3_vals;

	// Transforms are unique in which they are constructed
	// as they are parsed so that order is preserved.
	// The value portion is always a vec3.
	// Their xml syntax is:
	// 	<XForm type="scale/translate/rotate" value="0 100 0" />
	trimesh::xform x_form;
};

//
//	Camera
//
class CameraMeta {
public:
	Parameters p;
	std::string name;

	bool check_params();

	// Set by check_params:
	std::string type;
	trimesh::vec pos, dir, lookat;
};

//
//	Material
//
class MaterialMeta {
public:
	Parameters p;
	std::string name;

	bool check_params();

	// Set by check_params:
	std::string type;
	trimesh::vec diffuse, specular;
	float exponent; // i.e. shininess
};

//
//	Light
//
class LightMeta {
public:
	Parameters p;
	std::string name;

	bool check_params();

	// Set by check_params:
	std::string type;
	trimesh::vec pos, intensity, dir;
};

//
//	Object
//
//	ObjectMeta is a little special compared to the others as
//	it can build and create certain objects. When built, this
//	data is cached and return on subsequent build calls
//
class ObjectMeta {
public:
	ObjectMeta() : built_TriMesh(NULL), built_TetMesh(NULL) {}
	Parameters p;
	std::string name;

	bool check_params();

	// Set by check_params:
	std::string type;
	std::string material; // for material_map

	// Build functions
	std::shared_ptr<trimesh::TriMesh> as_TriMesh();
	std::shared_ptr<TetMesh> as_TetMesh();

protected:
	std::shared_ptr<trimesh::TriMesh> built_TriMesh;
	std::shared_ptr<TetMesh> built_TetMesh;

	#if MCLSCENE_SCENEMANAGER_H
	friend class SceneManager;
	#endif
};


} // end namespace mcl

#endif
