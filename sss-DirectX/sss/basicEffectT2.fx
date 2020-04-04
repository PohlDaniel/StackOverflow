//--------------------------------------------------------------------------------------
// basicEffect.fx
//--------------------------------------------------------------------------------------
matrix modelMatrix;
matrix viewMatrix;
matrix projectionMatrix;

Texture2D tex2D;
SamplerState linearSampler{

    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VS_INPUT{

	float3 position : POSITION;
    float2 Tex : TEXCOORD;  
};

struct PS_INPUT{

	float4 position : SV_POSITION;
    float2 Tex : TEXCOORD;  
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input ){

	PS_INPUT output;

	output.position = mul( float4(input.position, 1.0), modelMatrix );
    output.position = mul( output.position, viewMatrix );    
    output.position = mul( output.position, projectionMatrix );
	output.Tex = input.Tex;
	
    return output;  
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input ) : SV_Target{

    return tex2D.Sample( linearSampler, input.Tex ); 
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
