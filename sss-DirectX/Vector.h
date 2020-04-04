#ifndef _VECTOR_H
#define _VECTOR_H

#include "D3dx9math.h"

#define PI  3.1415926535897932384

class Matrix{

public:
	static D3DXMATRIXA16 getPerspective(float fovx, float aspect, float znear, float zfar);	
	static D3DXMATRIXA16 getInvPerspective(float fovx, float aspect, float znear, float zfar);
	static D3DXMATRIXA16 getLookAT(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up);
	static D3DXMATRIXA16 getInvLookAT(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up);
	static D3DXMATRIXA16 getLinearPerspective(float fovx, float aspect, float znear, float zfar);
	static D3DXMATRIXA16 getNormalMatrix(const D3DXMATRIXA16 &modelViewMatrix);

	static D3DXMATRIXA16 getOrthographic(float left, float right, float bottom, float top, float znear, float zfar);

};


#endif