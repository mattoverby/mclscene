// Copyright 2014 Matthew Overby.
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

#include "MCL/MetaTypes.hpp"

using namespace mcl;
using namespace trimesh;


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

void Param::normalize(){
	if( type != "vec3" ){ return; }
	trimesh::vec v = as_vec3();
	trimesh::normalize( v );
	std::stringstream ss; ss << v[0] << ' ' << v[1] << ' ' << v[2];
	value = ss.str();
}

void Param::fix_color(){
	if( type != "vec3" ){ return; }
	trimesh::vec c = as_vec3();
	for( int ci=0; ci<3; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
	if( c[0] > 1.0 || c[1] > 1.0 || c[2] > 1.0 ){
		for( int ci=0; ci<3; ++ci ){ c[ci]/=255.f; } // from 0-255 to 0-1
	}
	std::stringstream ss; ss << c[0] << ' ' << c[1] << ' ' << c[2];
	value = ss.str();
}


bool BaseMeta::load_params( const pugi::xml_node &curr_node ){

	pugi::xml_node::iterator param = curr_node.begin();
	for( param; param != curr_node.end(); ++param ) {
		pugi::xml_node curr_param = *param;

		std::string name = parse::to_lower( curr_param.name() );
		std::string type_id = curr_param.attribute("type").value();
		std::string value = curr_param.attribute("value").value();
		Param newP( name, value, type_id );

		if( name == "xform" ){

			std::stringstream ss( value );
			trimesh::vec v; ss >> v[0] >> v[1] >> v[2];

			if( parse::to_lower(type_id) == "scale" ){
				trimesh::xform scale = trimesh::xform::scale(v[0],v[1],v[2]);
				x_form = scale * x_form;
			}
			else if( parse::to_lower(type_id) == "translate" ){
				trimesh::xform translate = trimesh::xform::trans(v[0],v[1],v[2]);
				x_form = translate * x_form;
			}
			else if( parse::to_lower(type_id) == "rotate" ){
				v *= (M_PI/180.f); // convert to radians
				trimesh::xform rot;
				rot = rot * trimesh::xform::rot( v[0], trimesh::vec(1.f,0.f,0.f) );
				rot = rot * trimesh::xform::rot( v[1], trimesh::vec(0.f,1.f,0.f) );
				rot = rot * trimesh::xform::rot( v[2], trimesh::vec(0.f,0.f,1.f) );
				x_form = x_form * rot;
			}
		}
		else{
			param_map[ name ] = param_vec.size();
			param_vec.push_back( newP );	
		}

	} // end loop params

	return true;

}


Param BaseMeta::operator[]( const std::string tag ) const {
	std::unordered_map< std::string, int >::const_iterator it = param_map.find(tag);
	assert( it != param_map.end() ); assert( it->second < param_vec.size() );
	return param_vec[ it->second ];
}

void BaseMeta::add_param( const Param &p ){
	param_map[ p.name ] = param_vec.size();
	param_vec.push_back( p );	
}


bool CameraMeta::check_params(){

	if( param_map.count( "position" )==0 ){ printf("Camera Error: No vec3 position!"); return false; }
	if( param_map.count( "up" )==0 ){ printf("Camera Error: No vec3 up!"); return false; }
	if( param_map.count( "lookat" )==0 && param_map.count( "direction" )==0 ){ printf("Camera Error: No vec3 direction/lookat!"); return false; }
	if( param_map.count( "type" )==0 ){ printf("Camera Error: No str type!"); return false; }
	if( param_map.count( "lookat" )>0 && param_map.count( "direction" )>0 ){ printf("Camera Error: Both direction and lookat set!"); return false; }

	// Normalize directions
	if( param_map.count( "direction" )>0 ){
		param_vec[ param_map[ "direction" ] ].normalize();
		dir = param_vec[ param_map[ "direction" ] ].as_vec3();
		lookat = pos+dir;
	}
	else if( param_map.count( "lookat" )>0 ){
		lookat = param_vec[ param_map[ "lookat" ] ].as_vec3();
		dir = lookat-pos; normalize(dir);
	}

	// Set members
	pos = param_vec[ param_map[ "position" ] ].as_vec3();
	type = param_vec[ param_map[ "type" ] ].as_string();

	return true;
}


bool MaterialMeta::check_params(){

	if( param_map.count( "type" )==0 ){ printf("Material Error: No str type!"); return false; }

	type = param_vec[ param_map[ "type" ] ].as_string();
	diffuse = vec(.5,.5,.5);
	specular = vec(0,0,0);
	exponent = 0;

	if( param_map.count( "diffuse" )>0 ){
		param_vec[ param_map[ "diffuse" ] ].fix_color();
		diffuse = param_vec[ param_map[ "diffuse" ] ].as_vec3();
	}

	if( param_map.count( "specular" )>0 ){
		param_vec[ param_map[ "specular" ] ].fix_color();
		specular = param_vec[ param_map[ "specular" ] ].as_vec3();
	}

	if( param_map.count("exponent")>0 ){ exponent = param_vec[ param_map[ "exponent" ] ].as_double(); }

	return true;
}


bool LightMeta::check_params(){

	if( param_map.count( "type" )==0 ){ printf("Light Error: No str type!"); return false; }
	if( param_map.count( "intensity" )==0 ){ printf("Light Error: No vec3 intensity!"); return false; }
	type = param_vec[ param_map[ "type" ] ].as_string();

	if( type == "directional" ){
		if( param_map.count( "direction" )==0 ){ printf("Light Error: No vec3 direction!"); return false; }

		// Normalize direction
		param_vec[ param_map[ "direction" ] ].normalize();
		dir = param_vec[ param_map[ "direction" ] ].as_vec3();
	}
	else if( type == "point" ){
		if( param_map.count( "position" )==0 ){ printf("Light Error: No vec3 position!"); return false; }
		pos = param_vec[ param_map[ "position" ] ].as_vec3();
	}

	param_vec[ param_map[ "intensity" ] ].fix_color();
	intensity = param_vec[ param_map[ "intensity" ] ].as_vec3();

	return true;
}


bool ObjectMeta::check_params(){

	if( param_map.count( "type" )==0 ){ printf("\nObject Error: No str type!"); return false; }
	type = param_vec[ param_map[ "type" ] ].as_string();

	if( param_map.count( "material" )>0 ){ material = param_vec[ param_map[ "material" ] ].as_string(); }

	return true;
}


std::shared_ptr<trimesh::TriMesh> ObjectMeta::as_TriMesh(){

	if( built_TriMesh != NULL ){ return built_TriMesh; }

	//
	//	Sphere Type
	//
	if( parse::to_lower(type) == "sphere" ){

		if( param_map.count( "radius" )==0 ){ printf("\nObject Error: No radius!"); assert(false); }
		if( param_map.count( "center" )==0 ){ printf("\nObject Error: No vec3 center!"); assert(false); }

		double rad = param_vec[ param_map[ "radius" ] ].as_double();
		trimesh::vec center = param_vec[ param_map[ "center" ] ].as_vec3();

		int tess = 32;
		if( param_map.count( "tess" )>0 ){ param_vec[ param_map[ "tess" ] ].as_int(); }

		// Create a sphere (which creates a trimesh)
		built_obj = std::shared_ptr<BaseObject>( new Sphere( center, rad, tess ) );
		built_TriMesh = built_obj.get()->as_TriMesh();

		// Apply transformation matrix
		trimesh::apply_xform( built_TriMesh.get(), x_form );

		// Reset normals/tstrips
		built_TriMesh.get()->normals.clear();
		built_TriMesh.get()->tstrips.clear();
		built_TriMesh.get()->need_normals();
		built_TriMesh.get()->need_tstrips();

	}

	//
	//	Box Type
	//
	else if( parse::to_lower(type) == "box" ){

		if( param_map.count( "boxmin" )==0 ){ printf("\nObject Error: No vec3 boxmin!"); assert(false); }
		if( param_map.count( "boxmax" )==0 ){ printf("\nObject Error: No vec3 boxmax!"); assert(false); }

		trimesh::vec boxmin = param_vec[ param_map[ "boxmin" ] ].as_vec3();
		trimesh::vec boxmax = param_vec[ param_map[ "boxmax" ] ].as_vec3();

		int tess = 1;
		if( param_map.count( "tess" )>0 ){ param_vec[ param_map[ "tess" ] ].as_int(); }

		// Create a box (which creates a trimesh)
		built_obj = std::shared_ptr<BaseObject>( new Box( boxmin, boxmax, tess ) );
		built_TriMesh = built_obj.get()->as_TriMesh();

		// Apply transformation matrix
		trimesh::apply_xform( built_TriMesh.get(), x_form );

		// Reset normals/tstrips
		built_TriMesh.get()->normals.clear();
		built_TriMesh.get()->tstrips.clear();
		built_TriMesh.get()->need_normals();
		built_TriMesh.get()->need_tstrips();

	}

	//
	//	TriMesh Type
	//
	else if( parse::to_lower(type) == "trimesh" ){

		if( param_map.count("file")==0 ){ printf("TriMesh Error: No file specified"); assert(false); }

		std::string file = param_vec[ param_map[ "file" ] ].as_string();

		// Try to load the trimesh
		std::shared_ptr<trimesh::TriMesh> newMesh( trimesh::TriMesh::read( file.c_str() ) );
		if( !newMesh.get() ){ printf("Trimesh Error: Could not load %s", file.c_str() ); assert(false); }
		newMesh.get()->set_verbose(0);

		// Now clean the mesh
		trimesh::remove_unused_vertices( newMesh.get() );

		// Apply transformation matrix
		trimesh::apply_xform( newMesh.get(), x_form );

		// Create triangle strip for rendering
		newMesh.get()->need_normals();
		newMesh.get()->need_tstrips();

		// Store the trimesh for later
		built_TriMesh = newMesh;
	}

	else{ printf("\nObject Error: Cannot convert to TriMesh!"); assert(false); }

	return built_TriMesh;

} // end build trimesh


std::shared_ptr<TetMesh> ObjectMeta::as_TetMesh(){

	if( parse::to_lower(type) != "tetmesh" ){ printf("\nObject Error: Not type TetMesh!"); assert(false); }
	if( built_TetMesh != NULL ){ return built_TetMesh; }

	if( param_map.count("file")==0 ){ printf("\nTetMesh Error: No file specified"); assert(false); }

	std::string file = param_vec[ param_map[ "file" ] ].as_string();

	std::shared_ptr<TetMesh> newMesh( new TetMesh() );

	if( !newMesh.get()->load( file ) ){ assert(false); }
	newMesh.get()->apply_xform( x_form );

	// Store the trimesh for later
	built_TetMesh = newMesh;

	return newMesh;

} // end build tet mesh

