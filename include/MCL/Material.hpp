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

#ifndef MCLSCENE_MATERIAL_H
#define MCLSCENE_MATERIAL_H 1

#include <memory>
#include <cassert>
#include "MCL/Param.hpp"

namespace mcl {

//
//	Base material class
//	TODO OpenGL Shader class that derives from Material
//
class Material {
public:
	Material() : flags(0) {}
	Material( Vec3f amb, Vec3f diff, Vec3f spec, float shini ) {
		app.amb = amb; app.diff = diff; app.spec = spec; app.shini = shini; }

	virtual ~Material(){}

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode ){
		std::cout << "TODO: Material::get_xml" << std::endl;
		return "";
	}

	// Used by mcl::Application
	// This data is used by the mclscene OpenGL renderer
	// Colors should be between 0 and 1.
	// Shininess should be between 0 and 1 (and multiplied by 128 later)
	struct AppData {
		AppData() : amb(0,0,0), diff(1,0,0), spec(0,0,0), shini(1), texture("") {}
		Vec3f amb, diff, spec;
		float shini;
		std::string texture;
	} app ;

	// Several flags can be tied to an object, to check them:
	//	bool has_flag = obj->flags & BaseObject::SOME_FLAG
	// You can add your own flag by using any bits past LASTFLAG
	int flags;
	enum {
		// Index replacements:
		NOTSET = -1,
		// Material flags:
		RED_BACKFACE = 1 << 0, // Color backfacing triangles red
		LASTFLAG = 1 << 1, // Color backfacing triangles red
	};
};


} // end namespace mcl

#endif
