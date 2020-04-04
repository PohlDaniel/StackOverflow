#version 410 core

layout (location = 0) out float outDepth;
layout (location = 1) out vec2  outVelocity;
layout (location = 2) out vec4  outSpecularColor;
layout (location = 3) out vec4  outColor;

uniform float near = 1.0; 
uniform float far  = 100.0; 

uniform sampler2D u_texture;
uniform sampler2DShadow u_shadow;
uniform sampler2D u_textureColor;
uniform sampler2D u_textureNormal;
uniform sampler2D u_beckmannTex;

uniform float bumpiness = 0.89;
uniform float specularIntensity = 0.46;
uniform float specularRoughness = 0.3;
uniform float specularFresnel = 0.81;

uniform vec3 lightPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor = vec3(1.2, 1.2, 1.2);

uniform float lightFalloffStart;
uniform float lightFarPlane = 100.0;
uniform float lightAttenuation = 1.0 / 128.0;
uniform float lightFalloffWidth = 0.05;
uniform float lightBias = -0.01;

uniform bool sssEnabled = true;
uniform bool translucencyEnabled = true;

uniform float sssWidth;
uniform float translucency = 0.0;
uniform float ambient = 0.0;

varying vec3 v_normal;
varying vec3 v_tangent;
varying vec3 v_bitangent;
varying vec2 v_texCoord;
varying vec3 v_worldPosition;
varying vec3 v_view;
varying vec4 sv_position;

varying vec4 sc;

float getDepthPassSpaceZ(float zWC, float near, float far){

	// Assume standard depth range [0..1]
    float z_n = 2.0 * zWC - 1.0;
    float z_e =  (2.0 * near * far) / (far + near + z_n * (near - far));	//[near, far]

	//divided by far to get the range [near/far, 1.0] just for visualisation
	//float z_e =  (2.0 * near) / (far + near + z_n * (near - far));	

	return z_e;
}

vec3 BumpMap(sampler2D normalTex, vec2 texcoord) {
    vec3 bump;
    bump.xy = texture2D( u_textureNormal, v_texCoord ).rg * 2.0 - 1.0;
    bump.z = sqrt(1.0 - bump.x * bump.x - bump.y * bump.y);
    return normalize(bump);
}

float Fresnel(vec3 half, vec3 view, float f0) {
    float base = 1.0 - dot(view, half);
    float exponential = pow(base, 5.0);
    return exponential + f0 * (1.0 - exponential);
}

float SpecularKSK(sampler2D beckmannTex, vec3 normal, vec3 light, vec3 view, float roughness) {
    vec3 half = view + light;
    vec3 halfn = normalize(half);

    float ndotl = max(dot(normal, light), 0.0);
    float ndoth = max(dot(normal, halfn), 0.0);

    float ph = pow(2.0 * texture2D(beckmannTex, vec2(ndoth, roughness)).r, 10.0);
    float f = mix(0.25, Fresnel(halfn, view, 0.028), specularFresnel);
    float ksk = max(ph * f / dot(half, half), 0.0);

    return ndotl * ksk;    
}

float ShadowPCF(vec4 _shadowPosition) {

	vec4 shadowPosition = _shadowPosition;
    shadowPosition.xyz /= shadowPosition.w;
     
	vec2 UVCoords;
    UVCoords.x = 0.5 * shadowPosition.x + 0.5;
    UVCoords.y = 0.5 * shadowPosition.y + 0.5;
    float z = 0.5 * shadowPosition.z + 0.5;
	z += lightBias;

    float w = 1024.0;
	float h = 1024.0;
   
    float xOffset = 1.0/w;
    float yOffset = 1.0/h;
   
	float shadow = 0.0;

    for (int y = -1 ; y <= 1 ; y++) {
        for (int x = -1 ; x <= 1 ; x++) {
            vec2 Offsets = vec2(x * xOffset, y * yOffset);
            vec3 UVC = vec3(UVCoords + Offsets, z);
            shadow += texture(u_shadow, UVC);
        }
    }

    return shadow;
}



const vec3 weights[5] = vec3[5](
	vec3(0.153845, 0.159644, 0.01224),
	vec3(0.019712, 0.010854, 0.383008),
	vec3(0.048265, 0.402794, 0.022724),
	vec3(0.42143, 0.021578, 0.161395),
	vec3(0.009353, 0.050785, 0.051702)
);

const vec3 variances[5] = vec3[5](
	vec3(0.243373, 0.19885, 0.000128),
	vec3(0.001579, 0.000128, 1.76679),
	vec3(0.016459, 2.29636, 0.001403),
	vec3(3.10841, 0.001473, 0.164507),
	vec3(0.000131, 0.014631, 0.013066)
);

