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


} // end namespace scene

#endif
