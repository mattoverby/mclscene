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

#ifndef MCLSCENE_MATERIAL_H
#define MCLSCENE_MATERIAL_H 1

#include <memory>
#include <cassert>
#include "MCL/Param.hpp"

///
///	Simple object types
///
namespace mcl {


// Texture resource will eventually have more parameters
// like mapping algorithm, etc...
class TextureResource {
public:
	TextureResource( std::string name="", std::string file="" ) : m_name(name), m_file(file) {}
	std::string m_name, m_file;
};


//
//	Base, pure virtual
//
class BaseMaterial {
public:
	virtual ~BaseMaterial(){}

	virtual std::string get_type() const = 0;

	// For GLSL materials, the shader is the prefix to the
	// ".vert" and ".frag" files w.r.t. the config directory.
	// When rendering, if no shader is specified, a default
	// Blinn-Phong shader is used.
	virtual std::string get_shader(){ return ""; }

	bool has_texture() { return m_texture.m_file.size(); }

	// Returns a string containing xml code for saving to a scenefile.
	// Mode is:
	//	0 = mclscene
	//	1 = mitsuba
	virtual std::string get_xml( std::string material_name, int mode ){ return ""; }

	TextureResource m_texture;
};


//
//	OpenGL Materials
//
class OGLMaterial : public BaseMaterial {
public:
	OGLMaterial() : diffuse(.5,.5,.5), specular(0,0,0), shininess(0), edge_color(-1,-1,-1) {}

	trimesh::vec diffuse;
	trimesh::vec specular;
	int shininess;
	trimesh::vec edge_color;

	std::string get_type() const { return "ogl"; }

	std::string get_xml( std::string material_name, int mode ){

		// mclscene
		if( mode == 0 ){
			bool draw_edges = ( edge_color[0]>=0.f && edge_color[1]>=0.f && edge_color[2]>=0.f );
			std::stringstream xml;
			xml << "\t<Material name=\"" << material_name << "\" type=\"ogl\" >\n";
			xml << "\t\t<Diffuse type=\"vec3\" value=\"" << diffuse.str() << "\" />\n";
			xml << "\t\t<Specular type=\"vec3\" value=\"" << specular.str() << "\" />\n";
			xml << "\t\t<Shininess type=\"int\" value=\"" << shininess << "\" />\n";
			if( draw_edges ){ xml << "\t\t<Edges type=\"vec3\" value=\"" << edge_color.str() << "\" />\n"; }
			xml << "\t</Material>";
			return xml.str();
		}

		return "";

	} // end get xml
};


//
//	Blinn Phong for glsl
//
class glslBlinnPhong : public BaseMaterial {
public:
	glslBlinnPhong() : diffuse(.5,.5,.5), specular(0,0,0), shininess(0), shader("") {}

	trimesh::vec diffuse;
	trimesh::vec specular;
	int shininess;
	std::string shader;

//	OGLMaterial() : diffuse(.5,.5,.5), specular(0,0,0), shininess(0), edge_color(-1,-1,-1) {}

//	trimesh::vec diffuse;
//	trimesh::vec specular;
//	int shininess;
//	trimesh::vec edge_color;

	std::string get_type() const { return "glslblinnphong"; }
	std::string get_xml( std::string material_name, int mode ){ return ""; }
};


} // end namespace mcl

#endif
