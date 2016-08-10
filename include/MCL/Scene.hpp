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

#ifndef MCLSCENE_SCENE_H
#define MCLSCENE_SCENE_H 1

#include "MCL/SceneManager.hpp"

namespace scene {


//
//	scene::make_object
//
static std::shared_ptr<mcl::BaseObject> make_object( mcl::SceneManager &scene, std::string type, std::string name="" ){

	using namespace mcl;
	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "obj" << scene.objects.size(); name = newname.str();
	}
	Component obj( "object", name, parse::to_lower(type) );
	std::shared_ptr<BaseObject> newObject = scene.createObject( obj );
	// Add it to the SceneManager and return it
	scene.objects.push_back( newObject );
	scene.objects_map[name] = newObject;
	return newObject;

} // end make object


//
//	scene::make_light
//
static std::shared_ptr<mcl::BaseLight> make_light( mcl::SceneManager &scene, std::string type, std::string name="" ){

	using namespace mcl;
	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "light" << scene.lights.size(); name = newname.str();
	}
	Component obj( "light", name, parse::to_lower(type) );
	std::shared_ptr<BaseLight> newLight = scene.createLight( obj );
	// Add it to the SceneManager and return it
	scene.lights.push_back( newLight );
	scene.lights_map[name] = newLight;
	return newLight;

} // end make light


//
//	scene::make_camera
//
static std::shared_ptr<mcl::BaseCamera> make_camera( mcl::SceneManager &scene, std::string type, std::string name="" ){

	using namespace mcl;
	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "camera" << scene.lights.size(); name = newname.str();
	}
	Component obj( "camera", name, parse::to_lower(type) );
	std::shared_ptr<BaseCamera> newCam = scene.createCamera( obj );
	// Add it to the SceneManager and return it
	scene.cameras.push_back( newCam );
	scene.cameras_map[name] = newCam;
	return newCam;

} // end make light


//
//	scene::make_material
//
static std::shared_ptr<mcl::BaseMaterial> make_material( mcl::SceneManager &scene, std::string type, std::string name="" ){

	using namespace mcl;
	if( name.size()==0 ){ // give it a name if it doesn't have one
		std::stringstream newname; newname << "camera" << scene.lights.size(); name = newname.str();
	}
	Component obj( "material", name, parse::to_lower(type) );
	std::shared_ptr<BaseMaterial> newMat = scene.createMaterial( obj );
	// Add it to the SceneManager and return it
	scene.materials.push_back( newMat );
	scene.materials_map[name] = newMat;
	return newMat;

} // end make light


//
//	scene::load_xml
//	Returns true on success
//
static bool load_xml( mcl::SceneManager &scene, std::string filename ){
	using namespace mcl;

	//
	//	Load the XML file into mcl::Component
	//

	std::vector< Component > components;
	std::string xmldir = parse::fileDir( filename );
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	if( !result ){
		std::cerr << "\n**Scene::load_xml Error: Unable to load " << filename << std::endl;
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
			std::cerr << "\n**Scene::load_xml Error: Component \"" << curr_node.name() << "\" need a name and type." << std::endl;
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
			std::shared_ptr<BaseCamera> cam = scene.createCamera( components[j] );
			if( cam != NULL ){
				scene.cameras.push_back( cam );
				scene.cameras_map[name] = cam;
			}
		} // end build Camera

		//	Build Light
		else if( tag == "light" ){
			std::shared_ptr<BaseLight> light = scene.createLight( components[j] );
			if( light != NULL ){
				scene.lights.push_back( light );
				scene.lights_map[name] = light;
			}
		} // end build Light

		//	Build Material
		else if( tag == "material" ){
			std::shared_ptr<BaseMaterial> mat = scene.createMaterial( components[j] );
			if( mat != NULL ){
				scene.materials.push_back( mat );
				scene.materials_map[name] = mat;
			}
		} // end build material

		//	Build Object
		else if( tag == "object" ){
			std::shared_ptr<BaseObject> obj = scene.createObject( components[j] );
			if( obj != NULL ){
				scene.objects.push_back( obj );
				scene.objects_map[name] = obj;
			}
		} // end build object

	} // end loop components

	//
	//	Success, all done.
	//
	return true;

} // end load xml

} // end namespace scene

#endif
