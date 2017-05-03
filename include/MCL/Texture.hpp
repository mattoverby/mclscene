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

// TODO OpenGL includes
#include <string>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace mcl {

//
//	Helper class for managing OpenGL textures.
//
class Texture {
public:
	Texture() : width(0), height(0), smooth(true), repeated(true), gl_handle(0) {}

	// Destructor frees texture mem
	~Texture(){ if(gl_handle>0){ glDeleteTextures(1, &gl_handle); } }

	// Returns whether the texture is valid or not
	inline bool valid(){ return gl_handle!=0; }

	// Returns the OpenGL handle
	inline unsigned int handle() const { return gl_handle; }

	// Creates a texture from file
	inline bool create_from_file( const std::string &filename, bool alpha=false );
 
	// Creates a texture from memory
	inline bool create_from_memory( int width_, int height_, float *data, bool alpha=false );

	// Turn smoothing off or on
	inline void set_smooth( bool s );

	// Sets repeated on or off
	inline void set_repeated( bool r );

	// Returns size in pixels of the loaded texture
	inline void get_size( int *w, int *h ) const { *w=width; *h=height; }

private:
	int width, height;
	bool smooth, repeated;
	unsigned int gl_handle;
};


//
//	Implementation
//


inline bool Texture::create_from_file( const std::string &filename, bool alpha ){

	glGenTextures( 1, &gl_handle );
	if( gl_handle == 0 ){
		printf("\n**Texture::create Error: Failed to gen");
		return false;
	}

	int comp;
	int mode = STBI_rgb;
	if( alpha ){ mode = STBI_rgb_alpha; }
	unsigned char* image = stbi_load(filename.c_str(), &width, &height, &comp, mode);
	if( image == NULL ){ return false; }

	glBindTexture(GL_TEXTURE_2D, gl_handle);
	glTexImage2D(GL_TEXTURE_2D, 0, alpha?GL_RGBA:GL_RGB, width, height, 0, alpha?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeated?GL_REPEAT:GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeated?GL_REPEAT:GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth?GL_LINEAR:GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image);
	return true;
}


inline bool Texture::create_from_memory( int width_, int height_, float *data, bool alpha ){

	glGenTextures( 1, &gl_handle );
	if( gl_handle == 0 ){ printf("\n**Texture::create Error: Failed to gen"); return false; }

	width = width_;
	height = height_;
	int mode = GL_RGBA;
	if( !alpha ){ mode = GL_RGB; }
	glBindTexture( GL_TEXTURE_2D, gl_handle );
	glTexImage2D( GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_FLOAT, data );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeated?GL_REPEAT:GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeated?GL_REPEAT:GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth?GL_LINEAR:GL_NEAREST);
	glBindTexture( GL_TEXTURE_2D, 0 );
}


inline void Texture::set_smooth( bool s ){
	bool update = ( s != smooth ) && gl_handle != 0;
	smooth = s;
	if( update ){
		glBindTexture( GL_TEXTURE_2D, gl_handle );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth?GL_LINEAR:GL_NEAREST);
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
} // end set smooth


inline void Texture::set_repeated( bool r ){
	bool update = ( r != repeated ) && gl_handle != 0;
	repeated = r;
	if( update ){
		glBindTexture( GL_TEXTURE_2D, gl_handle );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeated?GL_REPEAT:GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeated?GL_REPEAT:GL_CLAMP_TO_EDGE);
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
} // end set repeated


} // end namespace mcl


#endif
