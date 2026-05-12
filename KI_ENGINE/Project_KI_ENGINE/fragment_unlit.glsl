#version 460 core

in vec3 Normal;
in vec3 FragPos;

uniform vec3 objColor;

out vec4 FragColor;

void main() {
    FragColor = vec4(objColor, 1.0);
}