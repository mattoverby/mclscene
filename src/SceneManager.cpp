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

SceneManager::SceneManager() {
	root_bvh=NULL;
	createObject = default_build_object;
	createCamera = default_build_camera;
	createLight = default_build_light;
	createMaterial = default_build_material;
}


bool SceneManager::load( std::string filename ){

	//
	//	Load the XML file into mcl::Component
	//

	std::vector< Component > components;
	std::string xmldir = parse::fileDir( filename );
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	if( !result ){
		std::cerr << "\n**SceneManager::load_xml Error: Unable to load " << filename << std::endl;
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
		std::string tag = curr_node.name();
		if( name.size() == 0 || type.size() == 0 ){
			std::cerr << "\n**SceneManager::load_xml Error: Component \"" << curr_node.name() << "\" need a name and type." << std::endl;
			return false;
		}

		// Load the parameters
		std::vector<Param> params;
		{
			load_params( params, curr_node );
			// If any parameters are "file" or "texture" give it the path name from the current execution directory
			for( int i=0; i<params.size(); ++i ){
				if( parse::to_lower(params[i].tag) == "file" || parse::to_lower(params[i].tag) == "texture" ){
					params[i].value = xmldir + params[i].as_string();
				}
			}
		} // end load parameters

		// Create the component
		components.push_back( Component( tag, name, type ) );
		components.back().params = params;

	} // end loop scene info

	//
	//	Now we have a list of components, we can create them with the SceneManager
	//

	// Loop components and invoke callbacks
	for( int j=0; j<components.size(); ++j ){

		std::string tag = parse::to_lower(components[j].tag);
		std::string name = parse::to_lower(components[j].name);

		//	Build Camera
		if( tag == "camera" ){
			std::shared_ptr<BaseCamera> cam = createCamera( components[j] );
			if( cam != NULL ){
				cameras.push_back( cam );
				cameras_map[name] = cam;
			}
		} // end build Camera

		//	Build Light
		else if( tag == "light" ){
			std::shared_ptr<BaseLight> light = createLight( components[j] );
			if( light != NULL ){
				lights.push_back( light );
				lights_map[name] = light;
			}
		} // end build Light

		//	Build Material
		else if( tag == "material" ){
			std::shared_ptr<BaseMaterial> mat = createMaterial( components[j] );
			if( mat != NULL ){
				materials.push_back( mat );
				materials_map[name] = mat;
			}
		} // end build material

		//	Build Object
		else if( tag == "object" ){
			std::shared_ptr<BaseObject> obj = createObject( components[j] );
			if( obj != NULL ){
				objects.push_back( obj );
				objects_map[name] = obj;
			}
		} // end build object

	} // end loop components

	//
	//	Success, all done.
	//
	return true;
	
} // end load


void SceneManager::save( std::string xmlfile, int mode ){

	// I really should use pugixml instead of string, however that would
	// increase dependency usage more than I'd like (in the light/object/etc... classes).

	std::stringstream xml;
	std::cout << "Saving scene file: " << std::flush;

	if( mode == 0 ){

		xml << "<?xml version=\"1.0\"?>\n";
		xml << "<mclscene>";

		// Loop over objects
		for( int i=0; i<objects.size(); ++i ){
			std::stringstream obj_name; obj_name << "object" << i;
			xml << "\n" << objects[i]->get_xml( obj_name.str(), mode );
		}

		// Loop over materials, they must retain their original name
		std::unordered_map< std::string, std::shared_ptr<BaseMaterial> >::iterator mat_iter = materials_map.begin();
		for( mat_iter; mat_iter != materials_map.end(); ++mat_iter ){
			xml << "\n" << mat_iter->second->get_xml( mat_iter->first, mode );
		}

		// Loop over cameras
		for( int i=0; i<cameras.size(); ++i ){
			std::stringstream cam_name; cam_name << "camera" << i;
			xml << "\n" << cameras[i]->get_xml( cam_name.str(), mode );
		}

		// Loop over lights
		for( int i=0; i<lights.size(); ++i ){
			std::stringstream light_name; light_name << "light" << i;
			xml << "\n" << lights[i]->get_xml( light_name.str(), mode );
		}

		xml << "\n</mclscene>";

	} else {
		std::cerr << "SceneManager::save Error: I don't know that save type" << std::endl;
		return;
	}

	// Save to a file
	std::stringstream filename;
	filename << MCLSCENE_BUILD_DIR << "/" << parse::get_timestamp() << ".xml";
	std::cout << filename.str() << std::endl;
	std::ofstream filestream;
	filestream.open( xmlfile.c_str() );
	filestream << xml.str();
	filestream.close();

} // end save


void SceneManager::build_bvh( std::string split_mode ){

	if( root_bvh==NULL ){ root_bvh = std::shared_ptr<BVHNode>( new BVHNode() ); }
	else{ root_bvh.reset( new BVHNode() ); }

	if( split_mode == "spatial" ){
		int num_nodes = BVHBuilder::make_tree_spatial( root_bvh, objects );
	}

	else if( split_mode == "linear" ){
		int num_nodes = BVHBuilder::make_tree_lbvh( root_bvh, objects );
	}

	else{ std::cerr << "**SceneManager::build_bvh Error: I don't know BVH type \"" << split_mode << "\"" << std::endl; }

} // end build bvh


std::shared_ptr<BVHNode> SceneManager::SceneManager::get_bvh( bool recompute, std::string type ){
	if( recompute || root_bvh==NULL ){ build_bvh( parse::to_lower(type) ); }
	return root_bvh;
}


std::shared_ptr<mcl::BaseObject> SceneManager::make_object( std::string type, std::string name ){

	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "obj" << objects.size(); name = newname.str();
	}
	Component obj( "object", name, parse::to_lower(type) );
	std::shared_ptr<BaseObject> newObject = createObject( obj );
	// Add it to the SceneManager and return it
	objects.push_back( newObject );
	objects_map[name] = newObject;
	return newObject;

} // end make object


std::shared_ptr<mcl::BaseLight> SceneManager::make_light( std::string type, std::string name ){

	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "light" << lights.size(); name = newname.str();
	}
	Component obj( "light", name, parse::to_lower(type) );
	std::shared_ptr<BaseLight> newLight = createLight( obj );
	// Add it to the SceneManager and return it
	lights.push_back( newLight );
	lights_map[name] = newLight;
	return newLight;

} // end make light


std::shared_ptr<mcl::BaseCamera> SceneManager::make_camera( std::string type, std::string name ){

	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "camera" << lights.size(); name = newname.str();
	}
	Component obj( "camera", name, parse::to_lower(type) );
	std::shared_ptr<BaseCamera> newCam = createCamera( obj );
	// Add it to the SceneManager and return it
	cameras.push_back( newCam );
	cameras_map[name] = newCam;
	return newCam;

} // end make light


std::shared_ptr<mcl::BaseMaterial> SceneManager::make_material( std::string type, std::string name ){

	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "camera" << lights.size(); name = newname.str();
	}
	Component obj( "material", name, parse::to_lower(type) );
	std::shared_ptr<BaseMaterial> newMat = createMaterial( obj );
	// Add it to the SceneManager and return it
	materials.push_back( newMat );
	materials_map[name] = newMat;
	return newMat;

} // end make light
