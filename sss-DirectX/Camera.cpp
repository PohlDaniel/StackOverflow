#include <cmath>
#include <iostream>
#include "Camera.h"

Camera::Camera(){
	
	m_accumPitchDegrees = 0.0f;

	WORLD_XAXIS = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	WORLD_YAXIS = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	WORLD_ZAXIS = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	m_eye = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_xAxis = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	m_yAxis = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	m_zAxis = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	m_viewDir = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	D3DXMatrixIdentity(&m_viewMatrix);
	D3DXMatrixIdentity(&m_invViewMatrix);
	D3DXMatrixIdentity(&m_projMatrix);
	D3DXMatrixIdentity(&m_invProjMatrix);
	D3DXMatrixIdentity(&m_orthMatrix);

	updateViewMatrix(false);
}

Camera::Camera(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &xAxis, const D3DXVECTOR3 &yAxis, const D3DXVECTOR3 &zAxis){

	m_accumPitchDegrees = 0.0f;

	WORLD_XAXIS = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	WORLD_YAXIS = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	WORLD_ZAXIS = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	m_eye = eye;
	m_xAxis = xAxis;
	m_yAxis = yAxis;
	m_zAxis = zAxis;
	m_viewDir = zAxis;

	D3DXMatrixIdentity(&m_viewMatrix);
	D3DXMatrixIdentity(&m_invViewMatrix);
	D3DXMatrixIdentity(&m_projMatrix);
	D3DXMatrixIdentity(&m_invProjMatrix);
	D3DXMatrixIdentity(&m_orthMatrix);

	updateViewMatrix(true);
}

Camera::Camera(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up){

	m_accumPitchDegrees = 0.0f;

	WORLD_XAXIS = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	WORLD_YAXIS = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	WORLD_ZAXIS = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	m_eye = eye;
	m_xAxis = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	m_yAxis = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	m_zAxis = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	m_viewDir = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	D3DXMatrixIdentity(&m_viewMatrix);
	D3DXMatrixIdentity(&m_invViewMatrix);
	D3DXMatrixIdentity(&m_projMatrix);
	D3DXMatrixIdentity(&m_invProjMatrix);
	D3DXMatrixIdentity(&m_orthMatrix);

	updateViewMatrix(eye, target, up);
}

Camera::Camera(const D3DXVECTOR3  &eye, const D3DXVECTOR3  &xAxis, const D3DXVECTOR3  &yAxis, const D3DXVECTOR3  &zAxis, const D3DXVECTOR3  &target, const D3DXVECTOR3  &up){

	m_accumPitchDegrees = 0.0f;

	WORLD_XAXIS = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	WORLD_YAXIS = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	WORLD_ZAXIS = D3DXVECTOR3(0.0f, 0.0f, 1.0f);

	m_eye = eye;
	m_xAxis = xAxis;
	m_yAxis = yAxis;
	m_zAxis = zAxis;
	m_viewDir = zAxis;

	D3DXMatrixIdentity(&m_viewMatrix);
	D3DXMatrixIdentity(&m_invViewMatrix);
	D3DXMatrixIdentity(&m_projMatrix);
	D3DXMatrixIdentity(&m_invProjMatrix);
	D3DXMatrixIdentity(&m_orthMatrix);

	updateViewMatrix(eye, target, up);
}



Camera::~Camera(){

}

