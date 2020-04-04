#version 410 core

varying float dist;

out float color;
//out vec3 color;

void main(void){
   
  color = dist;
   
  // color = vec3(0.5, 0.5, 0.5);
}