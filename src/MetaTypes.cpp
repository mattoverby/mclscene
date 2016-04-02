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


bool Parameters::load_params( const pugi::xml_node &curr_node ){

//	centralize[3]=0.f;

	pugi::xml_node::iterator param = curr_node.begin();
	for( param; param != curr_node.end(); ++param ) {
		pugi::xml_node curr_param = *param;

		std::string name = parse::to_lower( curr_param.name() );
		std::string type_id = curr_param.attribute("type").value();
		std::string value = curr_param.attribute("value").value();

		if( name == "xform" ){

			std::stringstream ss( value );
			vec v; ss >> v[0] >> v[1] >> v[2];

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
//			else if( parse::to_lower(type_id) == "centralize" ){
//				centralize[0]=v[0]; centralize[1]=v[1]; centralize[2]=v[2];
//				centralize[3]=1.f;
//			}
		}
		else{

			std::stringstream ss( value );
			if( type_id == "d" || type_id == "double" ){
				double val; ss >> val; dbl_vals[name].push_back(val); }
			else if( type_id == "f" || type_id == "float" ){
				float val; ss >> val; float_vals[name].push_back(val); }
			else if( type_id == "b" || type_id == "bool" ){
				bool val; ss >> val; bool_vals[name].push_back(val); }
			else if( type_id == "c" || type_id == "char" ){
				char val; ss >> val; char_vals[name].push_back(val); }
			else if( type_id == "i" || type_id == "int" ){
				int val; ss >> val; int_vals[name].push_back(val); }
			else if( type_id == "l" || type_id == "long" ){
				long val; ss >> val; long_vals[name].push_back(val); }
			else if( type_id == "Ss" || type_id == "string" ){
				std::string val; ss >> val; str_vals[name].push_back(val); }
			else if( type_id == "vec3" ){
				vec v; ss >> v[0] >> v[1] >> v[2]; vec3_vals[name].push_back(v); }
			else{ std::cerr << "\nParameters Error: Unknown type \"" << type_id << "\"" << std::endl; return false; }

		}

	} // end loop params

	return true;
}


bool CameraMeta::check_params(){

	if( p.vec3_vals.count( "position" )==0 ){ printf("Camera Error: No vec3 position!"); return false; }
	if( p.vec3_vals.count( "up" )==0 ){ printf("Camera Error: No vec3 up!"); return false; }
	if( p.vec3_vals.count( "lookat" )==0 && p.vec3_vals.count( "direction" )==0 ){ printf("Camera Error: No vec3 direction/lookat!"); return false; }
	if( p.str_vals.count( "type" )==0 ){ printf("Camera Error: No str type!"); return false; }
	if( p.vec3_vals.count( "lookat" )>0 && p.vec3_vals.count( "direction" )>0 ){ printf("Camera Error: Both direction lookat set!"); return false; }



	// Normalize directions
	if( p.vec3_vals.count( "direction" )>0 ){
		for( int i=0; i<p.vec3_vals["direction"].size(); ++i ){
			normalize( p.vec3_vals["direction"][i] );
		}
	}

	// Set members
	pos = p.vec3_vals["position"].back();
	type = p.str_vals["type"].back();

	if( p.vec3_vals.count( "lookat" )==0 ){
		dir = p.vec3_vals["direction"].back();
		lookat = pos+dir;
	}
	else if( p.vec3_vals.count( "direction" )==0 ){
		lookat = p.vec3_vals["lookat"].back();
		dir = lookat-pos; normalize(dir);
	}

	return true;
}


