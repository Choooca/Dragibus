#version 450

layout (location = 0) out vec4 out_color;

layout (location = 0) in vec2 tex_coord;

void main(){
	out_color = vec4(tex_coord, 0.0f, 0.0f);
}