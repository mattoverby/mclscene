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
	//	Plane, 2 or more triangles
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


	//
	//	Unknown
	//
	return NULL;

} // end object builder

//
//	Default Material Builder
//
static std::shared_ptr<BaseMaterial> default_build_material( Component &component ){

	if( parse::to_lower(component.type) == "diffuse" ){

		std::shared_ptr<DiffuseMaterial> mat( new DiffuseMaterial() );
		for( int i=0; i<component.params.size(); ++i ){
			if( parse::to_lower(component.params[i].tag)=="diffuse" || parse::to_lower(component.params[i].tag)=="color" ){
				component.params[i].fix_color();
				mat->diffuse=component.params[i].as_vec3();
			}
			if( parse::to_lower(component.params[i].tag)=="edges" ){
				component.params[i].fix_color();
				mat->edge_color=component.params[i].as_vec3();
			}
		}
		std::shared_ptr<BaseMaterial> new_mat( mat );
		return new_mat;

	} // end build diffuse

	else if( parse::to_lower(component.type) == "specular" ){

		std::shared_ptr<SpecularMaterial> mat( new SpecularMaterial() );
		for( int i=0; i<component.params.size(); ++i ){
			if( parse::to_lower(component.params[i].tag)=="diffuse" || parse::to_lower(component.params[i].tag)=="color" ){
				component.params[i].fix_color();
				mat->diffuse=component.params[i].as_vec3();
			}
			if( parse::to_lower(component.params[i].tag)=="specular" ){
				component.params[i].fix_color();
				mat->specular=component.params[i].as_vec3();
			}
			if( parse::to_lower(component.params[i].tag)=="edges" ){
				component.params[i].fix_color();
				mat->edge_color=component.params[i].as_vec3();
			}
			if( parse::to_lower(component.params[i].tag)=="shininess" || parse::to_lower(component.params[i].tag)=="exponent" ){ mat->shininess=component.params[i].as_double(); }
		}
		std::shared_ptr<BaseMaterial> new_mat( mat );
		return new_mat;

	} // end build specular

	//
	//	Unknown
	//
	return NULL;

} // end build material


static void load_params( std::vector<Param> &params, const pugi::xml_node &curr_node ){

	pugi::xml_node::iterator param = curr_node.begin();
	for( param; param != curr_node.end(); ++param ) {
		pugi::xml_node curr_param = *param;

		std::string tag = parse::to_lower( curr_param.name() );
		std::string type_id = curr_param.attribute("type").value();
		std::string value = curr_param.attribute("value").value();
		Param newP( tag, value, type_id );

		if( tag == "xform" ){

			std::stringstream ss( value );
			trimesh::vec v; ss >> v[0] >> v[1] >> v[2];

			trimesh::xform x_form;

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

			std::stringstream xf_ss; xf_ss << x_form;
			newP.value = xf_ss.str();
		}

		params.push_back( newP );	

	} // end loop params

} // end load parameters

} // end namespace mcl

#endif
