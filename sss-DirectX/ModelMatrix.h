#ifndef _MODELMATRIX_H
#define _MODELMATRIX_H

#include "Vector.h"
#include "D3dx9math.h"


//--------------------------------------------------------------------------------------
// Right-Handed System
//--------------------------------------------------------------------------------------
class ModelMatrix{

public:

	ModelMatrix();
	~ModelMatrix();

	const D3DXMATRIXA16 &getTransformationMatrix() const;
	const D3DXMATRIXA16 &getInvTransformationMatrix() const;

	void rotate(const D3DXVECTOR3 &axis, float degrees);
	void translate(float dx, float dy, float dz);
	void scale(float a, float b, float c);
	void setRotPos(const D3DXVECTOR3 &axis, float degrees, float dx, float dy, float dz);

	void setRotXYZPos(const D3DXVECTOR3 &axisX, float degreesX,
		const D3DXVECTOR3 &axisY, float degreesY,
		const D3DXVECTOR3 &axisZ, float degreesZ,
		float dx, float dy, float dz);

	D3DXMATRIXA16 orientation;
	D3DXVECTOR3 position;

private:

	D3DXMATRIXA16 startOrientation;
	D3DXVECTOR3 startPosition;
	bool pos;
	bool posRot;

	D3DXMATRIXA16 T;
	D3DXMATRIXA16 invT;
};


#endif