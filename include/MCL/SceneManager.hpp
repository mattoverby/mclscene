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

#ifndef MCLSCENE_SCENEMANAGER_H
#define MCLSCENE_SCENEMANAGER_H 1

#include "bsphere.h" // in trimesh2
#include "BVH.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "DefaultBuilders.hpp"
#include <functional>

//
//	Creating a scene through SceneManager is done in three steps:
//	1) Add builder callbacks to the SceneManager (optional).
//	2) Create the scene via component containers (see Param.hpp). This can be done
//	   programatically or by a call to load(...).
//	3) If load was not called with auto_build=true, call build_components().
//
//
namespace mcl {

//
// Callback funcions are invoked on a call to build_components().
// See details in that function's doc. Note that these functions CAN change the component itself.
//
typedef std::function<std::shared_ptr<BaseCamera> ( Component &component )> BuildCamCallback;
typedef std::function<std::shared_ptr<BaseObject> ( Component &component )> BuildObjCallback;
typedef std::function<std::shared_ptr<BaseLight> ( Component &component )> BuildLightCallback;
typedef std::function<std::shared_ptr<BaseMaterial> ( Component &component )> BuildMatCallback;

class SceneManager {

	public:
		SceneManager();

		//
		// Load a configuration file, can be called multiple times for different files.
		// Additional calls will add (or replace) stuff to the scene.
		// This fills the components member data. If auto_build is true, build_components
		// is called after the file is parsed.
		// Returns true on success
		//
		bool load( std::string xmlfile, bool auto_build=true );

		//
		// Exports to a scene file. Mesh files are saved to the build directory.
		// Note that some of the original scene file information will be lost
		// (i.e. names, transformations)
		// Mode is:
		//	0 = mclscene
		//
		void save( std::string xmlfile, int mode=0 );

		//
		// Computes bounding volume heirarchy (AABB)
		// Type is either spatial (object median) or linear
		//
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false, std::string type="linear" );

		//
		// The scene is a list of components (e.g. Object, Light, etc...)
		// This vector is filled on a load(...) call, or you can add to it manually.
		// When you call build_components, this vector is looped over and the callbacks
		// are invoked.
		//
		std::vector< Component > components;
		Component &get( std::string name );
		Component &operator[]( std::string name ){ return get(name); }
		bool exists( std::string name ) const;

		//
		// Invokes the callbacks while looping over the components vector.
		// Returns true on success.
		//
		bool build_components();

		//
		// Builder callbacks are executed on a call to build_components().
		// If build_components is called and the builder vectors are empty, default
		// ones are added. See DefaultBuilders.hpp.
		//
		void add_callback( BuildCamCallback cb ){ cam_builders.push_back( cb ); createCamera = cb; }
		void add_callback( BuildObjCallback cb ){ obj_builders.push_back( cb ); createObject = cb; }
		void add_callback( BuildLightCallback cb ){ light_builders.push_back( cb ); createLight = cb; }
		void add_callback( BuildMatCallback cb ){ mat_builders.push_back( cb ); createMaterial = cb; }
		BuildObjCallback createObject;
		BuildCamCallback createCamera;
		BuildLightCallback createLight;
		BuildMatCallback createMaterial;

		//
		// Scene components returned from the builder callbacks.
		//
		std::vector< std::shared_ptr<BaseObject> > objects;
		std::vector< std::shared_ptr<BaseMaterial> > materials;
		std::vector< std::shared_ptr<BaseCamera> > cameras;
		std::vector< std::shared_ptr<BaseLight> > lights;
		std::unordered_map< std::string, std::shared_ptr<BaseObject> > objects_map; // name -> object
		std::unordered_map< std::string, std::shared_ptr<BaseMaterial> > materials_map; // name -> material
		std::unordered_map< std::string, std::shared_ptr<BaseCamera> > cameras_map; // name -> camera
		std::unordered_map< std::string, std::shared_ptr<BaseLight> > lights_map; // name -> light

		//
		// Vector of trimeshes for objects that have the get_TriMesh() function,
		// filled by the build_meshes() function which is called by build_components().
		// I use this for OpenGL rendering of scenes.
		//
		std::vector< std::shared_ptr<trimesh::TriMesh> > meshes;

	protected:
		// Root bvh is created by build_bvh
		void build_bvh( int split_mode ); // 0=object median, 1=linear (parallel)
		std::shared_ptr<BVHNode> root_bvh;

		// Builder vectors
		void build_meshes(); // fills the meshes vector, called by build_components
		bool objects_built;
		std::vector< BuildCamCallback > cam_builders;
		std::vector< BuildObjCallback > obj_builders;
		std::vector< BuildLightCallback > light_builders;
		std::vector< BuildMatCallback > mat_builders;

}; // end class SceneManager

} // end namespace mcl

#endif
