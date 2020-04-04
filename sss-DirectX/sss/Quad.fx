Texture2D finalTex;

SamplerState PointSampler {
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


struct VS_INPUT{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD;
};

struct PS_INPUT{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input) {			
	PS_INPUT output;
	output.position = input.position;
	output.texcoord = input.texcoord;
    return output;  
			
}



float4 PS(PS_INPUT input ) : SV_TARGET{
	return finalTex.Sample(PointSampler, input.texcoord);
}

DepthStencilState DisableDepthStencil {
	DepthEnable = FALSE;
	StencilEnable = FALSE;
};

BlendState NoBlending {
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = FALSE;
};

technique10 render {
	pass P0 {
		SetVertexShader(CompileShader(vs_4_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS()));

		SetDepthStencilState(DisableDepthStencil, 0);
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
	}
}