bool MaterialMeta::check_params(){

	if( p.str_vals.count( "type" )==0 ){ printf("Material Error: No str type!"); return false; }
	diffuse = vec(.5,.5,.5);
	specular = vec(0,0,0);
	exponent = 64;

	// Make color 0 to 1
	if( p.vec3_vals.count("diffuse")>0 ){
	for( int i=0; i<p.vec3_vals["diffuse"].size(); ++i ){
		vec c = p.vec3_vals[ "diffuse" ][i];
		for( int ci=0; ci<3; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
		if( c[0] > 1.0 || c[1] > 1.0 || c[2] > 1.0 ){
			for( int ci=0; ci<3; ++ci ){ c[ci]/=255.f; } // from 0-255 to 0-1
		}
		p.vec3_vals[ "diffuse" ][i]=c;
		diffuse = c;
	}}

	// Make color 0 to 1
	if( p.vec3_vals.count("specular")>0 ){
	for( int i=0; i<p.vec3_vals["specular"].size(); ++i ){
		vec c = p.vec3_vals[ "specular" ][i];
		for( int ci=0; ci<3; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
		if( c[0] > 1.0 || c[1] > 1.0 || c[2] > 1.0 ){
			for( int ci=0; ci<3; ++ci ){ c[ci]/=255.f; } // from 0-255 to 0-1
		}
		p.vec3_vals[ "specular" ][i]=c;
		specular = c;
	}}

	if( p.float_vals.count("exponent")>0 ){ exponent = p.float_vals["exponent"].back(); }

	return true;
}


bool LightMeta::check_params(){

	if( p.str_vals.count( "type" )==0 ){ printf("Light Error: No str type!"); return false; }
	if( p.vec3_vals.count( "intensity" )==0 ){ printf("Light Error: No vec3 intensity!"); return false; }

	if( type == "directional" ){
		if( p.vec3_vals.count( "direction" )==0 ){ printf("Light Error: No vec3 direction!"); return false; }

		// Normalize direction
		if( p.vec3_vals.count( "direction" )>0 ){
			for( int i=0; i<p.vec3_vals["direction"].size(); ++i ){
				normalize( p.vec3_vals["direction"][i] );
			}
		}
	}
	else if( type == "point" ){
		if( p.vec3_vals.count( "position" )==0 ){ printf("Light Error: No vec3 position!"); return false; }
	}

	// Make color 0 to 1
	for( int i=0; i<p.vec3_vals["intensity"].size(); ++i ){
		vec c = p.vec3_vals[ "intensity" ][i];
		for( int ci=0; ci<3; ++ci ){ if( c[ci]<0.f ){ c[ci]=0.f; } } // min zero
		if( c[0] > 1.0 || c[1] > 1.0 || c[2] > 1.0 ){
			for( int ci=0; ci<3; ++ci ){ c[ci]/=255.f; } // from 0-255 to 0-1
		}
		p.vec3_vals[ "intensity" ][i]=c;
	}

	// Set members
	pos = p.vec3_vals["position"].back();
	type = p.str_vals["type"].back();
	intensity = p.vec3_vals["intensity"].back();
	if( p.vec3_vals.count( "direction" )>0 ){ dir = p.vec3_vals["direction"].back(); }

	return true;
}


bool ObjectMeta::check_params(){

	if( p.str_vals.count( "type" )==0 ){ printf("Object Error: No str type!"); return false; }

	if( p.str_vals.count( "material" )>0 ){ material = p.str_vals["material"].back(); }

	// Set members
	type = p.str_vals["type"].back();

	return true;
}


std::shared_ptr<trimesh::TriMesh> ObjectMeta::as_TriMesh(){

	if( type != "TriMesh" ){ printf("Object Error: Not type TriMesh!"); return NULL; }
	if( built_TriMesh != NULL ){ return built_TriMesh; }

	if( p.str_vals.count("file")==0 ){ printf("TriMesh Error: No file specified"); return NULL; }

	std::string file = p.str_vals["file"].back();

	// Try to load the trimesh
	std::shared_ptr<trimesh::TriMesh> newMesh( trimesh::TriMesh::read( file.c_str() ) );
	if( !newMesh.get() ){ printf("Trimesh Error: Could not load %s", file.c_str() ); return NULL; }
	newMesh.get()->set_verbose(0);

	// Now clean the mesh
	trimesh::remove_unused_vertices( newMesh.get() );

	// Apply transformation matrix
	trimesh::apply_xform( newMesh.get(), p.x_form );

	// Create triangle strip for rendering
	newMesh.get()->need_normals();
	newMesh.get()->need_tstrips();

	// Store the trimesh for later
	built_TriMesh = newMesh;

	return newMesh;

} // end build trimesh


std::shared_ptr<TetMesh> ObjectMeta::as_TetMesh(){

	if( type != "TetMesh" ){ printf("Object Error: Not type TetMesh!"); return NULL; }
	if( built_TetMesh != NULL ){ return built_TetMesh; }

	if( p.str_vals.count("file")==0 ){ printf("TetMesh Error: No file specified"); return NULL; }

	std::string file = p.str_vals["file"].back();

	std::shared_ptr<TetMesh> newMesh( new TetMesh() );

	if( !newMesh.get()->load( file ) ){ return NULL; }
	newMesh.get()->apply_xform( p.x_form );

	// Store the trimesh for later
	built_TetMesh = newMesh;

	return newMesh;

} // end build tet mesh

