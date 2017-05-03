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
	// Preset ramps: mapping goes from
	// 0 = first color, 1 = last color
	enum {
		GRAYSCALE, // grayscale
		COLD_HOT, // blue to red
		BLACKBODY, // black, yellow, white
	};

	// "gradlist" stores a list of loaded color gradients.
	// "colors" is a pointer to the activly loaded color gradient.
	ColorMap(){
		gradlist[ "mcl_preset_ramp" ] = std::vector<cpair>();
		colors = &gradlist[ "mcl_preset_ramp" ];
		use_preset(GRAYSCALE);
	}

	// Value is between 0 and 1
	// The avg value is used for 3-color gradients, with the
	// middle color being the average.
	mcl::Vec3f get( float value );

	// Adds a color to the current ramp at position (0-1) val
	void add( float val, const mcl::Vec3f &c );

	// Load a preset map. The "avg" argument is used
	// to set the middle of the ramp which is only used
	// for some gradients (e.g. hot cold).
	void use_preset( int preset, float avg=0.5 );

	// Clears current gradient
	void clear(){ colors->clear(); }

	// Use a labeled color gradient loaded with
	// the "load_" function. Returns true if the gradient
	// was successfully loaded.
//	bool use( std::string label ){}

	// Load a ParaView colormap xml file.
//	void load_paraview( std::string xml ){}

	// Javascript hex colors from colorbrewer2.org, e.g.:
	// mymap.set("['#e5f5f9','#99d8c9','#2ca25f']");
//	void set( std::string new_map );

private:
	struct cpair {
		cpair( float v_, mcl::Vec3f c_ ){ c=c_; v=v_; }
		float v; mcl::Vec3f c;	
	};
	std::vector<cpair> *colors; // currently loaded color map
	std::unordered_map<std::string, std::vector<cpair> > gradlist;

}; // end class color map


//
//	Implementation
//

mcl::Vec3f ColorMap::get( float value ){

	if( colors->size()==0 ){ use_preset(GRAYSCALE); }

	// Check min/max
	if( value >= 1.f ){ return colors->back().c; }
	else if( value <= 0.f ){ return colors->front().c; }

	// Loop gradient and find color point
	const int n_c = colors->size();
	for( int i=0; i<n_c; ++i ){
		cpair *c = &(*colors)[i];
		if( value < c->v ){
			// From http://www.andrewnoske.com
			cpair *prevc = &(*colors)[std::max(0,i-1)];
			float diff = prevc->v - c->v;
			float frac = ( diff==0.f ) ? 0.f : (value-c->v) / diff;
			mcl::Vec3f color = (prevc->c - c->c)*frac + c->c;
			return color;
		}
	}

	return colors->back().c; // return last color

} // end get color


void ColorMap::add( float val, const mcl::Vec3f &c ){

	if( val > 1.f ){ val = 1.f; }
	if( val < 0.f ){ val = 0.f; }

	// Add and sort with lambda. Doesn't work well with many inserts
	colors->push_back( cpair(val,c) );
	std::sort(colors->begin(), colors->end(), []( const cpair &left, cpair &right) {
		return left.v < right.v;
	});
}


void ColorMap::use_preset( int preset, float avg ){

	// We'll just keep overwriting the same preset
	colors->clear();
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
			float frac = std::min( (1.f-avg), avg )*0.2f;
			float upper = avg+frac;
			float lower = avg-frac;
			add( 1.f, Vec3f(202,0,32) );
			add( upper, Vec3f(244,165,130) );
			add( lower, Vec3f(146,197,222) );
			add( 0.f, Vec3f(5,113,176) );
			for( int i=0; i<colors->size(); ++i ){ (*colors)[i].c *= 1.f/255.f; }
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
