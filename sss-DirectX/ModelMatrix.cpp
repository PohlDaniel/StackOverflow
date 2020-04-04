
#include <iostream>
#include "ModelMatrix.h"

//D3DMatrices are column major

ModelMatrix::ModelMatrix(){

	pos = false;
	posRot = false;

	D3DXMatrixIdentity(&T);
	D3DXMatrixIdentity(&invT);
	D3DXMatrixIdentity(&orientation);
	
	startPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	
}

ModelMatrix::~ModelMatrix(){

}


void ModelMatrix::setRotXYZPos(const D3DXVECTOR3 &axisX, float degreesX,
	const D3DXVECTOR3 &axisY, float degreesY,
	const D3DXVECTOR3 &axisZ, float degreesZ,
	float dx, float dy, float dz){

	if (posRot) return;

	D3DXMATRIXA16 rotMtx;
	D3DXMATRIXA16 invRotMtx;

	if (degreesX != 0 || (!axisX[0] == 0.0 && !axisX[1] == 0.0 && !axisX[2] == 0.0)){
		float rad = (degreesX * PI) / 180.0f;
		D3DXMatrixRotationAxis(&rotMtx, &axisX, rad);
		D3DXMatrixTranspose(&invRotMtx, &rotMtx);

		T = T * rotMtx;
		invT = invRotMtx * invT;

	}

	if (degreesY != 0 || (!axisY[0] == 0.0 && !axisY[1] == 0.0 && !axisY[2] == 0.0)){
		float rad = (degreesY * PI) / 180.0f;
		D3DXMatrixRotationAxis(&rotMtx, &axisY, rad);
		D3DXMatrixTranspose(&invRotMtx, &rotMtx);

		T = T * rotMtx;
		invT = invRotMtx * invT;
	}

	if (degreesZ != 0 || (!axisZ[0] == 0.0 && !axisZ[1] == 0.0 && !axisZ[2] == 0.0)){
		float rad = (degreesZ * PI) / 180.0f;
		D3DXMatrixRotationAxis(&rotMtx, &axisZ, rad);
		D3DXMatrixTranspose(&invRotMtx, &rotMtx);

		T = T * rotMtx;
		invT = invRotMtx * invT;

	}
	startOrientation = T;
	orientation = T;

	//T = T * Translate
	T._11 = T._11 + T._14 * dx; T._12 = T._12 + T._14 * dy; T._13 = T._13 + T._14 * dz;
	T._21 = T._21 + T._24 * dx; T._22 = T._22 + T._24 * dy; T._23 = T._23 + T._24 * dz;
	T._31 = T._31 + T._34 * dx; T._32 = T._32 + T._34 * dy; T._33 = T._33 + T._34 * dz;
	T._41 = T._41 + T._44 * dx; T._42 = T._42 + T._44 * dy; T._43 = T._43 + T._44 * dz;

	//T^-1 = Translate^-1 * T^-1 
	invT._41 = invT._41 - dx*invT._11 - dy*invT._21 - dz*invT._31;
	invT._42 = invT._42 - dx*invT._12 - dy*invT._22 - dz*invT._32;
	invT._43 = invT._43 - dx*invT._13 - dy*invT._23 - dz*invT._33;
	invT._44 = invT._44 - dx*invT._14 - dy*invT._24 - dz*invT._34;

	startPosition = D3DXVECTOR3(dx, dz, dy);
	position = D3DXVECTOR3(dx, dz, dy);

	if (startPosition[0] != 0.0f || startPosition[1] != 0.0f || startPosition[2] != 0.0f){

		pos = true;
	}

	posRot = true;
}

void ModelMatrix::setRotPos(const D3DXVECTOR3 &axis, float degrees, float dx, float dy, float dz){
	

	if (posRot) return;

	D3DXMATRIX rotMtx;
	D3DXMATRIX invRotMtx;

	if (degrees != 0 || (!axis[0] == 0.0 && !axis[1] == 0.0 && !axis[2] == 0.0)){

		float rad = (degrees * PI) / 180.0f;

		D3DXMatrixRotationAxis(&rotMtx, &axis, rad);
		D3DXMatrixTranspose(&invRotMtx, &rotMtx);

		T = T * rotMtx;
		invT = invRotMtx * invT;
	}

	startOrientation = T;
	orientation = T;

	//T = T * Translate
	T._11 = T._11 + T._14 * dx; T._12 = T._12 + T._14 * dy; T._13 = T._13 + T._14 * dz;
	T._21 = T._21 + T._24 * dx; T._22 = T._22 + T._24 * dy; T._23 = T._23 + T._24 * dz;
	T._31 = T._31 + T._34 * dx; T._32 = T._32 + T._34 * dy; T._33 = T._33 + T._34 * dz;
	T._41 = T._41 + T._44 * dx; T._42 = T._42 + T._44 * dy; T._43 = T._43 + T._44 * dz;

	//T^-1 = Translate^-1 * T^-1 
	invT._41 = invT._41 - dx*invT._11 - dy*invT._21 - dz*invT._31;
	invT._42 = invT._42 - dx*invT._12 - dy*invT._22 - dz*invT._32;
	invT._43 = invT._43 - dx*invT._13 - dy*invT._23 - dz*invT._33;
	invT._44 = invT._44 - dx*invT._14 - dy*invT._24 - dz*invT._34;

	startPosition = D3DXVECTOR3(dx, dz, dy);
	position = D3DXVECTOR3(dx, dz, dy);

	
	if (startPosition[0] != 0.0f || startPosition[1] != 0.0f || startPosition[2] != 0.0f){
		
		pos = true;
	}

	posRot = true;
}

