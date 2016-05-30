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

#ifndef MCLSCENE_DEFAULTBUILDERS_H
#define MCLSCENE_DEFAULTBUILDERS_H 1

#include "TetMesh.hpp"
#include "TriangleMesh.hpp"
#include "Material.hpp"
#include "../../deps/pugixml/pugixml.hpp"

namespace mcl {

//
//	Default Object Builder: Everything is a trimesh or tetmesh.
//
static std::shared_ptr<BaseObject> default_build_object( Component &obj ){

	using namespace trimesh;
	std::string type = parse::to_lower(obj.type);
	std::string name = obj.name;

	//
	//	First build the transform and other common params
	//
	xform x_form;
	std::string material = "";
	for( int i=0; i<obj.params.size(); ++i ){
		if( parse::to_lower(obj.params[i].tag)=="xform" ){
			x_form = x_form * obj.params[i].as_xform();
		}
		else if( parse::to_lower(obj.params[i].tag)=="material" ){
			material = obj.params[i].as_string();
		}
	}
	

	//
	//	Sphere
	//
	if( type == "sphere" ){

		std::shared_ptr<TriMesh> tris( new TriMesh() );

		double radius = 1.0;
		vec center(0,0,0);
		int tessellation = 1;

		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="radius" ){ radius=obj.params[i].as_double(); }
			else if( parse::to_lower(obj.params[i].tag)=="center" ){ center=obj.params[i].as_vec3(); }
			else if( parse::to_lower(obj.params[i].tag)=="tess" ){ tessellation=obj.params[i].as_int(); }
		}

		make_sphere_polar( tris.get(), tessellation, tessellation );

		// Now scale it by the radius
		xform s_xf = trimesh::xform::scale(radius,radius,radius);
		apply_xform(tris.get(), s_xf);

		// Translate so center is correct
		xform t_xf = trimesh::xform::trans(center[0],center[1],center[2]);
		apply_xform(tris.get(), t_xf);

		tris.get()->need_normals();
		tris.get()->need_tstrips();
		std::shared_ptr<BaseObject> new_obj( new mcl::TriangleMesh(tris,material) );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build sphere


	//
	//	Box
	//
	else if( type == "box" ){

		std::shared_ptr<TriMesh> tris( new TriMesh() );

		vec boxmin(-1,-1,-1); vec boxmax(1,1,1);
		int tessellation=1;
		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="boxmin" ){ boxmin=obj.params[i].as_vec3(); }
			else if( parse::to_lower(obj.params[i].tag)=="boxmax" ){ boxmax=obj.params[i].as_vec3(); }
			else if( parse::to_lower(obj.params[i].tag)=="tess" ){ tessellation=obj.params[i].as_int(); }
		}

		tris = std::shared_ptr<trimesh::TriMesh>( new trimesh::TriMesh() );

		// First create a boring cube
		trimesh::make_cube( tris.get(), tessellation ); // tess=1 -> 12 tris
		tris.get()->need_bbox();

		// Now translate it so boxmins are the same
		trimesh::vec offset = tris.get()->bbox.min - boxmin;
		trimesh::xform t_xf = trimesh::xform::trans(offset[0],offset[1],offset[2]);
		trimesh::apply_xform(tris.get(), t_xf);
		tris.get()->bbox.valid = false;
		tris.get()->need_bbox();

		// Now scale so that boxmaxes are the same
		trimesh::vec size = tris.get()->bbox.max - boxmax;
		trimesh::xform s_xf = trimesh::xform::scale(size[0],size[1],size[2]);
		trimesh::apply_xform(tris.get(), s_xf);
		tris.get()->bbox.valid = false;
		tris.get()->need_bbox();

		tris.get()->need_normals();
		tris.get()->need_tstrips();
		std::shared_ptr<BaseObject> new_obj( new mcl::TriangleMesh(tris,material) );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build box


	//
	//	Plane, 2 or more triangles
	//
	else if( type == "plane" ){

		std::shared_ptr<TriMesh> tris( new TriMesh() );

		int width = 10;
		int length = 10;
		double noise = 0.0;

		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="width" ){ width=obj.params[i].as_int(); }
			else if( parse::to_lower(obj.params[i].tag)=="length" ){ length=obj.params[i].as_int(); }
			else if( parse::to_lower(obj.params[i].tag)=="noise" ){ noise=obj.params[i].as_double(); }
		}

		make_sym_plane( tris.get(), width, length );
		if( noise > 0.0 ){ trimesh::noisify( tris.get(), noise ); }

		tris.get()->need_normals();
		tris.get()->need_tstrips();
		std::shared_ptr<BaseObject> new_obj( new mcl::TriangleMesh(tris,material) );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build plane


	//
	//	Beam
	//
	else if( type == "beam" ){

		std::shared_ptr<TriMesh> tris( new TriMesh() );

		int tess = 3;
		int chunks = 5;

		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="tess" ){ tess=obj.params[i].as_int(); }
			else if( parse::to_lower(obj.params[i].tag)=="chunks" ){ chunks=obj.params[i].as_int(); }
		}


		make_beam( tris.get(), tess, chunks );

		tris.get()->need_normals();
		tris.get()->need_tstrips();
		std::shared_ptr<BaseObject> new_obj( new mcl::TriangleMesh(tris,material) );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build beam

	//
	//	Cylinder
	//
	else if( type == "cylinder" ){

		std::shared_ptr<TriMesh> tris( new TriMesh() );

		float radius = 1.f;
		int tess_l=10, tess_c=10;

		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="tess_l" ){ tess_l=obj.params[i].as_int(); }
			if( parse::to_lower(obj.params[i].tag)=="tess_c" ){ tess_c=obj.params[i].as_int(); }
			else if( parse::to_lower(obj.params[i].tag)=="radius" ){ radius=obj.params[i].as_float(); }
		}

		trimesh::make_ccyl( tris.get(), tess_l, tess_c, radius );
		tris.get()->need_normals();
		tris.get()->need_tstrips();
		std::shared_ptr<BaseObject> new_obj( new mcl::TriangleMesh(tris,material) );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build cylinder

	//
	//	Triangle Mesh, 2 or more triangles
	//
	else if( type == "trimesh" ){

		std::shared_ptr<TriMesh> tris( new TriMesh() );
		tris->set_verbose(0);

		std::string filename = "";
		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="file" ){ filename=obj.params[i].as_string(); }
		}
		if( !filename.size() ){ printf("\nTriangleMesh Error for obj %s: No file specified", name.c_str()); assert(false); } 

		// Try to load the trimesh
		tris.reset( trimesh::TriMesh::read( filename.c_str() ) );
		if( tris == NULL ){
			printf("\nnTriangleMesh Error for obj %s: failed to load file %s", name.c_str(), filename.c_str()); assert(false);
		}

		// Now clean the mesh
		remove_unused_vertices( tris.get() );

		tris.get()->need_normals();
		tris.get()->need_tstrips();
		std::shared_ptr<BaseObject> new_obj( new mcl::TriangleMesh(tris,material) );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build trimesh


	//
	//	Tet Mesh
	//
	else if( type == "tetmesh" ){

		std::shared_ptr<TetMesh> mesh( new TetMesh(material) );
		std::string filename = "";
		for( int i=0; i<obj.params.size(); ++i ){
			if( parse::to_lower(obj.params[i].tag)=="file" ){ filename=obj.params[i].as_string(); }
		}
		if( !filename.size() ){ printf("\nTetMesh Error for obj %s: No file specified", name.c_str()); assert(false); }
		if( !mesh->load( filename ) ){ printf("\nTetMesh Error for obj %s: failed to load file %s", name.c_str(), filename.c_str()); assert(false); }
		mesh->need_normals();
		std::shared_ptr<BaseObject> new_obj( mesh );
		new_obj->apply_xform( x_form );
		return new_obj;

	} // end build tet mesh

	else{
		std::cerr << "**Error: I don't know how to create an object of type " << type << std::endl;
	}


	//
	//	Unknown
	//
	return NULL;

} // end object builder

