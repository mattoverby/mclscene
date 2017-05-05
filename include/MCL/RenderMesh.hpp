// Copyright (c) 2017 University of Minnesota
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

#ifndef MCLSCENE_RENDERMESH_H
#define MCLSCENE_RENDERMESH_H 1

#include "MCL/Object.hpp"
#include "MCL/Material.hpp"
#include "MCL/Texture.hpp"
#include "TriMesh_algo.h" // mesh subdivision
#include "MCL/TriangleMesh.hpp"

namespace mcl {

//
//	A render mesh helps with interop between MCL and OpenGL
//
class RenderMesh {
public:
	static inline std::shared_ptr<RenderMesh> create(
		std::shared_ptr<BaseObject> obj_=NULL, std::shared_ptr<Material> mat_=NULL ){
		return std::shared_ptr<RenderMesh>( new RenderMesh(obj_,mat_) );
	}

	// You can still use RenderMeshes by self-managing the vertex/face pointers.
	RenderMesh() : RenderMesh(NULL,NULL) {}

	// Create a RenderMesh from an Object and Material
	RenderMesh( std::shared_ptr<BaseObject> obj_, std::shared_ptr<Material> mat_ );

	// Copy vertex data to GPU. Returns true if vertex data has successfully
	// been set from the stored Object pointer.
	inline bool load_buffers();

	// See if the current mesh should be invisible
	inline bool is_invisible() const { return object->flags & BaseObject::INVISIBLE; }

	// Vertex data
	float *vertices, *normals, *texcoords;
	int num_vertices, num_normals, num_texcoords;

	// Primitive data
	int *faces, *edges;
	int num_faces, num_edges;

	// OpenGL handles
	unsigned int verts_vbo, normals_vbo, texcoords_vbo, faces_ibo, wire_ibo, tris_vao;
	unsigned int tex_id;

	// Pointers to the original object and material
	std::shared_ptr<BaseObject> object;
	std::shared_ptr<Material> material;

private:
	std::unique_ptr<mcl::Texture> texture;
	inline bool update(); // update pointer data
	inline void subdivide_mesh( trimesh::TriMesh &tempmesh );
	inline void make_flat( TriangleMesh &tempmesh );
};


//
//	Implementation
//


// Create a RenderMesh from an Object and Material
inline RenderMesh::RenderMesh( std::shared_ptr<BaseObject> obj_, std::shared_ptr<Material> mat_ ) :
	vertices(0), normals(0), texcoords(0), faces(0), edges(0),
	num_vertices(0), num_normals(0), num_texcoords(0), num_faces(0), num_edges(0),
	verts_vbo(0), normals_vbo(0), texcoords_vbo(0), faces_ibo(0), wire_ibo(0), tris_vao(0), tex_id(0),
	object(obj_), material(mat_) {

