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

} // end namespace mcl

#endif