//
//	Default Material Builder
//
static std::shared_ptr<BaseMaterial> default_build_material( Component &component ){

	std::string type = parse::to_lower(component.type);
	std::string name = component.name;

	//
	//	OpenGL Materials
	//
//	if( type == "ogl" ){
	{

		std::shared_ptr<OGLMaterial> mat( new OGLMaterial() );
		for( int i=0; i<component.params.size(); ++i ){
			if( parse::to_lower(component.params[i].tag)=="diffuse" || parse::to_lower(component.params[i].tag)=="color" ){
				component.params[i].fix_color();
				mat->diffuse=component.params[i].as_vec3();
			}
			else if( parse::to_lower(component.params[i].tag)=="specular" ){
				component.params[i].fix_color();
				mat->specular=component.params[i].as_vec3();
			}
			else if( parse::to_lower(component.params[i].tag)=="texture" ){
				mat->m_texture=TextureResource( component.name, component.params[i].as_string() );
			}
			else if( parse::to_lower(component.params[i].tag)=="edges" ){
				component.params[i].fix_color();
				mat->edge_color=component.params[i].as_vec3();
			}
			else if( parse::to_lower(component.params[i].tag)=="shininess" || parse::to_lower(component.params[i].tag)=="exponent" ){ mat->shininess=component.params[i].as_int(); }
		}
		std::shared_ptr<BaseMaterial> new_mat( mat );
		return new_mat;
	}

	//
	//	Unknown
	//
	return NULL;

} // end build material


//
//	Default Light Builder
//
static std::shared_ptr<BaseLight> default_build_light( Component &component ){

	std::string type = parse::to_lower(component.type);
	std::string name = component.name;

//	int m_type;
//	trimesh::vec m_pos;
//	trimesh::vec m_ambient, m_diffuse, m_specular; // colors


	//
	//	OpenGL Light
	//
//	if( type == "ogl" ){
	{
		std::shared_ptr<OGLLight> light( new OGLLight() );
		for( int i=0; i<component.params.size(); ++i ){
			if( parse::to_lower(component.params[i].tag)=="diffuse" ){
				component.params[i].fix_color();
				light->m_diffuse=component.params[i].as_vec3();
			}
			else if( parse::to_lower(component.params[i].tag)=="ambient" ){
				component.params[i].fix_color();
				light->m_ambient=component.params[i].as_vec3();
			}
			else if( parse::to_lower(component.params[i].tag)=="specular" ){
				component.params[i].fix_color();
				light->m_specular=component.params[i].as_vec3();
			}
			else if( parse::to_lower(component.params[i].tag)=="position" ){
				light->m_pos=component.params[i].as_vec3();
				light->m_type=1; // point light
			}
			else if( parse::to_lower(component.params[i].tag)=="direction" ){
				light->m_pos=component.params[i].as_vec3();
				light->m_type=0; // directional light
			}
		}
		std::shared_ptr<BaseLight> new_light( light );
		return new_light;
	}

	//
	//	Unknown
	//
	return NULL;

} // end build light


} // end namespace mcl

#endif
