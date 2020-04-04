




#ifndef STENCIL_INITIALIZED
#define STENCIL_INITIALIZED 0
#endif


float aspectRatio;
float3 gStrength;
float3 gFalloff;
float sssWidth;
int id;


DepthStencilState BlurStencil {
    DepthEnable = FALSE;
    StencilEnable = TRUE;
    FrontFaceStencilFunc = EQUAL;
};

DepthStencilState InitStencil {
    DepthEnable = FALSE;
    StencilEnable = TRUE;
    FrontFaceStencilPass = REPLACE;
};

BlendState NoBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
    BlendEnable[1] = FALSE;
};


SamplerState LinearSampler { 
	Filter = MIN_MAG_LINEAR_MIP_POINT; 
	AddressU = Clamp; 
	AddressV = Clamp; 
};

SamplerState PointSampler { 
	Filter = MIN_MAG_MIP_POINT; 
	AddressU = Clamp; 
	AddressV = Clamp; 
};
Texture2D colorTex;
Texture2D depthTex;



const float4 kernel2[17] = {
	float4(0.0727469,  0.0798656,  0.0871559,  0),
	float4(0.0134888,  0.0110569,  0.00889932,  -3.64969),
	float4(0.0336357,  0.0284734,  0.0238736,  -2.7943),
	float4(0.0450509,  0.0401367,  0.0355976,  -2.05295),
	float4(0.0594925,  0.0560719,  0.0523212,  -1.42566),	
	float4(0.0734215,  0.0727698,  0.0715558,  -0.912423),
	float4(0.082168,  0.0850056,  0.0872929,  -0.513238),
	float4(0.0823918,  0.0872948,  0.0921819,  -0.228106),
	float4(0.0739774,  0.079258,  0.0846997,  -0.0570264),
	float4(0.0739774,  0.079258,  0.0846997,  0.0570264),	
	float4(0.0823918,  0.0872948,  0.0921819,  0.228106),
	float4(0.082168,  0.0850056, 0.087293,  0.513238),
	float4(0.0734215,  0.0727698,  0.0715558,  0.912423),
	float4(0.0594925,  0.0560719,  0.0523213,  1.42566),
	float4(0.0450509,  0.0401367,  0.0355976,  2.05295),	
	float4(0.0336357, 0.0284734,  0.0238736,  2.7943),
	float4(0.0134888,  0.0110569,  0.00889932,  3.64969)
};


void DX10_SSSSBlurVS(float4 position : POSITION,
                     out float4 svPosition : SV_POSITION,
                     inout float2 texcoord : TEXCOORD0) {
    svPosition = position;
}

float4 DX10_SSSSBlurPS(float4 position : SV_POSITION,
                       float2 texcoord : TEXCOORD0,
                       uniform Texture2D colorTex,
                       uniform Texture2D depthTex,
                       uniform float sssWidth,
                       uniform float2 dir,
                       uniform bool initStencil) : SV_TARGET {
    // Fetch color of current pixel:
    float4 colorM = colorTex.SampleLevel(PointSampler, texcoord, 0);
	float colorMa = colorM.a;
		  colorMa = 1.0;
    // Initialize the stencil buffer in case it was not already available:
    if (initStencil) // (Checked in compile time, it's optimized away)
        if (colorMa == 0.0) discard;

    // Fetch linear depth of current pixel:
    float depthM = depthTex.SampleLevel(PointSampler, texcoord, 0).r;

    // Calculate the sssWidth scale (1.0 for a unit plane sitting on the
    // projection window):
    float distanceToProjectionWindow = 1.0 / tan(0.5 * radians(20.0));
    float scale = distanceToProjectionWindow / depthM;
	
	// Calculate the final step to fetch the surrounding pixels:
    float2 finalStep = scale * dir;
    finalStep *= colorMa; // Modulate it using the alpha channel.
    finalStep *= 1.0 / (2.0 * sssWidth); // sssWidth in mm / world space unit, divided by 2 as uv coords are from [0 1]
	
    // Accumulate the center sample:
    float4 colorBlurred = colorM;
    colorBlurred.rgb *= kernel2[0].rgb;

    // Accumulate the other samples:
    [unroll]
    for (int i = 1; i < 17; i++) {
        // Fetch color and depth for current sample:
        float2 offset = texcoord + kernel2[i].a * finalStep;
        float4 color = colorTex.SampleLevel(PointSampler, offset, 0);

      
        // Accumulate:
        colorBlurred.rgb += kernel2[i].rgb * color.rgb;
    }

    return colorBlurred;
}


technique10 SSS {
    pass SSSSBlurX {
        SetVertexShader(CompileShader(vs_4_0, DX10_SSSSBlurVS()));
        SetGeometryShader(NULL);

        #if STENCIL_INITIALIZED == 1
        SetPixelShader(CompileShader(ps_4_0, DX10_SSSSBlurPS(colorTex, depthTex, sssWidth, float2(aspectRatio, 0.0), false)));
        SetDepthStencilState(BlurStencil, id);
        #else
        SetPixelShader(CompileShader(ps_4_0, DX10_SSSSBlurPS(colorTex, depthTex, sssWidth, float2(aspectRatio, 0.0), true)));
        SetDepthStencilState(InitStencil, id);
        #endif

        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }

    pass SSSSBlurY {
        SetVertexShader(CompileShader(vs_4_0, DX10_SSSSBlurVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, DX10_SSSSBlurPS(colorTex, depthTex, sssWidth, float2(0.0, 1.0), false)));
        
        SetDepthStencilState(BlurStencil, id);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }	
	
	
}
