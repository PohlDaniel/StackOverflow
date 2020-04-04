#include "ShadowMap.h"
#include "Vector.h"

ID3D10Effect *ShadowMap::effect;
ID3D10InputLayout *ShadowMap::vertexLayout;


void ShadowMap::init(ID3D10Device *device) {
    HRESULT hr;
    if (effect == NULL)
		D3DX10CreateEffectFromFile(L"sss/ShadowMap.fx", NULL, NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, device, NULL, NULL, &effect, NULL, NULL);

    const D3D10_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = sizeof(layout) / sizeof(D3D10_INPUT_ELEMENT_DESC);

    D3D10_PASS_DESC desc;
    effect->GetTechniqueByName("ShadowMap")->GetPassByIndex(0)->GetDesc(&desc);
    device->CreateInputLayout(layout, numElements, desc.pIAInputSignature, desc.IAInputSignatureSize, &vertexLayout);
}


void ShadowMap::release() {
   
}


ShadowMap::ShadowMap(ID3D10Device *device, int width, int height) 
        : device(device) {

	depthmapWidth = width;
	depthmapHeight = height;
    depthStencil = new DepthStencil(device, width, height);

	m_biasMatrix = D3DXMATRIX(0.5, 0.0, 0.0, 0.0,
							  0.0, -0.5, 0.0, 0.0,
							  0.0, 0.0, 1.0, 0.0,
							  0.5, 0.5, 0.0, 1.0);
}


ShadowMap::~ShadowMap() {
}


void ShadowMap::begin() {
    HRESULT hr;

    device->IASetInputLayout(vertexLayout);

	device->ClearDepthStencilView(*depthStencil, D3D10_CLEAR_DEPTH, 1.0, 0);


	effect->GetVariableByName("view")->AsMatrix()->SetMatrix((float*)&m_viewMatrix);
	effect->GetVariableByName("projection")->AsMatrix()->SetMatrix((float*)&m_linearProjMatrixD3D);

	device->OMSetRenderTargets(0, NULL, *depthStencil);

	UINT numViewports = 1;
	device->RSGetViewports(&numViewports, &viewport);
	depthStencil->setViewport();
}

void ShadowMap::end() {
    device->RSSetViewports(1, &viewport);
    device->OMSetRenderTargets(0, NULL, NULL);
}


void ShadowMap::setWorldMatrix(const D3DXMATRIX &world) {
	effect->GetVariableByName("world")->AsMatrix()->SetMatrix((float*)&world);
}

D3DXMATRIX ShadowMap::getViewProjectionTextureMatrix() {
	D3DXMATRIX scale;
	D3DXMatrixScaling(&scale, 0.5f, -0.5f, 1.0f);

	D3DXMATRIX translation;
	D3DXMatrixTranslation(&translation, 0.5f, 0.5f, 0.0f);
	return m_viewMatrix * m_projMatrix * scale * translation;
}

void ShadowMap::setViewMatrix(const D3DXVECTOR3 &lightPos, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up) {


	D3DXMatrixLookAtLH(&m_viewMatrix, &lightPos, &target, &up);

	m_eye = lightPos;
	m_target = target;
	D3DXVec3Normalize(&m_viewDir, &(m_target - m_eye));
}

void ShadowMap::setProjectionMatrix(float fovx, float aspect, float znear, float zfar) {

	D3DXMatrixPerspectiveFovLH(&m_projMatrixD3D, fovx, (FLOAT)depthmapWidth / (FLOAT)depthmapHeight, znear, zfar);

	m_projMatrix = Matrix::getPerspective(fovx, (FLOAT)depthmapWidth / (FLOAT)depthmapHeight, znear, zfar);
	m_linearProjMatrixD3D = Matrix::getLinearPerspective(fovx, (FLOAT)depthmapWidth / (FLOAT)depthmapHeight, znear, zfar);
}



void ShadowMap::setOrthMatrix(float left, float right, float bottom, float top, float znear, float zfar) {

	D3DXMatrixOrthoLH(&m_orthMatrix, abs(right - left), abs(top - bottom), znear, zfar);

}

const D3DXMATRIXA16 &ShadowMap::getOrthographicMatrix() const {

	return m_orthMatrix;
}

const D3DXMATRIXA16 &ShadowMap::getDepthPassMatrix() const {

	return    m_projMatrix * m_biasMatrix;
}

const D3DXMATRIXA16 &ShadowMap::getProjectionMatrix() const {

	return  m_projMatrix;
}

const D3DXMATRIXA16 &ShadowMap::getProjectionMatrixD3D() const {

	return  m_projMatrixD3D;
}

const D3DXMATRIXA16 &ShadowMap::getLinearProjectionMatrixD3D() const {

	return  m_linearProjMatrixD3D;
}

const D3DXMATRIXA16 &ShadowMap::getViewMatrix() const {

	return m_viewMatrix;
}


const D3DXVECTOR3	&ShadowMap::getPosition() const {

	return m_eye;
}