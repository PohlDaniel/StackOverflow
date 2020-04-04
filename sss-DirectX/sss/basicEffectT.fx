//Set the HLSL version:
#ifndef SMAA_HLSL_4
#define SSSS_HLSL_4 1
#endif

// And include our header!
#include "SeparableSSS.h"


#pragma warning(disable: 3571) 
#define N_LIGHTS 5 


//--------------------------------------------------------------------------------------
// basicEffect.fx
//--------------------------------------------------------------------------------------
matrix modelMatrix;
matrix viewMatrix;
matrix projectionMatrix;
matrix normalMatrix;

matrix projectionShadow;
matrix viewShadow;
matrix viewProjectionShadow;
float farPlane;




struct Light {
    float3 position;
    float3 direction;
    float falloffStart;
    float falloffWidth;
    float3 color;
    float attenuation;
    float farPlane;
    float bias;
    float4x4 viewProjection;
};

cbuffer UpdatedPerFrame {
    float3 cameraPosition;
    Light lights[N_LIGHTS];

    matrix currWorldViewProj;
    matrix prevWorldViewProj;
    float2 jitter;
}

cbuffer UpdatedPerObject {
    matrix world;
    matrix worldInverseTranspose;
    int id;
    float bumpiness;
    float specularIntensity;
    float specularRoughness;
    float specularFresnel;
    float translucency;
    float sssEnabled;
    float sssWidth;
    float ambient;
}

float translucencyEnabled;

Texture2D shadowMaps[N_LIGHTS];
Texture2D diffuseTex;
Texture2D normalTex;
Texture2D specularAOTex;
Texture2D beckmannTex;
TextureCube irradianceTex;

Texture2D specularsTex;

SamplerState linearSampler{

    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};


SamplerState AnisotropicSampler {
    Filter = ANISOTROPIC;
    AddressU = Clamp;
    AddressV = Clamp;
    MaxAnisotropy = 16;
};

SamplerState AnisotropicSamplerWrap {
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
    MaxAnisotropy = 16;
};

struct VS_INPUT{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PS_INPUT{

	 // Position and texcoord:
    float4 svPosition : SV_POSITION;
    float2 texcoord : TEXCOORD0;

    // For reprojection:
    float3 currPosition : TEXCOORD1;
    float3 prevPosition : TEXCOORD2;

    // For shading:
    centroid float3 worldPosition : TEXCOORD3;
    centroid float3 view : TEXCOORD4;
    centroid float3 normal : TEXCOORD5;
    centroid float3 tangent : TEXCOORD6;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input ){

	PS_INPUT output;

	output.svPosition = mul( float4(input.position, 1.0), modelMatrix );
    output.svPosition = mul( output.svPosition, viewMatrix );    
    output.svPosition = mul( output.svPosition, projectionMatrix );
	output.texcoord = input.texcoord;
	
	output.normal = mul(input.normal, (float3x3) normalMatrix);  
    output.worldPosition = mul(input.position, modelMatrix).xyz;
	
    return output;  
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input ) : SV_Target{

	//float thikness = SSSSThikness(input.worldPosition, input.normal, shadowMap, viewProjectionShadow, farPlane);
	//return float4(thikness,thikness,thikness, 1.0);
	
    return diffuseTex.Sample( AnisotropicSamplerWrap, input.texcoord ); 
	//return float4(0.5, 0.8 ,0.0 ,1); 
}


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique10 render{

    pass P0{
	
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
