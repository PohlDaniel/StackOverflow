#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "RenderTarget.h"

class ShadowMap {
    public:
        static void init(ID3D10Device *device);
        static void release();

        ShadowMap(ID3D10Device *device, int width, int height);
        ~ShadowMap();

        void begin();
        void setWorldMatrix(const D3DXMATRIX &world);
        void end();

        ID3D10EffectTechnique *getTechnique() const { return effect->GetTechniqueByName("ShadowMap"); }
        operator ID3D10ShaderResourceView * const () { return *depthStencil; }

		D3DXMATRIX getViewProjectionTextureMatrix();

        ID3D10Device *device;
        DepthStencil *depthStencil;
        D3D10_VIEWPORT viewport;

        static ID3D10Effect *effect;
        static ID3D10InputLayout *vertexLayout;

	public:
		void ShadowMap::setViewMatrix(const D3DXVECTOR3 &lightPos, const D3DXVECTOR3 &target, const D3DXVECTOR3 &up);
		void ShadowMap::setProjectionMatrix(float fovx, float aspect, float znear, float zfar);
		void ShadowMap::setOrthMatrix(float left, float right, float bottom, float top, float znear, float zfar);
		const D3DXMATRIXA16 &ShadowMap::getOrthographicMatrix() const;
		const D3DXMATRIXA16 &ShadowMap::getDepthPassMatrix() const;
		const D3DXMATRIXA16 &ShadowMap::getProjectionMatrix() const;
		const D3DXMATRIXA16 &ShadowMap::getProjectionMatrixD3D() const;
		const D3DXMATRIXA16 &ShadowMap::getLinearProjectionMatrixD3D() const;
		const D3DXMATRIXA16 &ShadowMap::getViewMatrix() const;
		const D3DXVECTOR3	&ShadowMap::getPosition() const;

		D3DXVECTOR3		m_eye;
		D3DXVECTOR3		m_target;
		D3DXVECTOR3		m_viewDir;

		D3DXMATRIXA16		m_viewMatrix;
		D3DXMATRIXA16		m_projMatrix;

		D3DXMATRIXA16		m_biasMatrix;
		D3DXMATRIXA16		m_orthMatrix;
		D3DXMATRIXA16		m_projMatrixD3D;
		D3DXMATRIXA16		m_linearProjMatrixD3D;

		int depthmapWidth;
		int depthmapHeight;
};

#endif
