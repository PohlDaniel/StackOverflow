#version 410 core

uniform mat4 modelview;
uniform mat4 projection;
uniform vec3 light_pos; // world space
uniform vec3 light_pos2;

uniform mat4 u_projection;
uniform mat4 u_modelView;
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_normalMatrix;

uniform mat4 u_projectionShadow;
uniform mat4 u_viewShadow;

uniform vec3 cameraPosition;


layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_texCoord;
layout(location = 2) in vec3 i_normal;
layout(location = 3) in vec3 i_tangent;
layout(location = 4) in vec3 i_bitangent;

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

void main(){    

	v_normal  = (u_normalMatrix * vec4(i_tangent, 0.0)).xyz;
	v_tangent = (u_normalMatrix * vec4(i_bitangent, 0.0)).xyz;
	v_bitangent = (u_normalMatrix * vec4(i_normal, 0.0)).xyz;
	v_texCoord = i_texCoord;
	v_worldPosition = (u_model * vec4(i_position, 1.0)).xyz;
	v_view = cameraPosition - v_worldPosition;

    frag_eye_pos = (u_modelView * vec4(i_position, 1.0)).xyz;
    frag_eye_dir = normalize(-frag_eye_pos);
    frag_eye_normal = normalize((u_normalMatrix * vec4(i_normal, 0.0)).xyz); 
    
    frag_eye_light_pos = (u_view * vec4(light_pos, 1.0)).xyz; // <-- assumes view matrix == identity matrix   //(modelview * light_pos).xyz;
    frag_eye_light_normal = normalize(frag_eye_light_pos - frag_eye_pos);
    
    frag_eye_light_pos2 = (u_view * vec4(light_pos2, 1.0)).xyz; 
    
	
    gl_Position = u_projection *  u_modelView  * vec4(i_position, 1.0);
	
	
	
	
	vec3 shrinkedPos = i_position - 0.005 * normalize(i_normal);
	sc = u_projectionShadow   * u_viewShadow * u_model * vec4(shrinkedPos, 1.0);
    //sc.xyz = sc.xyz*0.5 + vec3(0.5)*sc.w;
}
