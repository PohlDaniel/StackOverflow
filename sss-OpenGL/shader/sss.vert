#version 410 core

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


varying vec3 v_normal;
varying vec3 v_tangent;
varying vec3 v_bitangent;
varying vec2 v_texCoord;
varying vec3 v_worldPosition;
varying vec3 v_view;
varying vec4 sv_position;

varying vec4 sc;


void main(){    

	v_normal  = (u_normalMatrix * vec4(i_normal, 0.0)).xyz;
	v_tangent = (u_normalMatrix * vec4(i_tangent, 0.0)).xyz;
	v_bitangent = (u_normalMatrix * vec4(i_bitangent, 0.0)).xyz;
	v_texCoord = i_texCoord;
	v_worldPosition = (u_model * vec4(i_position, 1.0)).xyz;
	v_view = cameraPosition - v_worldPosition;


    gl_Position = u_projection *  u_modelView  * vec4(i_position, 1.0);
	sv_position = gl_Position;
	
	
	
	vec3 shrinkedPos = i_position - 0.005 * normalize(i_normal);
	sc = u_projectionShadow   * u_viewShadow * u_model * vec4(shrinkedPos, 1.0);

}
