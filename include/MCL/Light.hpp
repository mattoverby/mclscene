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

#ifndef MCLSCENE_LIGHT_H
#define MCLSCENE_LIGHT_H 1

#include <memory>
#include <cassert>
#include "Vec.h"

namespace mcl {


//
//	Base, pure virtual
//
class BaseLight {
public:
	virtual ~BaseLight(){}
	virtual std::string get_type() const = 0;

	// Returns a string containing xml code for saving to a scenefile.
	// Mode is:
	//	0 = mclscene
	//	1 = mitsuba
	virtual std::string get_xml( std::string light_name, int mode ){ return ""; }
};


class OGLLight : public BaseLight {
public:
	// m_type = 0 (directional) or 1 (point)
	OGLLight() : m_type(1), m_pos(1,1,0), m_ambient(0.3,0.3,0.3), m_diffuse(.7,.7,.7), m_specular(0.8,0.8,0.8) {}
	std::string get_type() const { return "ogl"; }

	int m_type; // 0 (directional) or 1 (point)
	trimesh::vec m_pos;
	trimesh::vec m_ambient, m_diffuse, m_specular; // colors

	std::string get_xml( std::string light_name, int mode ){

		// mclscene
		if( mode == 0 ){
			std::stringstream xml;
			xml << "\t<Light name=\"" << light_name << "\" type=\"ogl\" >\n";
			xml << "\t\t<Ambient value=\"" << m_ambient.str() << "\" />\n";
			xml << "\t\t<Diffuse value=\"" << m_diffuse.str() << "\" />\n";
			xml << "\t\t<Specular value=\"" << m_specular.str() << "\" />\n";
			if( m_type==0 ){ xml << "\t\t<Direction value=\"" << m_pos.str() << "\" />\n"; }
			else { xml << "\t\t<Position value=\"" << m_pos.str() << "\" />\n"; }
			xml << "\t</Light>";
			return xml.str();
		}

		return "";

	} // end get xml

};


} // end namespace mcl

#endif
