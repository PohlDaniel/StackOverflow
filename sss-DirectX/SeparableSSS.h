
#ifndef SSS_H
#define SSS_H

#include "RenderTarget.h"
#include <string>
#include <dxgi.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <dxerr.h>

class SeparableSSS {
    public:
      
        SeparableSSS(ID3D10Device *device, 
                     int width, int height,
                     float fovy,
                     float sssWidth,
                     int nSamples=17,
                     bool stencilInitialized=true,
                     bool followShape=true,
                     bool separateStrengthSource=false,
                     DXGI_FORMAT format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        ~SeparableSSS();
       
        void go(ID3D10RenderTargetView *mainRTV,
                ID3D10ShaderResourceView *mainSRV,
                ID3D10ShaderResourceView *depthSRV,
                ID3D10DepthStencilView *depthDSV,
                ID3D10ShaderResourceView *stregthSRV = NULL,
                int id=1);
      
        void setWidth(float width) { this->sssWidth = width; }
        float getWidth() const { return sssWidth; }
		std::vector<D3DXVECTOR3> const & getSsssWeights() { return ssssWeights; }
		std::vector<D3DXVECTOR3> const & getSsssVariances() { return ssssVariances; }
		std::vector<D3DXVECTOR3> const & getTranslucencyWeights() { return translucencyWeights; }
		std::vector<D3DXVECTOR3> const & getTranslucencyVariances() { return translucencyVariances; }
    private:
		
        ID3D10Device *device;

        float sssWidth;
        int nSamples;
        bool stencilInitialized;
        D3DXVECTOR3 strength;
        D3DXVECTOR3 falloff;

		std::vector<D3DXVECTOR3> ssssWeights;
		std::vector<D3DXVECTOR3> ssssVariances;

		std::vector<D3DXVECTOR3> translucencyWeights;
		std::vector<D3DXVECTOR3> translucencyVariances;
		
        ID3D10Effect *effect;
        RenderTarget *tmpRT;
        Quad *quad;

        std::vector<D3DXVECTOR4> kernel;
        ID3D10EffectScalarVariable *idVariable, *sssWidthVariable;
        ID3D10EffectVectorVariable *kernelVariable;
        ID3D10EffectShaderResourceVariable *colorTexVariable, *depthTexVariable, *strengthTexVariable;
        ID3D10EffectTechnique *technique;

		bool useImg2DKernel;
};

#endif
