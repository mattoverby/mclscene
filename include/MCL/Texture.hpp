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

#ifndef MCLSCENE_TEXTURE_H
#define MCLSCENE_TEXTURE_H 1

#include "MCL/Vec.hpp"
#include "SOIL2.h"

namespace mcl {

//
//	Helper class for managing OpenGL textures.
//
class Texture {
public:
	Texture() : gl_handle(0), smooth(true), repeated(true) {}

	// Returns the OpenGL handle
	inline unsigned int handle() const { return gl_handle; }

	inline bool load_from_file( const std::string &filename );
 
//	bool loadFromMemory (const void *data, size_t size)

	// Turn smoothing off or on
	inline void set_smooth( bool s );

	// Sets repeated on or off
	inline void set_repeated( bool r );

private:
	bool smooth, repeated;
	unsigned int gl_handle;
};

//
//	Implementation
//

inline bool Texture::load_from_file( const std::string &filename ){

	// TODO replace with my own loader
	int channels, tex_width, tex_height;
	GLuint texture_id = SOIL_load_OGL_texture( mat->app.texture.c_str(), &tex_width, &tex_height, &channels, SOIL_LOAD_AUTO, 0, 0 );
	if( texture_id == 0 ){ std::cerr << "\n**Texture::load Error: Failed to load file " << mat->app.texture << std::endl; continue; }

	glGenTextures( 1, &gl_handle );

}

inline void Texture::set_smooth( bool s ){
	bool update = ( s != smooth );
	smooth = s;
	if( update && gl_handle != 0 ){
		glBindTexture( GL_TEXTURE_2D, gl_handle );
		if( smooth ){ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); }
		else { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); }
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
} // end set smooth


inline void Texture::set_repeated( bool r ){
	bool update = ( r != repeated );
	repeated = r;
	if( update && gl_handle != 0 ){
		glBindTexture( GL_TEXTURE_2D, gl_handle );
		if( repeated ){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
} // end set repeated


} // end namespace mcl



#endif
