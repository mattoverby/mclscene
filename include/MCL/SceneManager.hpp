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

namespace mcl {

// Callback functions for building unique derived types in custom projects
typedef boost::function<std::shared_ptr<BaseCamera> ( std::string name, std::string type, std::vector<Param> &params )> BuildCamCallback;
typedef boost::function<std::shared_ptr<BaseObject> ( std::string name, std::string type, std::vector<Param> &params )> BuildObjCallback;
typedef boost::function<std::shared_ptr<BaseLight> ( std::string name, std::string type, std::vector<Param> &params )> BuildLightCallback;
typedef boost::function<std::shared_ptr<BaseMaterial> ( std::string name, std::string type, std::vector<Param> &params )> BuildMatCallback;

class SceneManager {

	public:
		// Load a configuration file, can be called multiple times for different files.
		// Additional calls will add (or replace) stuff to the scene.
		// If no builder callbacks have been added to the scene, everything is built as
		// a trimesh/tetmesh and default cam/light/material builders are used.
		// See DefaultBuilders.hpp for details.
		// Returns true on success
		bool load( std::string xmlfile );

		// Builder callbacks are executing on a call to load(...).
		void add_callback( BuildCamCallback cb ){ cam_builders.push_back( cb ); }
		void add_callback( BuildObjCallback cb ){ obj_builders.push_back( cb ); }
		void add_callback( BuildLightCallback cb ){ light_builders.push_back( cb ); }
		void add_callback( BuildMatCallback cb ){ mat_builders.push_back( cb ); }

		// Build all objects as a trimesh. I use this for OpenGL rendering of scenes.
		// Only objects that have the get_TriMesh() function are built this way.
		void build_meshes();
		std::vector< std::shared_ptr<trimesh::TriMesh> > meshes;

		// Builds bounding volume heirarchies for each individual object
		// (same indices as meshes) as well as the whole scene
		// (obtained through get_bvh).
		void build_bvh();
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false );
		std::vector< std::shared_ptr<BVHNode> > mesh_bvh;

		// Computes the world bounding if it hasn't already
		trimesh::TriMesh::BSphere get_bsphere( bool recompute=false );

		// Vectors of scene components created through the builder callbacks.
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
		std::shared_ptr<BVHNode> root_bvh;

		// Builder vectors
		std::vector< BuildCamCallback > cam_builders;
		std::vector< BuildObjCallback > obj_builders;
		std::vector< BuildLightCallback > light_builders;
		std::vector< BuildMatCallback > mat_builders;

		// Builds bounding sphere
		void build_boundary();
		trimesh::TriMesh::BSphere bsphere;

}; // end class SceneManager

} // end namespace mcl

#endif
