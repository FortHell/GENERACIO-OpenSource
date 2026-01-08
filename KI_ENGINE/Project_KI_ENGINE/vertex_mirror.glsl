#version 460 core
const vec2 v[3] = vec2[]( vec2(-1,-1), vec2(3,-1), vec2(-1,3));
out vec2 uv;
void main() {
    uv = (v[gl_VertexID] + 1.0) * 0.5;
    gl_Position = vec4(v[gl_VertexID],0,1);
}