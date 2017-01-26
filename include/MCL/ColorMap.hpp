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

#ifndef MCLSCENE_COLORMAP_H
#define MCLSCENE_COLORMAP_H 1

#include "Vec.hpp"

namespace mcl {

//	TODO this class in under construction
//
//	ColorMap is a class to manage colors for data sets.
//	There are several default named mappings, but you can
//	also set a map from colorbrewer2.org by copying the text
//	in export->javascript.
//
class ColorMap {
public:
	// Javascript hex colors from colorbrewer2.org, e.g.:
	// mymap.set_map("['#e5f5f9','#99d8c9','#2ca25f']");
	void set_map( std::string new_map );

private:
	std::vector<Vec3f> map; // rgbs

	Vec3f hex_to_rgb( std::string hex_str );

}; // end class color map


//
//	Implementation
//

void ColorMap::set_map( std::string new_map ){}


Vec3f ColorMap::hex_to_rgb( std::string hex_str ){}

} // end namespace mcl

#endif