void ModelMatrix::rotate(const D3DXVECTOR3 &axis, float degrees){

	D3DXMATRIX rotMtx;
	D3DXMatrixRotationAxis(&rotMtx, &axis, degrees);
	
	orientation = orientation *  rotMtx;

	D3DXMATRIX invRotMtx;
	D3DXMatrixTranspose(&invRotMtx, &rotMtx);

	

	if (!pos){

		T =  T * rotMtx ;
		invT =  invRotMtx * invT;

	}else{
		
		D3DXMATRIXA16 translate;
		D3DXMatrixTranslation(&translate, startPosition[0], startPosition[2], startPosition[1]);

		D3DXMATRIXA16 invTranslate;
		D3DXMatrixTranslation(&invTranslate, -startPosition[0], -startPosition[2], -startPosition[1]);

		T = T * invTranslate * rotMtx * translate;
		invT = invTranslate * invRotMtx * translate * invT;
	}
}

void ModelMatrix::translate(float dx, float dy, float dz){
	//----------------column major----------------
	//			   [ 1	 0	 0	 0 ]
	//			   [ 0	 1	 0	 0 ]
	//			   [ 0	 0	 1	 0 ]
	//			   [ dx	 dy	 dz	 1 ]

	position = position + D3DXVECTOR3(dx, dy, dz);

	D3DXMATRIXA16 translate;	
	
	//T = T * Translate
	T._11 = T._11 + T._14 * dx; T._12 = T._12 + T._14 * dy; T._13 = T._13 + T._14 * dz;
	T._21 = T._21 + T._24 * dx; T._22 = T._22 + T._24 * dy; T._23 = T._23 + T._24 * dz;
	T._31 = T._31 + T._34 * dx; T._32 = T._32 + T._34 * dy; T._33 = T._33 + T._34 * dz;
	T._41 = T._41 + T._44 * dx; T._42 = T._42 + T._44 * dy; T._43 = T._43 + T._44 * dz;

	//T^-1 = Translate^-1 * T^-1 
	invT._41 = invT._41 - dx*invT._11 - dy*invT._21 - dz*invT._31;
	invT._42 = invT._42 - dx*invT._12 - dy*invT._22 - dz*invT._32;
	invT._43 = invT._43 - dx*invT._13 - dy*invT._23 - dz*invT._33;
	invT._44 = invT._44 - dx*invT._14 - dy*invT._24 - dz*invT._34;
}

void ModelMatrix::scale(float a, float b, float c){

	//			   [ a	 0	 0	 0 ]
	//			   [ 0	 b	 0	 0 ]
	//			   [ 0	 0	 c	 0 ]
	//			   [ 0	 0	 0	 1 ]

	if (a == 0) a = 1.0;
	if (b == 0) b = 1.0;
	if (c == 0) c = 1.0;

	// T = T * scale
	T._11 = T._11 * a;  T._12 = T._12 * b; T._13 = T._13 * c;
	T._21 = T._21 * a;  T._22 = T._22 * b; T._23 = T._23 * c;
	T._31 = T._31 * a;  T._32 = T._32 * b; T._33 = T._33 * c;
	T._41 = T._41 * a;  T._42 = T._42 * b; T._43 = T._43 * c;

	// T^-1 =  Scale^{-1} * T^-1 
	invT._11 = invT._11 * (1.0 / a); 
	invT._12 = invT._12 * (1.0 / a); 
	invT._13 = invT._13 * (1.0 / a); 
	invT._14 = invT._14 * (1.0 / a);

	invT._21 = invT._21 * (1.0 / b); 
	invT._22 = invT._22 * (1.0 / b); 
	invT._23 = invT._23 * (1.0 / b); 
	invT._24 = invT._24 * (1.0 / b);

	invT._31 = invT._31 * (1.0 / c);
	invT._32 = invT._32 * (1.0 / c); 
	invT._33 = invT._33 * (1.0 / c); 
	invT._34 = invT._34 * (1.0 / c);
	
}

const D3DXMATRIXA16 &ModelMatrix::getTransformationMatrix() const{

	return T;
}

const D3DXMATRIXA16 &ModelMatrix::getInvTransformationMatrix() const{

	return invT;
}