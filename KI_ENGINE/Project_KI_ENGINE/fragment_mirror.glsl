#version 460 core
in vec2 uv;
out vec4 col;
uniform sampler2DArray texArr;
void main() {
	col = texture(texArr, vec3(uv,0));
}