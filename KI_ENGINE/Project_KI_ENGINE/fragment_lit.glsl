#version 460 core

in vec3 Normal;
in vec3 FragPos;

uniform vec3 objColor;
uniform vec3 sunDir;
uniform vec3 sunColor;

out vec4 FragColor;

void main() {
    // Basic Diffuse Lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-sunDir); 
    float diff = max(dot(norm, lightDir), 0.0);
    
    vec3 ambient = 0.2 * objColor;
    vec3 diffuse = diff * sunColor * objColor;
    
    FragColor = vec4(ambient + diffuse, 1.0);
}