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

#include "MCL/Vec.hpp"
#include <unordered_map>

namespace mcl {

//
//	ColorMap is for obtaining value-to-color gradients
//
class ColorMap {
public:
	// Preset ramps
	enum {
		GRAYSCALE,
		COLD_HOT, // blue -> red
		BLACKBODY, // black -> yellow
	};

	ColorMap(){ use_preset(GRAYSCALE); }

	// Value is between 0 and 1
	// The avg value is used for 3-color gradients, with the
	// middle color being the average.
	mcl::Vec3f get( float value );

	// Adds a color to the ramp at position (0-1) val
	void add( float val, const mcl::Vec3f &c );

	// Load a preset map
	void use_preset( int preset );

	// Loads a paraview xml file, which can later be 
	// used with the "use(...)" command.
	// See http://www.paraview.org/Wiki/Colormaps for details.
	void load_paraview( std::string xmlfile ){}

	// Loads a previously loaded color map (excluding preset)
	void use( std::string label ){}

	// Clears current gradient
	void clear(){ colors.clear(); }

	// Javascript hex colors from colorbrewer2.org, e.g.:
	// mymap.set("['#e5f5f9','#99d8c9','#2ca25f']");
//	void set( std::string new_map );

private:
	struct cpair {
		cpair( float v_, mcl::Vec3f c_ ){ c=c_; v=v_; }
		float v; mcl::Vec3f c;	
	};
	std::vector<cpair> colors; // the current color map

	// You can load color map files like paraview, which are
	// stored here and index by their name.
	std::unordered_map< std::string, std::vector<cpair> > loaded_maps;

//	Vec3f hex_to_rgb( std::string hex_str );

}; // end class color map


//
//	Implementation
//

mcl::Vec3f ColorMap::get( float value ){

	if( colors.size()==0 ){ use_preset(GRAYSCALE); }

	// Check min/max
	if( value >= 1.f ){ return colors.back().c; }
	else if( value <= 0.f ){ return colors.front().c; }

	// Loop gradient and find color point
	const int n_c = colors.size();
	for( int i=0; i<n_c; ++i ){
		cpair *c = &colors[i];
		if( value < c->v ){
			// From http://www.andrewnoske.com
			cpair *prevc = &colors[std::max(0,i-1)];
			float diff = prevc->v - c->v;
			float frac = ( diff==0.f ) ? 0.f : (value-c->v) / diff;
			mcl::Vec3f color = (prevc->c - c->c)*frac + c->c;
			return color;
		}
	}

	return colors.back().c; // return last color

} // end get color


void ColorMap::add( float val, const mcl::Vec3f &c ){

	// Add and sort with lambda. Doesn't work well with many inserts
	colors.push_back( cpair(val,c) );
	std::sort(colors.begin(), colors.end(), []( const cpair &left, cpair &right) {
		return left.v < right.v;
	});

} // end add color


void ColorMap::use_preset( int preset ){

	colors.clear();
	switch (preset) {

		default:{
			printf("Unknown preset, using grayscale");
			add( 0.f, Vec3f(0,0,0) );
			add( 1.f, Vec3f(1,1,1) );
		} break;

		case GRAYSCALE:{
			add( 0.f, Vec3f(0,0,0) );
			add( 1.f, Vec3f(1,1,1) );
		} break;

		case COLD_HOT:{
			add( 0.f, Vec3f(0,1,1) );
			add( 0.45f, Vec3f(0,0,1) );
			add( 0.5f, Vec3f(1,1,1) );
			add( 0.55f, Vec3f(1,0,0) );
			add( 1.f, Vec3f(1,1,0) );
		} break;

		case BLACKBODY:{
			add( 0.f, Vec3f(0,0,0) );
			add( 0.33f, Vec3f(0.9019,0,0) );
			add( 0.66f, Vec3f(0.9019,0.9019,0) );
			add( 1.f, Vec3f(1,1,1) );
		}

	} // end switch

} // end load preset

} // end namespace mcl

#endif
