#version 410 core

layout(location = 0) in vec3 i_position;


uniform mat4 u_projection;
uniform mat4 u_modelView;


varying float dist;

void main(void){

  gl_Position =   u_projection * u_modelView * vec4(i_position, 1.0); 
  vec4 position = (u_modelView * vec4(i_position, 1.0));
  
  dist = length(position.xyz);
}