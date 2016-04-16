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

#include "MCL/SceneManager.hpp"

using namespace mcl;
using namespace trimesh;

trimesh::box3 SceneManager::get_bbox( bool recompute ){
	if( bbox.valid && !recompute ){ return bbox; }
	build_boundary();
	return bbox;
}


trimesh::TriMesh::BSphere SceneManager::get_bsphere( bool recompute ){
	if( bsphere.valid && !recompute ){ return bsphere; }
	build_boundary();
	return bsphere;
}


void SceneManager::build_boundary(){

	bbox.clear();
	bsphere.valid = false;
	Miniball<3,float> mb;

	for( int i=0; i<cameras.size(); ++i ){
		bbox += cameras[i].pos;
		mb.check_in(cameras[i].pos);
	}

	for( int i=0; i<lights.size(); ++i ){
		if( lights[i].type=="point" ){ bbox += lights[i].pos; mb.check_in(lights[i].pos); }
	}

	for( int i=0; i<objects.size(); ++i ){
		std::shared_ptr<BaseObject> obj = objects[i].as_object();
		const std::shared_ptr<trimesh::TriMesh> tmesh = obj.get()->get_TriMesh();

		for( int j=0; j<tmesh.get()->vertices.size(); ++j ){
			bbox += tmesh.get()->vertices[j];
		}
		mb.check_in( tmesh.get()->vertices.begin(), tmesh.get()->vertices.end() );
	}

	mb.build();
	bsphere.center = mb.center();
	bsphere.r = sqrt(mb.squared_radius());
	bsphere.valid = true;

}


bool SceneManager::load( std::string xmlfile ){

	std::string xmldir = parse::fileDir( xmlfile );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlfile.c_str());
	if( !result ){
		std::cerr << "\n**SceneManager Error: Unable to load " << xmlfile << std::endl;
		return false;
	}

	// Get the node that stores scene info
	pugi::xml_node head_node = doc.first_child();
	while( parse::to_lower(head_node.name()) != "mclscene" && head_node ){ head_node = head_node.next_sibling(); }

	// Now parse scene information
	for( pugi::xml_node::iterator main_node = head_node.begin(); main_node != head_node.end(); ++main_node ){

		pugi::xml_node curr_node = *main_node;
		std::string name = curr_node.attribute("name").as_string();
		std::string type = curr_node.attribute("type").as_string();
		if( name.size() == 0 || type.size() == 0 ){
			std::cerr << "\n**SceneManager Error: Component \"" << curr_node.name() << "\" need a name and type." << std::endl;
			return false;
		}

		//
		//	Parse Camera
		//
		if( parse::to_lower(curr_node.name()) == "camera" ){

			CameraComponent cam;
			cam.name = name;
			cam.type = type;

			// Set defaults
			cam.add_param( Param( "type", "perspective", "string" ) );
			cam.add_param( Param( "position", "0 0 0", "vec3" ) );
			cam.add_param( Param( "direction", "0 0 -1", "vec3" ) );
			cam.add_param( Param( "up", "0 1 0", "vec3" ) );

			// Load parameters
			if( !cam.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !cam.check_params() ){ printf("\n**SceneManager Error: Camera check_params\n"); return false; }
			camera_map[ name ] = cameras.size();
			cameras.push_back( cam );

		} // end parse camera

		//
		//	Parse Light
		//
		if( parse::to_lower(curr_node.name()) == "light" ){

			LightComponent light;
			light.name = name;
			light.type = type;

			// Set defaults
			light.add_param( Param( "type", "point", "string" ) );
			light.add_param( Param( "position", "0 0 0", "vec3" ) );
			light.add_param( Param( "intensity", "1 1 1", "vec3" ) );

			// Load parameters
			if( !light.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !light.check_params() ){ printf("\n**SceneManager Error: Light check_params\n"); return false; }
			light_map[ name ] = lights.size();
			lights.push_back( light );

		} // end parse light

		//
		//	Parse Material
		//
		if( parse::to_lower(curr_node.name()) == "material" ){

			MaterialComponent mat;
			mat.name = name;
			mat.type = type;

			// Set defaults
			mat.add_param( Param( "type", "diffuse", "string" ) );

			// Load parameters
			if( !mat.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !mat.check_params() ){ printf("\n**SceneManager Error: Material check_params\n"); return false; }
			material_map[ name ] = materials.size();
			materials.push_back( mat );

		} // end parse material

		//
		//	Parse Object
		//
		if( parse::to_lower(curr_node.name()) == "object" ){

			ObjectComponent object;
			object.name = name;
			object.type = type;

			// Set defaults
			object.add_param( Param( "type", "none", "string" ) );

			// Load parameters
			if( !object.load_params( curr_node ) ){ return false; }

			// If there is a file, append the full path
			if( object.param_map.count("file")>0 ){
				std::string file = object.param_vec[ object.param_map["file"] ].as_string();
				std::string full_path = xmldir + file;
				object.param_vec[ object.param_map["file"] ].value = full_path;
			}

			// Check the parameters and store the data
			if( !object.check_params() ){ printf("\n**SceneManager Error: Object check_params\n"); return false; }
			object_map[ name ] = objects.size();
			objects.push_back( object );

		} // end parse object

	} // end loop scene info

	return true;

} // end load xml file


