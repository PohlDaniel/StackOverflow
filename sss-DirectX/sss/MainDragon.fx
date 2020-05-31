#define N_LIGHTS 5 

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

/*
SamplerState AnisotropicSampler {
    Filter = ANISOTROPIC;
    AddressU = Clamp;
    AddressV = Clamp;
    MaxAnisotropy = 16;
};
*/

SamplerState AnisotropicSampler {
    Filter = ANISOTROPIC;
    AddressU = Clamp;
    AddressV = Clamp;
    MaxAnisotropy = 16;
};

SamplerComparisonState ShadowSampler {
    ComparisonFunc = Less;
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
};


struct RenderV2P {
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

struct VS_INPUT{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
  
};


float3 weights2[] = {
	float3(0.153845, 0.159644, 0.01224),
	float3(0.019712, 0.010854, 0.383008),
	float3(0.048265, 0.402794, 0.022724),
	float3(0.42143, 0.021578, 0.161395),
	float3(0.009353, 0.050785, 0.051702)
};

float3 variances2[5] = {
	float3(0.243373, 0.19885, 0.000128),
	float3(0.001579, 0.000128, 1.76679),
	float3(0.016459, 2.29636, 0.001403),
	float3(3.10841, 0.001473, 0.164507),
	float3(0.000131, 0.014631, 0.013066)
};

float3 SSSSTransmittance3(float translucency,
        float sssWidth,
        float3 worldPosition,
        float3 worldNormal,
        float3 light,
        Texture2D shadowMap,
        float4x4 lightViewProjection,
        float lightFarPlane) {
   
	float scale = sssWidth * (1.0 - translucency); // sssWidth in  mm / world space unit
    float4 shrinkedPos = float4(worldPosition - 0.005 * worldNormal, 1.0);
    float4 shadowPosition = mul(shrinkedPos, lightViewProjection);
    float d1 = shadowMap.SampleLevel(PointSampler, shadowPosition.xy / shadowPosition.w, 0).r; // 'd1' has a range of 0..1
	
	
	
    float d2 = shadowPosition.z; // 'd2' has a range of 0..'lightFarPlane'
    d1 *= lightFarPlane; // So we scale 'd1' accordingly:
    float d = scale * abs(d1 - d2);

    float dd = -d * d;

	float3 profile = weights2[0] * exp(dd / variances2[0]) +
					 weights2[1] * exp(dd / variances2[1]) +
					 weights2[2] * exp(dd / variances2[2]) +
					 weights2[3] * exp(dd / variances2[3]) +
					 weights2[4] * exp(dd / variances2[4]);

    return profile * saturate(0.3 + dot(light, -worldNormal));
}

float getDepthPassSpaceZ(float zWC, float near, float far){

	// Assume standard depth range [0..1]  
    float z_e =   (near * far) / (far + zWC * (near - far));	//[near, far]
	return z_e;
}

float3 SSSSTransmittance4( float translucency, 
						   float sssWidth, 
						   float3 worldPosition, 
						   float3 worldNormal, 
						   float3 light, 
						   Texture2D shadowMap, 
						   float4x4 lightViewProjection,
						   float lightNearPlane,
						   float lightFarPlane){
	
	float scale = sssWidth * (1.0 - translucency);
	
	float4 shrinkedPos = float4(worldPosition - 0.005 * worldNormal, 1.0);
	float4 shadowPosition = mul(shrinkedPos, lightViewProjection);
	shadowPosition = shadowPosition/shadowPosition.w;
	
	float zIn = shadowMap.SampleLevel(PointSampler, shadowPosition.xy, 0).r; 
	float zOut = shadowPosition.z;
	
	zIn = getDepthPassSpaceZ(zIn, lightNearPlane, lightFarPlane);
    zOut = getDepthPassSpaceZ(zOut, lightNearPlane, lightFarPlane);
	
	float d = scale * (zOut - zIn);
	
	float dd = -d * d;

	float3 profile = weights2[0] * exp(dd / variances2[0]) +
					 weights2[1] * exp(dd / variances2[1]) +
					 weights2[2] * exp(dd / variances2[2]) +
					 weights2[3] * exp(dd / variances2[3]) +
					 weights2[4] * exp(dd / variances2[4]);

    return profile * saturate(0.3 + dot(light, -worldNormal));
}

RenderV2P RenderVS(VS_INPUT input) {
    RenderV2P output;

    // Transform to homogeneous projection space:
    output.svPosition = mul(float4(input.position , 1.0), currWorldViewProj);
    output.currPosition = output.svPosition.xyw;
    output.prevPosition = mul(float4(input.position , 1.0), prevWorldViewProj).xyw;

    // Covert the jitter from non-homogeneous coordiantes to homogeneous
    // coordinates and add it:
    // (note that for providing the jitter in non-homogeneous projection space,
    //  pixel coordinates (screen space) need to multiplied by two in the C++
    //  code)
    output.svPosition.xy -= jitter * output.svPosition.w;

    // Positions in projection space are in [-1, 1] range, while texture
    // coordinates are in [0, 1] range. So, we divide by 2 to get velocities in
    // the scale:
    output.currPosition.xy /= 2.0;
    output.prevPosition.xy /= 2.0;

    // Texture coordinates have a top-to-bottom y axis, so flip this axis:
    output.currPosition.y = -output.currPosition.y;
    output.prevPosition.y = -output.prevPosition.y;

    // Output texture coordinates:
    output.texcoord = input.texcoord;

    // Build the vectors required for shading:
    output.worldPosition = mul(float4(input.position , 1.0), world).xyz;
    output.view = cameraPosition - output.worldPosition;
    output.normal = mul(input.normal, (float3x3) worldInverseTranspose);
    output.tangent = mul(input.tangent, (float3x3) worldInverseTranspose);
	//output.bitangent = mul(input.bitangent, (float3x3) worldInverseTranspose);
	
    return output;
}


float3 BumpMap(Texture2D normalTex, float2 texcoord) {
    float3 bump;
    bump.xy = -1.0 + 2.0 * normalTex.Sample(AnisotropicSampler, texcoord).gr;
    bump.z = sqrt(1.0 - bump.x * bump.x - bump.y * bump.y);
    return normalize(bump);
}

float Fresnel(float3 half, float3 view, float f0) {
    float base = 1.0 - dot(view, half);
    float exponential = pow(base, 5.0);
    return exponential + f0 * (1.0 - exponential);
}

float SpecularKSK(Texture2D beckmannTex, float3 normal, float3 light, float3 view, float roughness) {
    float3 half = view + light;
    float3 halfn = normalize(half);

    float ndotl = max(dot(normal, light), 0.0);
    float ndoth = max(dot(normal, halfn), 0.0);

    float ph = pow(2.0 * beckmannTex.SampleLevel(LinearSampler, float2(ndoth, roughness), 0).r, 10.0);
    float f = lerp(0.25, Fresnel(halfn, view, 0.028), specularFresnel);
    float ksk = max(ph * f / dot(half, half), 0.0);

    return ndotl * ksk;   
}

float Shadow(float3 worldPosition, int i) {
    float4 shadowPosition = mul(float4(worldPosition, 1.0), lights[i].viewProjection);
    shadowPosition.xy /= shadowPosition.w;
    shadowPosition.z += lights[i].bias;
    return shadowMaps[i].SampleCmpLevelZero(ShadowSampler, shadowPosition.xy, shadowPosition.z / lights[i].farPlane).r;
}

float ShadowPCF(float3 worldPosition, int i, int samples, float width) {
    float4 shadowPosition = mul(float4(worldPosition, 1.0), lights[i].viewProjection);
    shadowPosition.xy /= shadowPosition.w;
    shadowPosition.z += lights[i].bias;
    
    float w, h;
    shadowMaps[i].GetDimensions(w, h);

    float shadow = 0.0;
    float offset = (samples - 1.0) / 2.0;
    [unroll]
    for (float x = -offset; x <= offset; x += 1.0) {
        [unroll]
        for (float y = -offset; y <= offset; y += 1.0) {
            float2 pos = shadowPosition.xy + width * float2(x, y) / w;
            shadow += shadowMaps[i].SampleCmpLevelZero(ShadowSampler, pos, shadowPosition.z / lights[i].farPlane).r;
        }
    }
    shadow /= samples * samples;
    return shadow;
}


float4 RenderPS(RenderV2P input,
                out float depth : SV_TARGET1,
                out float2 velocity : SV_TARGET2,
                out float4 specularColor : SV_TARGET3) : SV_TARGET0 {
    
	/*
	// We build the TBN frame here in order to be able to use the bump map for IBL:
    input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	//float3 bitangent = normalize(input.bitangent);
    float3 bitangent = cross(input.normal, input.tangent);
    float3x3 tbn = transpose(float3x3(input.tangent, bitangent, input.normal));

    // Transform bumped normal to world space, in order to use IBL for ambient lighting:
    float3 tangentNormal = lerp(float3(0.0, 0.0, 1.0), BumpMap(normalTex, input.texcoord), bumpiness);
    float3 normal = mul(tbn, tangentNormal);
	*/

	
	// HACK for using baked object space normals (because blender tangent export is not working)
	float3 objSpaceNormal = normalize(normalTex.Sample(AnisotropicSampler, input.texcoord).rgb - 0.5);
	objSpaceNormal.rgb = objSpaceNormal.rbg; objSpaceNormal.bg *= -1; // normal conversion
	float3 worldSpaceNormal = mul(objSpaceNormal, (float3x3) worldInverseTranspose);
	float3 normal = lerp(input.normal, worldSpaceNormal, bumpiness);
	
	
    input.view = normalize(input.view);

    // Fetch albedo, specular parameters and static ambient occlusion:
    float4 albedo = diffuseTex.Sample(AnisotropicSampler, input.texcoord);
    float3 specularAO = specularAOTex.Sample(LinearSampler, input.texcoord).rgb;

    float occlusion = specularAO.b;
    float intensity = specularAO.r * specularIntensity;
    float roughness = (specularAO.g / 0.3) * specularRoughness;

    // Initialize the output:
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    specularColor = float4(0.0, 0.0, 0.0, 0.0);

    [unroll]
    for (int i = 0; i < N_LIGHTS; i++) {
        float3 light = lights[i].position - input.worldPosition;
        float dist = length(light);
        light /= dist;

        float spot = dot(lights[i].direction, -light);
        [flatten]
        if (spot > lights[i].falloffStart) {
            // Calculate attenuation:
            float curve = min(pow(dist / lights[i].farPlane, 6.0), 1.0);
            float attenuation = lerp(1.0 / (1.0 + lights[i].attenuation * dist * dist), 0.0, curve);

            // And the spot light falloff:
            spot = saturate((spot - lights[i].falloffStart) / lights[i].falloffWidth);

            // Calculate some terms we will use later on:
            float3 f1 = lights[i].color * attenuation * spot;
            float3 f2 = albedo.rgb * f1;

            // Calculate the diffuse and specular lighting:
            float3 diffuse = saturate(dot(light, normal));
            float specular = intensity * SpecularKSK(beckmannTex, normal, light, input.view, roughness);


            // And also the shadowing:
            float shadow = ShadowPCF(input.worldPosition, i, 3, 1.0);

            // Add the diffuse and specular components:
            #ifdef SEPARATE_SPECULARS
            color.rgb += shadow * f2 * diffuse;
            specularColor.rgb += shadow * f1 * specular;
            #else
            color.rgb += shadow * (f2 * diffuse + f1 * specular);
            #endif

            // Add the transmittance component:
            if (sssEnabled && translucencyEnabled)
                color.rgb += f2 * albedo.a * SSSSTransmittance3(translucency, sssWidth, input.worldPosition, input.normal, light, shadowMaps[i], lights[i].viewProjection, lights[i].farPlane);
				//color.rgb += f2 * albedo.a * SSSSTransmittance4(translucency, sssWidth, input.worldPosition, input.normal, light, shadowMaps[i], lights[i].viewProjection, 1.0, lights[i].farPlane);
        }
    }

    // Add the ambient component:
    //color.rgb += occlusion * ambient * albedo.rgb * irradianceTex.Sample(LinearSampler, normal).rgb;
	color.rgb += occlusion * ambient * albedo.rgb;


    // Store the SSS strength:
    specularColor.a = albedo.a;

    // Store the depth value:
    depth = input.svPosition.w;

    // Convert to non-homogeneous points by dividing by w:
    input.currPosition.xy /= input.currPosition.z; // w is stored in z
    input.prevPosition.xy /= input.prevPosition.z;

    // Calculate velocity in non-homogeneous projection space:
    velocity = input.currPosition.xy - input.prevPosition.xy;

    // Compress the velocity for storing it in a 8-bit render target:
    color.a = sqrt(5.0 * length(velocity));
	
    return color;
	
	//float thikness = SSSSThikness(input.worldPosition, input.normal, shadowMaps[0], lights[0].viewProjection, lights[0].farPlane);
	
	//float3 light = lights[0].position - input.worldPosition;
    //float dist = length(light);
    //light /= dist;
	//float3 transmittance = SSSSTransmittance(translucency, sssWidth, input.worldPosition, input.normal, light, shadowMaps[0], lights[0].viewProjection, lights[0].farPlane);
	
	//return float4(transmittance, 1.0);
	//return float4(thikness,thikness,thikness, 1.0);
}

// Tangent space normal map shader used for HEAD
float4 RenderTSNMPS(RenderV2P input,
                out float depth : SV_TARGET1,
                out float2 velocity : SV_TARGET2,
                out float4 specularColor : SV_TARGET3) : SV_TARGET0 {
    // We build the TBN frame here in order to be able to use the bump map for IBL:
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    float3 bitangent = cross(input.normal, input.tangent);
    float3x3 tbn = transpose(float3x3(input.tangent, bitangent, input.normal));

    // Transform bumped normal to world space, in order to use IBL for ambient lighting:
    float3 tangentNormal = lerp(float3(0.0, 0.0, 1.0), BumpMap(normalTex, input.texcoord), bumpiness);
    float3 normal = mul(tbn, tangentNormal);
    input.view = normalize(input.view);

    // Fetch albedo, specular parameters and static ambient occlusion:
    float4 albedo = diffuseTex.Sample(AnisotropicSampler, input.texcoord);
    float3 specularAO = specularAOTex.Sample(LinearSampler, input.texcoord).rgb;

    float occlusion = specularAO.b;
    float intensity = specularAO.r * specularIntensity;
    float roughness = (specularAO.g / 0.3) * specularRoughness;

    // Initialize the output:
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    specularColor = float4(0.0, 0.0, 0.0, 0.0);

    [unroll]
    for (int i = 0; i < N_LIGHTS; i++) {
        float3 light = lights[i].position - input.worldPosition;
        float dist = length(light);
        light /= dist;

        float spot = dot(lights[i].direction, -light);
        [flatten]
        if (spot > lights[i].falloffStart) {
            // Calculate attenuation:
            float curve = min(pow(dist / lights[i].farPlane, 6.0), 1.0);
            float attenuation = lerp(1.0 / (1.0 + lights[i].attenuation * dist * dist), 0.0, curve);

            // And the spot light falloff:
            spot = saturate((spot - lights[i].falloffStart) / lights[i].falloffWidth);

            // Calculate some terms we will use later on:
            float3 f1 = lights[i].color * attenuation * spot;
            float3 f2 = albedo.rgb * f1;

            // Calculate the diffuse and specular lighting:
            float3 diffuse = saturate(dot(light, normal));
            float specular = intensity * SpecularKSK(beckmannTex, normal, light, input.view, roughness);

            // And also the shadowing:
            float shadow = ShadowPCF(input.worldPosition, i, 3, 1.0);

            // Add the diffuse and specular components:
            #ifdef SEPARATE_SPECULARS
            color.rgb += shadow * f2 * diffuse;
            specularColor.rgb += shadow * f1 * specular;
            #else
            color.rgb += shadow * (f2 * diffuse + f1 * specular);
            #endif

            // Add the transmittance component:
            if (sssEnabled && translucencyEnabled)
                color.rgb += f2 * albedo.a * SSSSTransmittance3(translucency, sssWidth, input.worldPosition, input.normal, light, shadowMaps[i], lights[i].viewProjection, lights[i].farPlane);
				//color.rgb += f2 * albedo.a * SSSSTransmittance4(translucency, sssWidth, input.worldPosition, input.normal, light, shadowMaps[i], lights[i].viewProjection, 1.0, lights[i].farPlane);
        }
    }

    // Add the ambient component:
    color.rgb += occlusion * ambient * albedo.rgb * irradianceTex.Sample(LinearSampler, normal).rgb;

    // Store the SSS strength:
    specularColor.a = albedo.a;

    // Store the depth value:
    depth = input.svPosition.w;

    // Convert to non-homogeneous points by dividing by w:
    input.currPosition.xy /= input.currPosition.z; // w is stored in z
    input.prevPosition.xy /= input.prevPosition.z;

    // Calculate velocity in non-homogeneous projection space:
    velocity = input.currPosition.xy - input.prevPosition.xy;

    // Compress the velocity for storing it in a 8-bit render target:
    color.a = sqrt(5.0 * length(velocity));

    return color;
}

void PassVS(float4 position : POSITION,
            out float4 svposition : SV_POSITION,
            inout float2 texcoord : TEXCOORD0) {
    svposition = position;
}

float4 AddSpecularPS(float4 position : SV_POSITION,
                      float2 texcoord : TEXCOORD) : SV_TARGET {
    return specularsTex.Sample(PointSampler, texcoord);
}


DepthStencilState DisableDepthStencil {
    DepthEnable = FALSE;
    StencilEnable = FALSE;
};

DepthStencilState EnableDepthStencil {
    DepthEnable = TRUE;
    StencilEnable = TRUE;
    FrontFaceStencilPass = REPLACE;
};

BlendState NoBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
    BlendEnable[1] = FALSE;
};

BlendState AddSpecularBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    BlendEnable[1] = FALSE;
    SrcBlend = ONE;
    DestBlend = ONE;
    RenderTargetWriteMask[0] = 0x7;
};

RasterizerState EnableMultisampling {
    MultisampleEnable = TRUE;
};


technique10 Render {
    pass Render {
        SetVertexShader(CompileShader(vs_4_0, RenderVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RenderPS()));
        
        SetDepthStencilState(EnableDepthStencil, id);
        SetBlendState(NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
        SetRasterizerState(EnableMultisampling);
    }
}

technique10 RenderTSNM {
    pass RenderTSNM {
        SetVertexShader(CompileShader(vs_4_0, RenderVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, RenderTSNMPS()));
        
        SetDepthStencilState(EnableDepthStencil, id);
        SetBlendState(NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
        SetRasterizerState(EnableMultisampling);
    }
}

technique10 AddSpecular {
    pass AddSpecular {
        SetVertexShader(CompileShader(vs_4_0, PassVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, AddSpecularPS()));
        
        SetDepthStencilState(DisableDepthStencil, 0);
        SetBlendState(AddSpecularBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}