	update();
	if( material != NULL ){
		if( material->app.texture.size() > 0 ){
			texture = std::unique_ptr<Texture>( new Texture() );
			texture->create_from_file( material->app.texture );
			tex_id = texture->handle(); // will be zero if file failed to load.
		}
	}

} // end constructor


inline bool RenderMesh::update(){

	// Get vertex and face pointers
	if( object == NULL ){ return false; }
	bool success = object->get_vertices( vertices, num_vertices, normals, num_normals, texcoords, num_texcoords );
	if( !success ){ num_vertices = 0; return false; }
	success = object->get_primitives( Prim::Tri, faces, num_faces );
	if( !success ){ num_faces = 0; return false; }
	object->get_primitives( Prim::Edge, edges, num_edges );

	return true;
}


inline bool RenderMesh::load_buffers(){

	// Update vertex pointers and double check we have valid data
	if( !update() ){ return false; }
	if( num_vertices<=0 || num_normals<=0 || num_faces<=0 ){ return false; }

	// These are temporary meshes that stored flat or subdivided
	// vertices and faces. Their data will be mapped to the GPU if needed.
	TriangleMesh flatmesh;
	trimesh::TriMesh subdivmesh;
	if( object->flags & BaseObject::SUBDIVIDE ){ subdivide_mesh( subdivmesh ); }
	else if( object->flags & BaseObject::FLAT ){ make_flat( flatmesh ); }

	// Now copy vertex and face data to GPU
	GLenum draw_mode = GL_STATIC_DRAW;
	if( object->flags & BaseObject::DYNAMIC ){ draw_mode = GL_DYNAMIC_DRAW; }
	size_t stride = sizeof(float)*3;

	if( !verts_vbo ){ // Create the buffer for vertices
		glGenBuffers(1, &verts_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_vertices*stride, vertices, draw_mode);
	} else { // Otherwise update
		glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
		glBufferSubData( GL_ARRAY_BUFFER, 0, num_vertices*stride, vertices );
	}

	if( !normals_vbo ){ // Create the buffer for normals
		glGenBuffers(1, &normals_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_normals*stride, normals, draw_mode);
	} else { // Otherwise update
		glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
		glBufferSubData( GL_ARRAY_BUFFER, 0, num_normals*stride, normals );
	}

	 // Create the buffer for tex coords, these won't change
	if( !texcoords_vbo ){
		glGenBuffers(1, &texcoords_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_texcoords*sizeof(float)*2, texcoords, GL_STATIC_DRAW);
	}

	// Create the buffer for indices, these won't change
	if( !faces_ibo ){
		glGenBuffers(1, &faces_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faces_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_faces*sizeof(int)*3, faces, GL_STATIC_DRAW);
	}

	if( !wire_ibo && ( object->flags & BaseObject::WIREFRAME ) ){
		glGenBuffers(1, &wire_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wire_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_edges*sizeof(int)*2, edges, GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create the VAO
	if( !tris_vao ){

		glGenVertexArrays(1, &tris_vao);
		glBindVertexArray(tris_vao);

		// location=0 is the vertex
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=1 is the normal
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=2 is the tex coord
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);

		// Done setting data for the vao
		glBindVertexArray(0);
	}

	return true;

} // end copy data to GPU


inline void RenderMesh::subdivide_mesh( trimesh::TriMesh &tempmesh ){

	// Copy vertex data to tempmesh
	tempmesh.vertices.clear(); tempmesh.vertices.reserve( num_vertices );
	tempmesh.faces.clear(); tempmesh.faces.reserve( num_faces );
	tempmesh.texcoords.clear(); tempmesh.texcoords.reserve( num_texcoords );
	for( int i=0; i<num_vertices; ++i ){ tempmesh.vertices.push_back( trimesh::vec( vertices[i*3+0], vertices[i*3+1], vertices[i*3+2] ) ); }
	for( int i=0; i<num_faces; ++i ){ tempmesh.faces.push_back( trimesh::TriMesh::Face( faces[i*3+0], faces[i*3+1], faces[i*3+2] ) ); }
	for( int i=0; i<num_texcoords; ++i ){ tempmesh.texcoords.push_back( trimesh::vec2( texcoords[i*2+0], texcoords[i*2+1] ) ); }

	// Do subdivision with trimesh
	trimesh::subdiv( &tempmesh ); // creates faces
	tempmesh.need_normals(true);

	// We will use the app data stored with that object.
	vertices = &tempmesh.vertices[0][0];
	normals = &tempmesh.normals[0][0];
	faces = &tempmesh.faces[0][0];
	texcoords = &tempmesh.texcoords[0][0];
	num_vertices = tempmesh.vertices.size();
	num_normals = tempmesh.normals.size();
	num_faces = tempmesh.faces.size();
	num_texcoords = tempmesh.texcoords.size();

} // end subdivide mesh


inline void RenderMesh::make_flat( TriangleMesh &tempmesh ){

	tempmesh.vertices.reserve( num_vertices );
	tempmesh.faces.reserve( num_faces );
	tempmesh.texcoords.reserve( num_texcoords );
	for( int i=0; i<num_faces; ++i ){
		Vec3i f( faces[i*3], faces[i*3+1], faces[i*3+2] );
		int v_idx = tempmesh.vertices.size();
		tempmesh.vertices.push_back( Vec3f( vertices[f[0]*3], vertices[f[0]*3+1], vertices[f[0]*3+2] ) );
		tempmesh.vertices.push_back( Vec3f( vertices[f[1]*3], vertices[f[1]*3+1], vertices[f[1]*3+2] ) );
		tempmesh.vertices.push_back( Vec3f( vertices[f[2]*3], vertices[f[2]*3+1], vertices[f[2]*3+2] ) );
		tempmesh.faces.push_back( Vec3i(v_idx,v_idx+1,v_idx+2) );
		if( num_texcoords ){
			tempmesh.texcoords.push_back( Vec2f( texcoords[f[0]*2], texcoords[f[0]*2+1] ) );
			tempmesh.texcoords.push_back( Vec2f( texcoords[f[1]*2], texcoords[f[1]*2+1] ) );
			tempmesh.texcoords.push_back( Vec2f( texcoords[f[2]*2], texcoords[f[2]*2+1] ) );
		}
	}
	tempmesh.need_normals(true);
	tempmesh.need_edges(true);

	// We will use the app data stored with that object.
	vertices = &tempmesh.vertices[0][0];
	normals = &tempmesh.normals[0][0];
	faces = &tempmesh.faces[0][0];
	texcoords = &tempmesh.texcoords[0][0];
	edges = &tempmesh.edges[0][0];
	num_vertices = tempmesh.vertices.size();
	num_normals = tempmesh.normals.size();
	num_faces = tempmesh.faces.size();
	num_texcoords = tempmesh.texcoords.size();
	num_edges = tempmesh.edges.size();

} // end make flat shading

} // end namespace mcl

#endif
