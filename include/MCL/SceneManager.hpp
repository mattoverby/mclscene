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
#include <boost/function.hpp>
#include "Camera.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include "DefaultBuilders.hpp"


//
//	Creating a scene through SceneManager is done in three steps:
//	1) Add builder callbacks to the SceneManager.
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
typedef boost::function<std::shared_ptr<BaseCamera> ( Component &component )> BuildCamCallback;
typedef boost::function<std::shared_ptr<BaseObject> ( Component &component )> BuildObjCallback;
typedef boost::function<std::shared_ptr<BaseLight> ( Component &component )> BuildLightCallback;
typedef boost::function<std::shared_ptr<BaseMaterial> ( Component &component )> BuildMatCallback;
typedef boost::function<void( Component &component )> BuildCallback; // generic builder callback for unknown things

class SceneManager {

	public:
		SceneManager() { root_bvh=NULL; bsphere.r=0.f; }

		//
		// Load a configuration file, can be called multiple times for different files.
		// Additional calls will add (or replace) stuff to the scene.
		// This fills the components member data. If auto_build is true, build_components
		// is called after the file is parsed.
		// Returns true on success
		//
		bool load( std::string xmlfile, bool auto_build=true );

		//
		// Computes bounding volume heirarchy (AABB)
		// Type is either spatial or linear
		//
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false, std::string type="spatial" );

		//
		// Computes the world bounding sphere
		//
		trimesh::TriMesh::BSphere get_bsphere( bool recompute=false );

		//
		// The scene is a list of components (e.g. Object, Light, etc...)
		// This vector is filled on a load(...) call, or you can add to it manually.
		// When you call build_components, this vector is looped over and the callbacks
		// are invoked.
		//
		Component &get( std::string name );
		Component &operator[]( std::string name ){ return get(name); }
		bool exists( std::string name ) const;
		std::vector< Component > components;

		//
		// Invokes the callbacks while looping over the components vector.
		// Returns true on success.
		//
		bool build_components();

		//
		// Build all objects as a trimesh. I use this for OpenGL rendering of scenes.
		// Only objects that have the get_TriMesh() function are built this way.
		//
		void build_meshes();
		std::vector< std::shared_ptr<trimesh::TriMesh> > meshes;

		//
		// Builder callbacks are executing on a call to build_components()
		//
		void add_callback( BuildCamCallback cb ){ cam_builders.push_back( cb ); }
		void add_callback( BuildObjCallback cb ){ obj_builders.push_back( cb ); }
		void add_callback( BuildLightCallback cb ){ light_builders.push_back( cb ); }
		void add_callback( BuildMatCallback cb ){ mat_builders.push_back( cb ); }
		void add_callback( BuildCallback cb ){ builders.push_back( cb ); }

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

	protected:
		// Root bvh is created by build_bvh
		void build_bvh( int split_mode=1 ); // 0=spatial, 1=linear
		std::shared_ptr<BVHNode> root_bvh;

		// Builder vectors
		bool objects_built;
		std::vector< BuildCamCallback > cam_builders;
		std::vector< BuildObjCallback > obj_builders;
		std::vector< BuildLightCallback > light_builders;
		std::vector< BuildMatCallback > mat_builders;
		std::vector< BuildCallback > builders;

		// Builds bounding sphere
		void build_boundary();
		trimesh::TriMesh::BSphere bsphere;

}; // end class SceneManager

} // end namespace mcl

#endif
