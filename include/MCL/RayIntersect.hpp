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

#ifndef MCLSCENE_RAYINTERSECT_H
#define MCLSCENE_RAYINTERSECT_H 1

#include <memory>
#include "MCL/Projection.hpp"

namespace mcl {

//
//	TODO: clean up types
//

namespace intersect {

	class Ray {
	public:
		Ray(){ direction=Vec3f(0,0,-1); eps=1e-5f; }
		Ray( Vec3f o, Vec3f d, double e=1e-5f ){
			origin=o; direction=d; eps=e;
			origin += direction*eps;
		}
		Vec3f origin, direction;
		double eps;
	};

	class Payload {
	public:
		Payload( const Ray *ray ){ t_min=ray->eps; t_max=9999999.0; launch_point=ray->origin; }
		double t_min, t_max;
		Vec3f n, hit_point, launch_point;
		int material; // index into SceneManager::materials
	};

	static inline Vec3f reflect( const Vec3f incident, const Vec3f norm ){
		return ( incident - 2.f * norm * norm.dot( incident ) );
	}

	// ray -> triangle without early exit
	static inline bool ray_triangle( const Ray *ray, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2,
		const Vec3f &n0, const Vec3f &n1, const Vec3f &n2, Payload *payload );

	// ray -> triangle without smoothed normals
	static inline bool ray_triangle( const Ray *ray, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2, Payload *payload );

	// ray -> axis aligned bounding box
	// Returns true/false only and does not set the payload.
	static inline bool ray_aabb( const Ray *ray, const Vec3f &min, const Vec3f &max, const Payload *payload );

	// Squared distance from point to aabb
	static inline double point_aabb( const Vec3f &point, const Vec3f &min, const Vec3f &max );

	// Point-on-triangle test: returns projection on to triangle surface
	static inline mcl::Vec3f point_triangle( const Vec3f &point, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2 );

} // end namespace intersect

} // end namespace mcl

//
//	Implementation below
//

// ray -> triangle without early exit
static inline bool mcl::intersect::ray_triangle( const Ray *ray, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2,
	const Vec3f &n0, const Vec3f &n1, const Vec3f &n2, Payload *payload ){

	const Vec3f e0 = p1 - p0;
	const Vec3f e1 = p0 - p2;
	const Vec3f n = e1.cross( e0 );

	const Vec3f e2 = ( 1.0f / n.dot( ray->direction ) ) * ( p0 - ray->origin );
	const Vec3f i  = ray->direction.cross( e2 );

	double beta  = i.dot( e1 );
	double gamma = i.dot( e0 );
	double alpha = 1.f - beta - gamma;

	double t = n.dot( e2 );
	bool hit = ( (t<payload->t_max) & (t>payload->t_min) & (beta>=-ray->eps*0.5f) & (gamma>=-ray->eps*0.5f) & (beta+gamma<=1.f) );

	if( hit ){
		payload->n = alpha*n0 + beta*n1 + gamma*n2;
		payload->t_max = t;
		payload->hit_point = ray->origin + ray->direction*t;
		return true;
	}

	return false;

} // end  ray -> triangle

// ray -> triangle without smoothed normals
static inline bool mcl::intersect::ray_triangle( const Ray *ray, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2, Payload *payload ){

	const Vec3f e0 = p1 - p0;
	const Vec3f e1 = p0 - p2;
	const Vec3f n = e1.cross( e0 );

	const Vec3f e2 = ( 1.0f / n.dot( ray->direction ) ) * ( p0 - ray->origin );
	const Vec3f i  = ray->direction.cross( e2 );

	double beta  = i.dot( e1 );
	double gamma = i.dot( e0 );
	double alpha = 1.f - beta - gamma;

	double t = n.dot( e2 );
	bool hit = ( (t<payload->t_max) & (t>payload->t_min) & (beta>=-ray->eps*0.5f) & (gamma>=-ray->eps*0.5f) & (beta+gamma<=1.f) );

	if( hit ){
		payload->n = n;
		payload->t_max = t;
		payload->hit_point = ray->origin + ray->direction*t;
		return true;
	}

	return false;

} // end  ray -> triangle


// ray -> axis aligned bounding box
static inline bool mcl::intersect::ray_aabb( const Ray *ray, const Vec3f &min, const Vec3f &max, const Payload *payload ){

	double txmin=0.f, txmax=0.f;
	double dirX = 1.f / ray->direction[0];
	if( dirX >= 0.0 ){
		txmin = dirX * ( min[0] - ray->origin[0] );
		txmax = dirX * ( max[0] - ray->origin[0] );
	}
	else{
		txmax = dirX * ( min[0] - ray->origin[0] );
		txmin = dirX * ( max[0] - ray->origin[0] );
	}

	double tymin=0.f, tymax=0.f;
	double dirY = 1.f / ray->direction[1];
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

	double tzmin=0.f, tzmax=0.f;
	double dirZ = 1.f / ray->direction[2];
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


static inline double mcl::intersect::point_aabb( const Vec3f &point, const Vec3f &min, const Vec3f &max ){
	double sqDist=0.f;
	for( int i=0; i<3; ++i ){
		if( point[i] < min[i] ){ sqDist += (min[i]-point[i])*(min[i]-point[i]); }
		if( point[i] > max[i] ){ sqDist += (point[i]-max[i])*(point[i]-max[i]); }
	}
	return sqDist;
}


static inline mcl::Vec3f mcl::intersect::point_triangle( const Vec3f &point, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2 ){
	Eigen::Vector3d tripoints[3] = { p0.cast<double>(), p1.cast<double>(), p2.cast<double>() };
	Eigen::Vector3d proj = mcl::Projection::Triangle( tripoints, point.cast<double>() );
	return Vec3f( proj[0], proj[1], proj[2] );
}


#endif
