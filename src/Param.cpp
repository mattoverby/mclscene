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

#include "MCL/Param.hpp"

using namespace mcl;

double Param::as_double() const { std::stringstream ss(value); double v; ss>>v; return v; }

char Param::as_char() const { std::stringstream ss(value); char v; ss>>v; return v; }

std::string Param::as_string() const { return value; }

int Param::as_int() const { std::stringstream ss(value); int v; ss>>v; return v; }

long Param::as_long() const { std::stringstream ss(value); long v; ss>>v; return v; }

bool Param::as_bool() const { std::stringstream ss(value); bool v; ss>>v; return v; }

float Param::as_float() const { std::stringstream ss(value); float v; ss>>v; return v; }

trimesh::vec Param::as_vec3() const {
	std::stringstream ss(value);
	trimesh::vec v;
	for( int i=0; i<3; ++i ){ ss>>v[i]; }
	return v;
}

trimesh::vec2 Param::as_vec2() const {
	std::stringstream ss(value);
	trimesh::vec2 v;
	for( int i=0; i<2; ++i ){ ss>>v[i]; }
	return v;
}

trimesh::vec4 Param::as_vec4() const {
	std::stringstream ss(value);
	trimesh::vec4 v;
	for( int i=0; i<4; ++i ){ ss>>v[i]; }
	return v;
}

trimesh::xform Param::as_xform() const {
	trimesh::xform x_form;
	std::stringstream ss(value);
	ss >> x_form;
	return x_form;
}

void Param::normalize(){
std::cout << "TODO: Param::normalize" << std::endl;

	// vec2, vec3, or vec4?
	std::stringstream sscheck( as_string() );
	int num_elem = 0;
	while( sscheck.good() ){ float buff; sscheck >> buff; num_elem++; }

	if(num_elem==3){
		trimesh::vec v = as_vec3();
		trimesh::normalize( v );
		std::stringstream ss; ss << v[0] << ' ' << v[1] << ' ' << v[2];
		value = ss.str();
	}
	else if(num_elem==2){
		trimesh::vec2 v = as_vec2();
		trimesh::normalize( v );
		std::stringstream ss; ss << v[0] << ' ' << v[1];
		value = ss.str();
	}
	else if(num_elem==4){
		trimesh::vec4 v = as_vec4();
		trimesh::normalize( v );
		std::stringstream ss; ss << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << v[3];
		value = ss.str();
	}

}

void Param::fix_color(){

	// vec2, vec3, or vec4?
	std::stringstream sscheck( as_string() );
	int num_elem = 0;
	while( sscheck.good() ){ float buff; sscheck >> buff; num_elem++; }

	if(num_elem==3){
		trimesh::vec c = as_vec3();

		for( int ci=0; ci<3; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
		if( c[0] > 1.0 || c[1] > 1.0 || c[2] > 1.0 ){ for( int ci=0; ci<3; ++ci ){ c[ci]/=255.f; } } // from 0-255 to 0-1

		std::stringstream ss; ss << c[0] << ' ' << c[1] << ' ' << c[2];
		value = ss.str();
	}
	else if(num_elem==2){
		trimesh::vec2 c = as_vec2();

		for( int ci=0; ci<2; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
		if( c[0] > 1.0 || c[1] > 1.0 ){ for( int ci=0; ci<2; ++ci ){ c[ci]/=255.f; } } // from 0-255 to 0-1

		std::stringstream ss; ss << c[0] << ' ' << c[1];
		value = ss.str();
	}
	else if(num_elem==4){
		trimesh::vec4 c = as_vec4();

		for( int ci=0; ci<4; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
		if( c[0] > 1.0 || c[1] > 1.0 || c[2] > 1.0 || c[3] > 1.0 ){ for( int ci=0; ci<4; ++ci ){ c[ci]/=255.f; } } // from 0-255 to 0-1

		std::stringstream ss; ss << c[0] << ' ' << c[1] << ' ' << c[2] << ' ' << c[3];
		value = ss.str();
	}

}


mcl::Param &Component::get( std::string tag ){
	for( int i=0; i<params.size(); ++i ){
		if( params[i].tag == tag ){ return params[i]; }
	}
	// not found, add it
	params.push_back( mcl::Param(tag,"") );
	return params.back();
}


bool Component::exists( std::string tag ) const {
	for( int i=0; i<params.size(); ++i ){
		if( params[i].tag == tag ){ return true; }
	}
	return false;
}
