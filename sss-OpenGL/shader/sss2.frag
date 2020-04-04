#version 410 core

#define CONSTANT_ATTENUATION 1.0
#define LINEAR_ATTENUATION 0.7
#define QUADRATIC_ATTENUATION 1.8

uniform float near = 1.0; 
uniform float far  = 100.0; 

uniform sampler2D u_texture;
uniform sampler2D u_textureColor;
uniform sampler2D u_textureNormal;

uniform float bumpiness = 0.89;
uniform float specularIntensity = 0.46;
uniform float specularRoughness = 0.3;

uniform vec4 mat_diffuse = vec4(0.0, 0.45, 0.75, 1.0);
uniform vec4  light_diffuse = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec3  ambient_color= vec3(0.04, 0.04, 0.04);
uniform vec3  specular = vec3(0.0, 0.0, 0.0);
uniform float shininess = 200.0;
uniform float ambient = 2.0;
uniform int   power = 1;
uniform float distortion = 0.4;
uniform float scale = 8.0;
uniform float light_radius = 40.0;

uniform mat4 u_projectionShadow;
uniform mat4 u_bias;
uniform mat4 u_viewShadow;
uniform mat4 u_viewShadowTex;

varying vec3 frag_eye_pos;
varying vec3 frag_eye_dir;
varying vec3 frag_eye_normal;
varying vec3 frag_eye_light_normal;
varying vec3 frag_eye_light_pos;
varying vec3 frag_eye_light_pos2;

varying vec4 sc;

varying vec3 v_normal;
varying vec3 v_tangent;
varying vec3 v_bitangent;
varying vec2 v_texCoord;
varying vec3 v_worldPosition;
varying vec3 v_view;

float saturate(float val) {
    return clamp(val, 0.0, 1.0);
}

vec3 phong_shade(vec3 light_pos, vec3 eye, vec3 normal, float attentuation) {
    vec3 L = normalize(frag_eye_light_normal);  
    vec3 N = normalize(normal);
    vec3 R = reflect(-L, N);
    vec3 E = normalize(-eye);
    
    float ld = max(dot(N, L), 0.0);
    float ls = pow(max(dot(R, E), 0.0), 0.3 * shininess);
    return (ambient_color + (ld * mat_diffuse.xyz) + (ls * specular)) * attentuation;
}

vec3 blinn_shade(vec3 light_pos, vec3 eye, vec3 normal, float attentuation) {
    vec3 L = normalize(light_pos - eye);  
    vec3 N = normalize(normal);
    vec3 V = -normalize(eye);
    float dot2 = max(dot(N, L), 0.0);
    vec3 c = mat_diffuse.xyz * light_diffuse.xyz * dot2;
    vec3 H = normalize(V + L);
    vec3 specular = vec3(clamp(6.0 * pow(max(dot(N, H), 0.0), shininess), 0.0, 1.0));
    return (c + specular) * attentuation;
	
	//return c + specular;
}

vec3 blinn_shade2(vec3 light_pos, vec3 eye, vec3 normal, float attentuation) {

	vec4 diffuse = vec4(0.0, 0.75, 0.45, 1.0);   

    vec3 L = normalize(light_pos - eye);  
    vec3 N = normalize(normal);
    vec3 V = -normalize(eye);
    float dot2 = max(dot(N, L), 0.0);
    vec3 c = diffuse.xyz * light_diffuse.xyz * dot2;
    vec3 H = normalize(V + L);
    vec3 specular = vec3(clamp(6.0 * pow(max(dot(N, H), 0.0), 2000), 0.0, 1.0));
    return (c + specular) * attentuation;
	
	//return c + specular;
}


vec4 sss(float thickness, float attentuation) {
    vec3 light = normalize(frag_eye_light_normal + (frag_eye_normal * distortion));
    float dot2 = pow(saturate(dot(frag_eye_dir, -light)), power) * scale;
    float lt = attentuation * (dot2 + ambient) * thickness;
    //lt = 1.0 - lt;
	
    return clamp((mat_diffuse * light_diffuse * lt), 0.0,1.0);
}

float getDepthPassSpaceZ(float zWC, float near, float far){

	// Assume standard depth range [0..1]
    float z_n = 2.0 * zWC - 1.0;
    float z_e =  (2.0 * near * far) / (far + near + z_n * (near - far));	//[near, far]

	//divided by far to get the range [near/far, far] just for visualisation
	//float z_e =  (2.0 * near) / (far + near + z_n * (near - far));	

	return z_e;
}

vec3 BumpMap(sampler2D normalTex, vec2 texcoord) {
    vec3 bump;
    bump.xy = texture2D( u_textureNormal, v_texCoord ).rg * 2.0 - 1.0;
    bump.z = sqrt(1.0 - bump.x * bump.x - bump.y * bump.y);
    return normalize(bump);
}


void main(){

	
   vec4 scPostW = sc/sc.w;
   float zIn =  texture2D(u_texture, scPostW.xy ).r;
   float zOut = scPostW.z;

   zIn = getDepthPassSpaceZ(zIn, 1.0, 100.0);
   zOut = getDepthPassSpaceZ(zOut, 1.0, 100.0);
 

   float thickness =   0.05 *( zOut - zIn); 
   
   
   mat3 TBN = mat3(normalize(v_normal), normalize(v_tangent), normalize(v_bitangent));
   
   vec3 tangentNormal = mix(vec3(0.0, 0.0, 1.0), BumpMap(u_textureNormal, v_texCoord), bumpiness);
   vec3 normal = TBN * tangentNormal;
   
   vec3 view = normalize(v_view);

   vec4 albedo = texture2D(u_textureColor, v_texCoord);
   vec3 specularAO = vec3(0.6, 0.2, 0.9);
   
   float occlusion = specularAO.b;
   float intensity = specularAO.r * specularIntensity;
   float roughness = (specularAO.g / 0.3) * specularRoughness;

   // Initialize the output:
   vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
   vec4 specularColor = vec4(0.0, 0.0, 0.0, 0.0);
   
   
   //thickness = (1.0 - thickness) *0.1;       
   vec3 light_dir = (frag_eye_light_pos - frag_eye_pos) / light_radius;
   float light_attentuation = max(1.0 - dot(light_dir, light_dir), 0.0); //1.0 / (CONSTANT_ATTENUATION + LINEAR_ATTENUATION * d + QUADRATIC_ATTENUATION * d * d);
   //light_attentuation = 0.6;
   //vec3 phong_color = phong_shade(frag_eye_light_pos2, frag_eye_pos, frag_eye_normal, light_attentuation);
   vec3 blinn_color = blinn_shade(frag_eye_light_pos, frag_eye_pos, frag_eye_normal, light_attentuation);
   gl_FragColor = (vec4(blinn_color, 1.0)) + sss(thickness, light_attentuation);
   
   //gl_FragColor = vec4(thickness);
   //gl_FragColor = vec4(normal, 1.0);
}
