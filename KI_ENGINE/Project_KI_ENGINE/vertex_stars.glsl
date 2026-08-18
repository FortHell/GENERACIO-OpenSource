#version 460 core
const vec2 v[3] = vec2[]( vec2(-1,-1), vec2(3,-1), vec2(-1,3));
out vec2 ndc;
void main() {
    ndc = v[gl_VertexID];
    // Push to the far plane in clip space; harmless since this pass draws
    // with depth test/write disabled anyway.
    gl_Position = vec4(v[gl_VertexID], 0.9999, 1.0);
}
