#version 460 core
out vec4 col;
uniform vec4 objColor;
void main() {
	col = objColor;
}