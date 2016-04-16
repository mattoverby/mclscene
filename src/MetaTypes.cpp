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

#include "MCL/MetaTypes.hpp"

using namespace mcl;
using namespace trimesh;


bool Component::load_params( const pugi::xml_node &curr_node ){

	pugi::xml_node::iterator param = curr_node.begin();
	for( param; param != curr_node.end(); ++param ) {
		pugi::xml_node curr_param = *param;

		std::string tag = parse::to_lower( curr_param.name() );
		std::string type_id = curr_param.attribute("type").value();
		std::string value = curr_param.attribute("value").value();
		Param newP( tag, value, type_id );

		if( tag == "xform" ){

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
			param_map[ tag ] = param_vec.size();
			param_vec.push_back( newP );	
		}

	} // end loop params

	return true;

}


Param Component::operator[]( const std::string tag ) const {
	std::unordered_map< std::string, int >::const_iterator it = param_map.find( parse::to_lower(tag) );
	assert( it != param_map.end() ); assert( it->second < param_vec.size() );
	return param_vec[ it->second ];
}

Param &Component::operator[]( const std::string tag ) {
	std::unordered_map< std::string, int >::const_iterator it = param_map.find( parse::to_lower(tag) );
	assert( it != param_map.end() ); assert( it->second < param_vec.size() );
	return param_vec[ it->second ];
}

Param Component::get( const std::string tag ) const {
	return this->operator[](tag);
}

void Component::get( const std::string tag, std::vector<mcl::Param> *p ) const {
	if( !exists(tag) ){ return; }
	for( int i=0; i<param_vec.size(); ++i ){
		if( param_vec[i].tag == parse::to_lower(tag) ){
			p->push_back( param_vec[i] );
		}
	}
}

void Component::add_param( const Param &p ){
	param_map[ p.tag ] = param_vec.size();
	param_vec.push_back( p );	
}

bool Component::exists( const std::string tag ) const{
	std::unordered_map< std::string, int >::const_iterator it = param_map.find( parse::to_lower(tag) );
	if( it != param_map.end() && it->second < param_vec.size() ){ return true; }
	return false;
}


bool CameraComponent::check_params(){

	if( param_map.count( "position" )==0 ){ printf("\nCamera Error: No vec3 position!"); return false; }
	if( param_map.count( "up" )==0 ){ printf("\nCamera Error: No vec3 up!"); return false; }
	if( param_map.count( "lookat" )==0 && param_map.count( "direction" )==0 ){ printf("\nCamera Error: No vec3 direction/lookat!"); return false; }
	if( param_map.count( "lookat" )>0 && param_map.count( "direction" )>0 ){ printf("\nCamera Error: Both direction and lookat set!"); return false; }

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

	return true;
}


bool MaterialComponent::check_params(){

	diffuse = vec(.5,.5,.5);
	specular = vec(0,0,0);
	exponent = 0;

	if( param_map.count( "diffuse" )>0 ){
		param_vec[ param_map[ "diffuse" ] ].fix_color();
		diffuse = param_vec[ param_map[ "diffuse" ] ].as_vec3();
	}

	if( param_map.count( "edges" )>0 ){
		param_vec[ param_map[ "edges" ] ].fix_color();
	}

	if( param_map.count( "specular" )>0 ){
		param_vec[ param_map[ "specular" ] ].fix_color();
		specular = param_vec[ param_map[ "specular" ] ].as_vec3();
	}

	if( param_map.count("exponent")>0 ){ exponent = param_vec[ param_map[ "exponent" ] ].as_double(); }

	return true;
}


bool LightComponent::check_params(){

	if( param_map.count( "intensity" )==0 ){ printf("\nLight Error: No vec3 intensity!"); return false; }

	if( type == "directional" ){
		if( param_map.count( "direction" )==0 ){ printf("\nLight Error: No vec3 direction!"); return false; }

		// Normalize direction
		param_vec[ param_map[ "direction" ] ].normalize();
		dir = param_vec[ param_map[ "direction" ] ].as_vec3();
	}
	else if( type == "point" ){
		if( param_map.count( "position" )==0 ){ printf("\nLight Error: No vec3 position!"); return false; }
		pos = param_vec[ param_map[ "position" ] ].as_vec3();
	}

	param_vec[ param_map[ "intensity" ] ].fix_color();
	intensity = param_vec[ param_map[ "intensity" ] ].as_vec3();

	return true;
}


bool ObjectComponent::check_params(){

	if( param_map.count( "material" )>0 ){ material = param_vec[ param_map[ "material" ] ].as_string(); }

	return true;
}


std::shared_ptr<BaseObject> ObjectComponent::as_object(){

	if( built_obj != NULL ){ return built_obj; }
	std::string ltype = parse::to_lower(type);

	if( ltype == "sphere" ){ built_obj = std::shared_ptr<BaseObject>( new Sphere() ); }
	else if( ltype == "box" ){ built_obj = std::shared_ptr<BaseObject>( new Box() ); }
	else if( ltype == "plane" ){ built_obj = std::shared_ptr<BaseObject>( new Plane() ); }
	else if( ltype == "trimesh" ){ built_obj = std::shared_ptr<BaseObject>( new TriangleMesh() ); }
	else if( ltype == "tetmesh" ){ built_obj = std::shared_ptr<BaseObject>( new TetMesh() ); }

	built_obj.get()->init( param_vec );
	built_obj.get()->apply_xform( x_form );

	return built_obj;
}


const std::shared_ptr<trimesh::TriMesh> ObjectComponent::get_TriMesh(){

	// If the object has not been created yet, do so
	if( built_obj == NULL ){ built_obj = as_object(); }
	return built_obj.get()->get_TriMesh();

} // end build trimesh

template<typename T> std::shared_ptr<T> ObjectComponent::get(){

	if( built_obj != NULL ){ return built_obj; }
	return std::dynamic_pointer_cast<T>(built_obj); // totally safe
}

