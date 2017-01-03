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

#ifndef MCLSCENE_RAYINTERSECT_H
#define MCLSCENE_RAYINTERSECT_H 1

#include <memory>
#include "MCL/Vec3.hpp"
#include "MCL/Projection.hpp"

namespace mcl {

namespace intersect {

	class Ray {
	public:
		Ray(){ direction=Vec3d(0,0,-1); eps=1e-5f; }
		Ray( Vec3d o, Vec3d d, float e=1e-5f ){
			origin=o; direction=d; eps=e;
			origin += direction*eps;
		}
		Vec3d origin, direction;
		float eps;
	};

	class Payload {
	public:
		Payload( const Ray *ray ){ t_min=ray->eps; t_max=9999999.0; launch_point=ray->origin; }
		double t_min, t_max;
		Vec3d n, hit_point, launch_point;
		int material; // index into SceneManager::materials
	};

	static inline Vec3d reflect( const Vec3d incident, const Vec3d norm ){
		return ( incident - 2.f * norm * norm.dot( incident ) );
	}

	// ray -> triangle without early exit
	static inline bool ray_triangle( const Ray *ray, const Vec3d &p0, const Vec3d &p1, const Vec3d &p2,
		const Vec3d &n0, const Vec3d &n1, const Vec3d &n2, Payload *payload );

	// ray -> axis aligned bounding box
	// Returns true/false only and does not set the payload.
	static inline bool ray_aabb( const Ray *ray, const Vec3d &min, const Vec3d &max, const Payload *payload );

	// Squared distance from point to aabb
	static inline double point_aabb( const Vec3d &point, const Vec3d &min, const Vec3d &max );

	// Point-on-triangle test: returns projection on to triangle surface
	static inline mcl::Vec3d point_triangle( const Vec3d &point, const Vec3d &p0, const Vec3d &p1, const Vec3d &p2 );

} // end namespace intersect

} // end namespace mcl

//
//	Implementation below
//

// ray -> triangle without early exit
static inline bool mcl::intersect::ray_triangle( const Ray *ray, const Vec3d &p0, const Vec3d &p1, const Vec3d &p2,
	const Vec3d &n0, const Vec3d &n1, const Vec3d &n2, Payload *payload ){

	const Vec3d e0 = p1 - p0;
	const Vec3d e1 = p0 - p2;
	const Vec3d n = e1.cross( e0 );

	const Vec3d e2 = ( 1.0f / n.dot( ray->direction ) ) * ( p0 - ray->origin );
	const Vec3d i  = ray->direction.cross( e2 );

	float beta  = i.dot( e1 );
	float gamma = i.dot( e0 );
	float alpha = 1.f - beta - gamma;

	float t = n.dot( e2 );
	bool hit = ( (t<payload->t_max) & (t>payload->t_min) & (beta>=-ray->eps*0.5f) & (gamma>=-ray->eps*0.5f) & (beta+gamma<=1.f) );

	if( hit ){
		payload->n = alpha*n0 + beta*n1 + gamma*n2;
		payload->t_max = t;
		payload->hit_point = ray->origin + ray->direction*t;
		return true;
	}

	return false;

} // end  ray -> triangle

// ray -> axis aligned bounding box
static inline bool mcl::intersect::ray_aabb( const Ray *ray, const Vec3d &min, const Vec3d &max, const Payload *payload ){

	float txmin=0.f, txmax=0.f;
	float dirX = 1.f / ray->direction[0];
	if( dirX >= 0.0 ){
		txmin = dirX * ( min[0] - ray->origin[0] );
		txmax = dirX * ( max[0] - ray->origin[0] );
	}
	else{
		txmax = dirX * ( min[0] - ray->origin[0] );
		txmin = dirX * ( max[0] - ray->origin[0] );
	}

	float tymin=0.f, tymax=0.f;
	float dirY = 1.f / ray->direction[1];
	if( ray->direction[1] >= 0.0 ){
		tymin = dirY * ( min[1] - ray->origin[1] );
		tymax = dirY * ( max[1] - ray->origin[1] );
	}
	else{
		tymax = dirY * ( min[1] - ray->origin[1] );
		tymin = dirY * ( max[1] - ray->origin[1] );
	}

	// First check: x/y axis
	if( txmin > tymax || tymin > txmax ){ return false; }

	float tzmin=0.f, tzmax=0.f;
	float dirZ = 1.f / ray->direction[2];
	if( ray->direction[2] >= 0.0 ){
		tzmin = dirZ * ( min[2] - ray->origin[2] );
		tzmax = dirZ * ( max[2] - ray->origin[2] );
	}
	else{
		tzmax = dirZ * ( min[2] - ray->origin[2] );
		tzmin = dirZ * ( max[2] - ray->origin[2] );
	}

	// Second check: z axis
	if( txmin > tzmax || tzmin > txmax ){ return false; }
	if( tymin > tzmax || tzmin > tymax ){ return false; }

	return true;

} // end ray box intersection


static inline double mcl::intersect::point_aabb( const Vec3d &point, const Vec3d &min, const Vec3d &max ){
	float sqDist=0.f;
	for( int i=0; i<3; ++i ){
		if( point[i] < min[i] ){ sqDist += (min[i]-point[i])*(min[i]-point[i]); }
		if( point[i] > max[i] ){ sqDist += (point[i]-max[i])*(point[i]-max[i]); }
	}
	return sqDist;
}


static inline mcl::Vec3d mcl::intersect::point_triangle( const Vec3d &point, const Vec3d &p0, const Vec3d &p1, const Vec3d &p2 ){
	Vec3d tripoints[3] = { p0, p1, p2 };
	Vec3d proj = mcl::Projection::Triangle( tripoints, point );
	return Vec3d( proj[0], proj[1], proj[2] );
}


#endif
