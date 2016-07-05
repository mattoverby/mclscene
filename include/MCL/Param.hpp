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

#ifndef MCLSCENE_PARAM_H
#define MCLSCENE_PARAM_H 1

#include "Vec.h"
#include <string>
#include <sstream>
#include "XForm.h"
#include <unordered_map>
#include "../../deps/pugixml/pugixml.hpp"

namespace mcl {

// Nice utility functions for parsing
namespace parse {

	static std::string to_lower( std::string s ){ std::transform( s.begin(), s.end(), s.begin(), ::tolower ); return s; }

	// Returns directory to a file
	static std::string fileDir( std::string fname ){
		size_t pos = fname.find_last_of('/');
		return (std::string::npos == pos) ? "" : fname.substr(0, pos)+'/';
	}

	// Returns file extention
	static std::string get_ext( std::string fname ){
		size_t pos = fname.find_last_of('.');
		return (std::string::npos == pos) ? "" : fname.substr(pos+1,fname.size());
	}

	// Returns file name without extention or directory
	// TODO some basic error checking
	static std::string get_fname( std::string fname ){
		std::string dir = fileDir( fname );
		std::string ext = get_ext( fname );
		return fname.substr( dir.size(), fname.size()-ext.size()-dir.size()-1 );
	}
};


//
//	A parameter parsed from the scene file, stored as a string.
//	Has casting functions for convenience, but assumes the type
//	has an overloaded stream operator (with exception of trimesh::vec).
//	I'm really just copying what pugixml does.
//
//	Tag is ALWAYS lowercase
//
class Param {
public:
	Param( std::string tag_, std::string value_, std::string type_ ) : tag(tag_), value(value_), type(type_) {}

	double as_double() const;
	char as_char() const;
	std::string as_string() const;
	int as_int() const;
	long as_long() const;
	bool as_bool() const;
	float as_float() const;
	trimesh::vec4 as_vec4() const;
	trimesh::vec as_vec3() const;
	trimesh::vec2 as_vec2() const;
	trimesh::xform as_xform() const;

	// Stores the parsed data
	std::string tag;
	std::string value; // string value
	std::string type; // string type

	// Some useful vec3 functions:
	void normalize();
	void fix_color(); // if 0-255, sets 0-1
};


//
//	A component is basically a list of params.
//	Components are parsed from the xml file and always stored.
//
class Component {
public:
	Component( std::string tag_, std::string name_, std::string type_ ) : tag(tag_), name(name_), type(type_) {}
	std::string tag, name, type;
	Param &get( std::string tag );
	Param &operator[]( std::string tag ){ return get(tag); }
	bool exists( std::string tag ) const;
	std::vector<Param> params;
};


//
//	Parses the parameters from a pugi node and creates a vector of them.
//
static void load_params( std::vector<Param> &params, const pugi::xml_node &curr_node ){

	pugi::xml_node::iterator param = curr_node.begin();
	for( param; param != curr_node.end(); ++param ) {
		pugi::xml_node curr_param = *param;

		std::string tag = parse::to_lower( curr_param.name() );
		std::string type_id = curr_param.attribute("type").value();
		std::string value = curr_param.attribute("value").value();
		Param newP( tag, value, type_id );

		if( tag == "xform" ){

			std::stringstream ss( value );
			trimesh::vec v; ss >> v[0] >> v[1] >> v[2];

			trimesh::xform x_form;

			if( parse::to_lower(type_id) == "scale" ){
				x_form = trimesh::xform::scale(v[0],v[1],v[2]);
			}
			else if( parse::to_lower(type_id) == "translate" ){
				x_form = trimesh::xform::trans(v[0],v[1],v[2]);
			}
			else if( parse::to_lower(type_id) == "rotate" ){
				v *= (M_PI/180.f); // convert to radians
				trimesh::xform rot;
				rot = rot * trimesh::xform::rot( v[0], trimesh::vec(1.f,0.f,0.f) );
				rot = rot * trimesh::xform::rot( v[1], trimesh::vec(0.f,1.f,0.f) );
				rot = rot * trimesh::xform::rot( v[2], trimesh::vec(0.f,0.f,1.f) );
				x_form = rot;
			}

			std::stringstream xf_ss; xf_ss << x_form;
			newP.value = xf_ss.str();
		}

		params.push_back( newP );	

	} // end loop params

} // end load parameters


} // end namespace mcl

#endif
