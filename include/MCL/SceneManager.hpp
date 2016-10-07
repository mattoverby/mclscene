// Copyright (c) 2016 University of Minnesota
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

#ifndef MCLSCENE_SCENEMANAGER_H
#define MCLSCENE_SCENEMANAGER_H 1

#include "BVH.hpp"
#include "DefaultBuilders.hpp"

//
//	Loading a scene with SceneManager:
//	1) Replace the build callbacks with your own methods, if needed.
//		See include/MCL/DefaultBuilders.hpp for details.
//	2) Load an XML file with SceneManager::load (see conf/ for examples).
//		OR you can use the make_object, make_light, make_camera, and make_material
//		functions. They return shared pointers of the scene component
//		and you can set the parameters directly.
//
namespace mcl {

class SceneManager {

	public:
		SceneManager();
		~SceneManager();

		//
		// Load a configuration file, can be called multiple times for different files.
		// Additional calls will add stuff to the scene.
		// Returns true on success.
		//
		bool load( std::string filename );

		//
		// Exports to a scene file. Mesh files are saved to the build directory.
		// Note that some of the original scene file information will be lost,
		// E.g. the original mesh after an xform is applied.
		// Mode is:
		//	0 = mclscene
		//
		void save( std::string xmlfile, int mode=0 );

		//
		// Computes bounding volume heirarchy (AABB). Eventually I will add better heuristics.
		// Type is:
		// 	spatial = object median (slower, better balanced)
		//	linear = parallel build w/ morton codes (probably has an error somewhere)
		//
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false, std::string type="spatial" );

		//
		// Computes a naive bounding sphere, excluding cameras and lights.
		// Calls each object's bounds function, so may be costly for dynamic scenes.
		//
		void get_bsphere( trimesh::vec *center, float *radius, bool recompute=false );

		//
		// For a given camera distance from scene center, add lights to make
		// a three-point lighting rig. Assumes +y is up and camera is facing -z.
		// Any previous lights are removed.
		// This is called by the Gui if no lighting has been added to the scene.
		//
		void make_3pt_lighting( const trimesh::vec &center, float distance );

		//
		// Scene components.
		// The components are indexed in listed/added order.
		// Note that BaseObject has a "get_material" function which returns an
		// index into the materials vector (-1 if no material was set).
		//
		std::vector< std::shared_ptr<BaseObject> > objects;
		std::vector< std::shared_ptr<BaseCamera> > cameras;
		std::vector< std::shared_ptr<BaseLight> > lights;
		std::vector< std::shared_ptr<BaseMaterial> > materials;

		//
		// In addition to creating the components, the original parameters parsed
		// from the XML file (if any) are stored as vectors. They are listed in order parsed.
		//
		std::vector< std::vector<Param> > object_params;
		std::vector< std::vector<Param> > camera_params;
		std::vector< std::vector<Param> > light_params;
		std::vector< std::vector<Param> > material_params;

		//
		// Creator functions that build a scene component and adds it to the
		// vectors above. Calls the builder callbacks.
		// If you want to make a preset material, see Material.hpp for make_preset_material.
		//
		std::shared_ptr<BaseObject> make_object( std::string type );
		std::shared_ptr<BaseLight> make_light( std::string type );
		std::shared_ptr<BaseCamera> make_camera( std::string type );
		std::shared_ptr<BaseMaterial> make_material( std::string type );

		//
		// Similar to the "make_<thing>" functions above, only returns derived types.
		// Uses dynamic_pointer_cast, and I haven't debugged them.
		//
		template<typename T> std::shared_ptr<T> make_object( std::string type );
		template<typename T> std::shared_ptr<T> make_light( std::string type );
		template<typename T> std::shared_ptr<T> make_camera( std::string type );
		template<typename T> std::shared_ptr<T> make_material( std::string type );

		//
		// Creator Callbacks, invoked on a "load" or "make_<thing>" call.
		// These can be changed to whatever. For more details, see include/MCL/DefaultBuilders.hpp
		//
		BuildObjCallback createObject;
		BuildCamCallback createCamera;
		BuildLightCallback createLight;
		BuildMatCallback createMaterial;

	protected:

		// Root bvh is created by build_bvh.
		// build_bvh is called by get_bvh.
		void build_bvh( std::string split_mode );
		std::shared_ptr<BVHNode> root_bvh;

		// Cached bounding sphere stats:
		float last_radius;
		trimesh::vec last_center;

}; // end class SceneManager

//
//	Static functions
//

template<typename T> std::shared_ptr<T> SceneManager::make_object( std::string type ){
	std::shared_ptr<mcl::BaseObject> o = make_object(type);
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( o );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
}


template<typename T> std::shared_ptr<T> SceneManager::make_camera( std::string type ){
	std::shared_ptr<mcl::BaseCamera> o = make_camera(type);
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( o );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
}

template<typename T> std::shared_ptr<T> SceneManager::make_light( std::string type ){
	std::shared_ptr<mcl::BaseLight> o = make_light(type);
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( o );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
}

template<typename T> std::shared_ptr<T> SceneManager::make_material( std::string type ){
	std::shared_ptr<mcl::BaseMaterial> o = make_material(type);
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( o );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
}

} // end namespace mcl

#endif