void Camera::updateViewMatrix(bool orthogonalizeAxes){

	// Regenerate the camera's local axes to orthogonalize them.
	if (orthogonalizeAxes){
	
		D3DXVec3Normalize(&m_zAxis, &m_zAxis);

		D3DXVec3Cross(&m_yAxis,&m_zAxis, &m_xAxis);
		D3DXVec3Normalize(&m_yAxis, &m_yAxis);

		D3DXVec3Cross(&m_xAxis, &m_yAxis, &m_zAxis);
		D3DXVec3Normalize(&m_xAxis, &m_xAxis);

		m_viewDir = m_zAxis;
	}

	// Reconstruct the view matrix.
	/*m_viewMatrix = D3DXMATRIXA16(m_xAxis[0], m_yAxis[0], m_zAxis[0], 0.0,
								 m_xAxis[1], m_yAxis[1], m_zAxis[1], 0.0,
								 m_xAxis[2], m_yAxis[2], m_zAxis[2], 0.0,
								 -D3DXVec3Dot(&m_xAxis, &m_eye), -D3DXVec3Dot(&m_yAxis, &m_eye), -D3DXVec3Dot(&m_zAxis, &m_eye), 1.0);*/

	m_viewMatrix(0, 0) = m_xAxis[0];
	m_viewMatrix(1, 0) = m_xAxis[1];
	m_viewMatrix(2, 0) = m_xAxis[2];
	m_viewMatrix(3, 0) = -D3DXVec3Dot(&m_xAxis, &m_eye);

	m_viewMatrix(0, 1) = m_yAxis[0];
	m_viewMatrix(1, 1) = m_yAxis[1];
	m_viewMatrix(2, 1) = m_yAxis[2];
	m_viewMatrix(3, 1) = -D3DXVec3Dot(&m_yAxis, &m_eye);

	m_viewMatrix(0, 2) = m_zAxis[0];
	m_viewMatrix(1, 2) = m_zAxis[1];
	m_viewMatrix(2, 2) = m_zAxis[2];
	m_viewMatrix(3, 2) = -D3DXVec3Dot(&m_zAxis, &m_eye);


	// Reconstruct the inv view matrix.	
	/*m_invViewMatrix = D3DXMATRIXA16(m_xAxis[0], m_xAxis[1], m_xAxis[2], 0.0f,
									m_yAxis[0], m_yAxis[1], m_yAxis[2], 0.0f,
									m_zAxis[0], m_zAxis[1], m_zAxis[2], 0.0f,
									m_eye[0], m_eye[1], m_eye[2], 1.0);*/

	m_invViewMatrix(0, 0) = m_xAxis[0];
	m_invViewMatrix(1, 0) = m_yAxis[0];
	m_invViewMatrix(2, 0) = m_zAxis[0];
	m_invViewMatrix(3, 0) = m_eye[0];

	m_invViewMatrix(0, 1) = m_xAxis[1];
	m_invViewMatrix(1, 1) = m_yAxis[1];
	m_invViewMatrix(2, 1) = m_zAxis[1];
	m_invViewMatrix(3, 1) = m_eye[1];

	m_invViewMatrix(0, 2) = m_xAxis[2];
	m_invViewMatrix(1, 2) = m_yAxis[2];
	m_invViewMatrix(2, 2) = m_zAxis[2];
	m_invViewMatrix(3, 2) = m_eye[2];
}

void Camera::updateViewMatrix(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up){

	m_zAxis = target - eye;
	D3DXVec3Normalize(&m_zAxis, &m_zAxis);

	D3DXVec3Cross(&m_xAxis, &up, &m_zAxis);
	D3DXVec3Normalize(&m_xAxis, &m_xAxis);
	
	D3DXVec3Cross(&m_yAxis, &m_zAxis, &m_xAxis);
	D3DXVec3Normalize(&m_yAxis, &m_yAxis);

	D3DXVec3Cross(&WORLD_YAXIS, &m_zAxis, &m_xAxis);
	D3DXVec3Normalize(&WORLD_YAXIS, &WORLD_YAXIS);

	m_viewDir = m_zAxis;

	// Reconstruct the view matrix.
	/*m_viewMatrix = D3DXMATRIXA16(m_xAxis[0], m_yAxis[0], m_zAxis[0], 0.0,
								 m_xAxis[1], m_yAxis[1], m_zAxis[1], 0.0, 
								 m_xAxis[2], m_yAxis[2], m_zAxis[2], 0.0, 
								 -D3DXVec3Dot(&m_xAxis, &m_eye), -D3DXVec3Dot(&m_yAxis, &m_eye), -D3DXVec3Dot(&m_zAxis, &m_eye) , 1.0);*/

	m_viewMatrix(0, 0) = m_xAxis[0];
	m_viewMatrix(1, 0) = m_xAxis[1];
	m_viewMatrix(2, 0) = m_xAxis[2];
	m_viewMatrix(3, 0) = -D3DXVec3Dot(&m_xAxis, &m_eye);

	m_viewMatrix(0, 1) = m_yAxis[0];
	m_viewMatrix(1, 1) = m_yAxis[1];
	m_viewMatrix(2, 1) = m_yAxis[2];
	m_viewMatrix(3, 1) = -D3DXVec3Dot(&m_yAxis, &m_eye);

	m_viewMatrix(0, 2) = m_zAxis[0];
	m_viewMatrix(1, 2) = m_zAxis[1];
	m_viewMatrix(2, 2) = m_zAxis[2];
	m_viewMatrix(3, 2) = -D3DXVec3Dot(&m_zAxis, &m_eye);

	// Reconstruct the inv view matrix.	
	/*m_invViewMatrix = D3DXMATRIXA16(m_xAxis[0], m_xAxis[1], m_xAxis[2], 0.0f,
									m_yAxis[0], m_yAxis[1], m_yAxis[2], 0.0f,
									m_zAxis[0], m_zAxis[1], m_zAxis[2], 0.0f,
									m_eye[0], m_eye[1], m_eye[2], 1.0);*/

	m_invViewMatrix(0, 0) = m_xAxis[0];
	m_invViewMatrix(1, 0) = m_yAxis[0];
	m_invViewMatrix(2, 0) = m_zAxis[0];
	m_invViewMatrix(3, 0) = m_eye[0];

	m_invViewMatrix(0, 1) = m_xAxis[1];
	m_invViewMatrix(1, 1) = m_yAxis[1];
	m_invViewMatrix(2, 1) = m_zAxis[1];
	m_invViewMatrix(3, 1) = m_eye[1];

	m_invViewMatrix(0, 2) = m_xAxis[2];
	m_invViewMatrix(1, 2) = m_yAxis[2];
	m_invViewMatrix(2, 2) = m_zAxis[2];
	m_invViewMatrix(3, 2) = m_eye[2];
}

