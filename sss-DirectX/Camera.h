#ifndef __cameraH__
#define __cameraH__



#include "Vector.h"

class Camera{

public:

	Camera();

	Camera(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &xAxis, const D3DXVECTOR3 &yAxis, const D3DXVECTOR3 &zAxis);

	Camera(const D3DXVECTOR3 &eye,
		   const D3DXVECTOR3 &xAxis,
		   const D3DXVECTOR3 &yAxis,
		   const D3DXVECTOR3 &zAxis,
		   const D3DXVECTOR3 &target,
		   const D3DXVECTOR3 &up);

	Camera(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up);

	~Camera();

	void perspective(float fovx, float aspect, float znear, float zfar);
	void orthographic(float left, float right, float bottom, float top, float znear, float zfar);
	void move(float dx, float dy, float dz);
	void rotate(float pitch, float yaw, float roll);

	const D3DXMATRIXA16 &getViewMatrix() const;
	const D3DXMATRIXA16 &getInvViewMatrix() const;
	const D3DXMATRIXA16 &getProjectionMatrix() const;
	const D3DXMATRIXA16 &getInvProjectionMatrix() const;
	const D3DXMATRIXA16 &getOrthographicMatrix() const;
	const D3DXVECTOR3	&getPosition() const;
	const D3DXVECTOR3	&getCamX() const;
	const D3DXVECTOR3	&getCamY() const;
	const D3DXVECTOR3	&getViewDirection() const;

	void setPosition(float x, float y, float z);
	void setPosition(const D3DXVECTOR3 &position);

private:
	
    void rotateFirstPerson(float pitch, float yaw);
	void updateViewMatrix(bool orthogonalizeAxes);
	void updateViewMatrix(const D3DXVECTOR3 &eye, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up);


	D3DXVECTOR3 WORLD_XAXIS;
	D3DXVECTOR3 WORLD_YAXIS;
	D3DXVECTOR3 WORLD_ZAXIS;

	
    float			m_fovx;
    float			m_znear;
    float			m_zfar;
	float			m_aspectRatio;
	float			m_accumPitchDegrees;

	D3DXVECTOR3		m_eye;
	D3DXVECTOR3		m_xAxis;
	D3DXVECTOR3		m_yAxis;
	D3DXVECTOR3		m_zAxis;
	D3DXVECTOR3		m_viewDir;

	D3DXMATRIXA16		m_viewMatrix;
	D3DXMATRIXA16		m_invViewMatrix;
	D3DXMATRIXA16		m_projMatrix;
	D3DXMATRIXA16		m_invProjMatrix;
	D3DXMATRIXA16		m_orthMatrix;

};
#endif // __cameraH__