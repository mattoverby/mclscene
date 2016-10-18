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

#ifndef MCLSCENE_LIGHT_H
#define MCLSCENE_LIGHT_H 1

#include <memory>
#include "Vec.h"

namespace mcl {

struct AppLight {
	AppLight() : position(0,0,0), intensity(1,1,1), falloff(1,0.1f,0.01f), directional(false) {}
	trimesh::vec position, intensity;
	trimesh::vec falloff; // (constant, linear, quadratic)
	bool directional; // if true, position is actually direction
};


//
//	Base, pure virtual
//
class BaseLight {
public:
	virtual ~BaseLight(){}

	// Get lighting information as used by mcl::Application
	virtual void get_app( AppLight &light ){}

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode ){ return ""; }
};


//
//	Default light type (point/directional)
//
class DefaultLight : public BaseLight {
public:
	void get_app( AppLight &l ){ l=light; }
	std::string get_xml( int mode ){
		std::stringstream xml;
		if( light.directional ){
			xml << "\t<Light type=\"directional\" >\n";
			xml << "\t\t<Direction value=\"" << light.position.str() << "\" />\n";
		} else{
			xml << "\t<Light type=\"point\" >\n";
			xml << "\t\t<Position value=\"" << light.position.str() << "\" />\n";
		}
		xml << "\t\t<Intensity value=\"" << light.intensity.str() << "\" />\n";
		xml << "\t\t<Falloff value=\"" << light.falloff.str() << "\" />\n";
		xml << "\t</Light>";
		return xml.str();
	} // end get xml
	AppLight light;
};

} // end namespace mcl

#endif