void Camera::perspective(float fovx, float aspect, float znear, float zfar){

	m_fovx = fovx;
	m_aspectRatio = aspect;
	m_znear = znear;
	m_zfar = zfar;

	// Construct a projection matrix based on the horizontal field of view
	// 'fovx' rather than the more traditional vertical field of view 'fovy'.
	m_projMatrix = Matrix::getPerspectiveD3D(fovx, aspect, znear, zfar);
	m_invProjMatrix = Matrix::getInvPerspective(fovx, aspect, znear, zfar);
}

void Camera::orthographic(float left, float right, float bottom, float top, float znear, float zfar){

	m_znear = znear;
	m_zfar = zfar;

	m_orthMatrix = Matrix::getOrthographic(left, right, bottom, top, znear, zfar);
}

void Camera::move(float dx, float dy, float dz){

	D3DXVECTOR3 eye = m_eye;

	eye += m_xAxis * dx;
	eye += WORLD_YAXIS * dy;
	eye += m_viewDir * dz;

	setPosition(eye);
}



void Camera::rotate(float yaw, float pitch, float roll){

	// Rotates the camera based on its current behavior.
	// Note that not all behaviors support rolling.
	rotateFirstPerson(yaw, pitch);

	updateViewMatrix(true);
}

void Camera::rotateFirstPerson(float yawDegrees, float pitchDegrees){

	m_accumPitchDegrees += pitchDegrees;

	if (m_accumPitchDegrees > 90.0f){

		pitchDegrees = 90.0f - (m_accumPitchDegrees - pitchDegrees);
		m_accumPitchDegrees = 90.0f;
	}


	if (m_accumPitchDegrees < -90.0f){
		pitchDegrees = -90.0f - (m_accumPitchDegrees - pitchDegrees);
		m_accumPitchDegrees = -90.0f;
	}

	float yaw = D3DXToRadian(yawDegrees);
	float pitch = D3DXToRadian(pitchDegrees);

	D3DXMATRIX rotMtx;
	D3DXVECTOR4 result;

	// Rotate camera's existing x and z axes about the world y axis.
	if (yaw != 0.0f){

		//D3DXMatrixRotationY(&rotMtx, yaw);
		D3DXMatrixRotationAxis(&rotMtx, &WORLD_YAXIS, yaw);

		D3DXVec3Transform(&result, &m_xAxis, &rotMtx);
		m_xAxis = D3DXVECTOR3(result);

		D3DXVec3Transform(&result, &m_zAxis, &rotMtx);
		m_zAxis = D3DXVECTOR3(result);
	}

	// Rotate camera's existing y and z axes about its existing x axis.
	if (pitch != 0.0f){

		D3DXMatrixRotationAxis(&rotMtx, &m_xAxis, pitch);

		D3DXVec3Transform(&result, &m_yAxis, &rotMtx);
		m_yAxis = D3DXVECTOR3(result);

		D3DXVec3Transform(&result, &m_zAxis, &rotMtx);
		m_zAxis = D3DXVECTOR3(result);
	}
}

void Camera::setPosition(float x, float y, float z){

	m_eye = D3DXVECTOR3(x, y, z);
	updateViewMatrix(false);
}

void Camera::setPosition(const D3DXVECTOR3 &position){

	m_eye = position;
	updateViewMatrix(false);
}

const D3DXVECTOR3 &Camera::getPosition() const{
	return m_eye;
}

const D3DXVECTOR3 &Camera::getCamX() const{
	return m_xAxis;
}

const D3DXVECTOR3 &Camera::getCamY() const{
	return m_yAxis;
}



const D3DXVECTOR3 &Camera::getViewDirection() const{
	return m_viewDir;
}

const D3DXMATRIXA16 &Camera::getProjectionMatrix() const{

	return m_projMatrix;
}

const D3DXMATRIXA16 &Camera::getInvProjectionMatrix() const{

	return  m_invProjMatrix;
}

const D3DXMATRIXA16 &Camera::getOrthographicMatrix() const{

	return m_orthMatrix;
}

const D3DXMATRIXA16 &Camera::getViewMatrix() const{

	return m_viewMatrix;
}

const D3DXMATRIXA16 &Camera::getInvViewMatrix() const{

	return m_invViewMatrix;
}
