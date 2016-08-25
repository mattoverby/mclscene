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

#ifndef MCLSCENE_RENDERUTILS_HPP
#define MCLSCENE_RENDERUTILS_HPP 1

#include <math.h>

namespace mcl {


//////////////////////
/// Definitions
//////////////////////


namespace Draw {

	// Draws a sphere the bad way.
	static inline void drawSphere( float x, float y, float z, float r, int dimensions );

	// From: https://stackoverflow.com/questions/1700211/to-dynamically-increment-from-blue-to-red-using-c
	static inline float blend(float a, float b, float alpha){ return (1.f - alpha) * a + alpha * b; }

	// gradient should be 0-1. blended needs to be a 3-element array
	// From https://stackoverflow.com/questions/1700211/to-dynamically-increment-from-blue-to-red-using-c
	static inline void colorBlend( float *blended, float a[3], float b[3], float gradient );

	// Rotate a 3D point p about axis a
	static inline void rotatePoint( float *p, const float *a, const float &angle );

}; // end namespace Draw


}; // end namespace mcl


//////////////////////
/// Implementations Below
//////////////////////


static inline void mcl::Draw::drawSphere( float x, float y, float z, float r, int dimensions ) {

	int lats = dimensions;
	int longs = dimensions;

	// Drawing a sphere from
	// https://stackoverflow.com/questions/10294345/texture-coordinates-for-rendering-a-3d-sphere

	glPushMatrix();
	glTranslatef( x, y, z );

	double dTheta=180/dimensions, dLon=360/dimensions, degToRad=3.141592665885/180 ;
	for(double lat =0; lat <=180; lat+=dTheta)
	{

		glBegin( GL_QUAD_STRIP ) ;
		for(double lon =0 ; lon <=360 ; lon+=dLon)
		{  

			//Vertex 1
			x = r*cos(lon * degToRad) * sin(lat * degToRad) ;
			y = r*sin(lon * degToRad) * sin(lat * degToRad) ;
			z = r*cos(lat * degToRad) ;
			glNormal3d( x, y, z) ;
			glTexCoord2d(lon/360-0.25, lat/180);
			glVertex3d( x, y, z ) ;


			//Vetex 2
			x = r*cos(lon * degToRad) * sin( (lat + dTheta)* degToRad) ;
			y = r*sin(lon * degToRad) * sin((lat + dTheta) * degToRad) ;
			z = r*cos( (lat + dTheta) * degToRad ) ;
			glNormal3d( x, y, z ) ;
			glTexCoord2d(lon/360-0.25, (lat + dTheta-1)/(180)); 
			glVertex3d( x, y, z ) ;


			//Vertex 3
			x = r*cos((lon + dLon) * degToRad) * sin((lat) * degToRad) ;
			y = r*sin((lon + dLon) * degToRad) * sin((lat) * degToRad) ;
			z = r*cos((lat) * degToRad ) ;
			glNormal3d( x, y, z ) ;
			glTexCoord2d((lon + dLon)/(360)-0.25 ,(lat)/180);
			glVertex3d( x, y, z ) ;


			//Vertex 4
			x = r*cos((lon + dLon) * degToRad) * sin((lat + dTheta)* degToRad) ;
			y = r*sin((lon + dLon)* degToRad) * sin((lat + dTheta)* degToRad) ;
			z = r*cos((lat + dTheta)* degToRad ) ;
			glNormal3d( x, y, z ) ;
			glTexCoord2d((lon + dLon)/360-0.25, (lat + dTheta)/(180));
			glVertex3d( x, y, z ) ;


		}
		glEnd();
	}

	glPopMatrix();

} // end draw sphere


static inline void mcl::Draw::colorBlend( float *blended, float a[3], float b[3], float gradient ){
	if( gradient > 1.f ){ gradient = 1.f; }
	if( gradient < 0.f ){ gradient = 0.f; }
	blended[0] = blend( a[0], b[0], gradient );
	blended[1] = blend( a[1], b[1], gradient );
	blended[2] = blend( a[2], b[2], gradient );
}

static inline void mcl::Draw::rotatePoint( float *p, const float *a, const float &angle ){

	float pos[3];
	pos[0] = p[0];
	pos[1] = p[1];
	pos[2] = p[2];

	float axis[3];
	axis[0] = a[0];
	axis[1] = a[1];
	axis[2] = a[2];

	float c = cosf((float)angle);
	float s = sinf((float)angle);

	float rotMat[4][4];

	// Setup the rotation matrix, this matrix is based off of the rotation matrix used in glRotatef.
	rotMat[0][0] = axis[0] * axis[0] * (1 - c) + c;           rotMat[0][1] = axis[0] * axis[1] * (1 - c) - axis[2] * s; rotMat[0][2] = axis[0] * axis[2] * (1 - c) + axis[1] * s; rotMat[0][3] = 0;
	rotMat[1][0] = axis[1] * axis[0] * (1 - c) + axis[2] * s; rotMat[1][1] = axis[1] * axis[1] * (1 - c) + c;           rotMat[1][2] = axis[1] * axis[2] * (1 - c) - axis[0] * s; rotMat[1][3] = 0;
	rotMat[2][0] = axis[0] * axis[2] * (1 - c) - axis[1] * s; rotMat[2][1] = axis[1] * axis[2] * (1 - c) + axis[0] * s; rotMat[2][2] = axis[2] * axis[2] * (1 - c) + c;           rotMat[2][3] = 0;
	rotMat[3][0] = 0;                       rotMat[3][1] = 0;                       rotMat[3][2] = 0;                       rotMat[3][3] = 1;

	// Multiply the rotation matrix with the position vector.
	float tmp[3];
	tmp[0] = rotMat[0][0] * pos[0] + rotMat[0][1] * pos[1] + rotMat[0][2] * pos[2] + rotMat[0][3];
	tmp[1] = rotMat[1][0] * pos[0] + rotMat[1][1] * pos[1] + rotMat[1][2] * pos[2] + rotMat[1][3];
	tmp[2] = rotMat[2][0] * pos[0] + rotMat[2][1] * pos[1] + rotMat[2][2] * pos[2] + rotMat[2][3];
  
	// Update p
	p[0] = tmp[0];
	p[1] = tmp[1];
	p[2] = tmp[2];	
}

#endif