vec3 SSSSTransmittance( float translucency, float sssWidth, vec3 worldNormal, vec3 light, sampler2D shadowMap, vec4 shadowPos){

   float scale = sssWidth * (1.0 - translucency);

   vec4 scPostW = shadowPos/shadowPos.w;
   float zIn =  texture2D(u_texture, scPostW.xy ).r;
   float zOut = scPostW.z;

   zIn = getDepthPassSpaceZ(zIn, 1.0, 100.0);
   zOut = getDepthPassSpaceZ(zOut, 1.0, 100.0);
 
   float thickness =   scale *( zOut - zIn); 

   float dd = -thickness * thickness;

   vec3 profile = 	 weights[0] * exp(dd / variances[0]) +
					 weights[1] * exp(dd / variances[1]) +
					 weights[2] * exp(dd / variances[2]) +
					 weights[3] * exp(dd / variances[3]) +
					 weights[4] * exp(dd / variances[4]);

   /** 
    * Using the profile, we finally approximate the transmitted lighting from
    * the back of the object:
    */	 
   return profile;

}

void main(){

	
   vec4 scPostW = sc/sc.w;
   float zIn =  texture2D(u_texture, scPostW.xy ).r;
   float zOut = scPostW.z;

   zIn = getDepthPassSpaceZ(zIn, 1.0, 100.0);
   zOut = getDepthPassSpaceZ(zOut, 1.0, 100.0);
 

   float thickness =   0.05 *( zOut - zIn); 
   
   vec3 t = normalize(v_tangent);
   vec3 b = normalize(v_bitangent);
   vec3 n = normalize(v_normal);
	
   //not transposed to push view- and light- vector to normal space
   //mat3 TBN = mat3(t.x, b.x, n.x,
   //				 t.y, b.y, n.y,
   //				 t.z, b.z, n.z);
    
   //OpenGL will build the matrix  mat3 TBN = mat3( t, b, n) in a column major style
   //mat3 TBN = mat3(t.x, t.y, t.z,
   //				 b.x, b.y, b.z,
   //				 n.x, n.y, n.z);
   
   //transposed to push the tangent-space-normal back to view-space
   mat3 TBN = mat3( t, b, n); 
   
   
   vec3 tangentNormal = mix(vec3(0.0, 0.0, 1.0), BumpMap(u_textureNormal, v_texCoord), bumpiness);
   vec3 normal = TBN * tangentNormal; 
   //vec3 normal = normalize(TBN * normalize(texture2D( u_textureNormal, v_texCoord ).rgb * 2.0 - 1.0)) ;  
     
   vec3 view = normalize(v_view);
   
   vec4 albedo = texture2D(u_textureColor, v_texCoord);
   vec3 specularAO = vec3(0.6, 0.2, 0.9);
   
   float occlusion = specularAO.b;
   float intensity = specularAO.r * specularIntensity;
   float roughness = (specularAO.g / 0.3) * specularRoughness;

   // Initialize the output:
   outColor = vec4(0.0, 0.0, 0.0, 0.0);
   outSpecularColor = vec4(0.0, 0.0, 0.0, 0.0);
   		
		vec3 light = lightPosition - v_worldPosition;
		float dist = length(light);
		light /= dist;
   
		float spot = dot(lightDirection, -light);
		if (spot > lightFalloffStart) {
			// Calculate attenuation:
			
			 float curve = min(pow(dist / lightFarPlane, 6.0), 1.0);
             float attenuation = mix(1.0 / (1.0 + lightAttenuation * dist * dist), 0.0, curve);

             // And the spot light falloff:
             spot = clamp((spot - lightFalloffStart) / lightFalloffWidth, 0.0, 1.0);
			 
			 // Calculate some terms we will use later on:
			 vec3 f1 = lightColor * attenuation * spot;
             vec3 f2 = albedo.rgb * f1;

			 // Calculate the diffuse and specular lighting:
             vec3 diffuse = vec3(clamp(dot(light, normal) , 0.0, 1.0));
			 

             float specular = intensity * SpecularKSK(u_beckmannTex, normal, light, v_view, roughness);
			 specular = 0.0;
			 
			 // And also the shadowing:
             float shadow = ShadowPCF(sc);
			 shadow = 1.0;
			 
			 outColor.rgb += shadow * f2 * diffuse;
			 
			  if (sssEnabled && translucencyEnabled){

				outColor.rgb += f2 * albedo.a * SSSSTransmittance(translucency, sssWidth, normalize(v_normal), light, u_texture, sc);
			  }
		}
 
	// Add the ambient component:
    outColor.rgb += occlusion * ambient * albedo.rgb ;
 
	// Store the SSS strength:
    outSpecularColor.a = albedo.a;
	
    // Store the depth value:
    outDepth = sv_position.w;
}
