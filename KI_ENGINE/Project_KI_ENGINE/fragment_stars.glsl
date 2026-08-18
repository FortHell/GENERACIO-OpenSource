#version 460 core

in vec2 ndc;
out vec4 FragColor;

uniform mat4 invViewProj;
uniform vec3 camPos;
uniform float uTime;

vec3 hash33(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453123);
}

// Picks a per-star tint. Every star can land white or slightly warm/yellow.
// When extraHues is true, a minority of stars instead lean slightly blue,
// green, or red, for palette variety. h2.z picks which bucket a star falls
// into (fixed per-star, since h2 comes from that star's own hash), h2.y
// blends how strongly the tint leans that direction.
vec3 starTint(vec3 h2, bool extraHues) {
    vec3 white = vec3(1.0);
    vec3 warm  = vec3(1.0, 0.92, 0.75); // slightly yellow
    vec3 blue  = vec3(0.1, 0.1, 1.0); // slightly blue
    vec3 green = vec3(0.2, 1.0, 0.2); // slightly green
    vec3 red   = vec3(1.0, 0.15, 0.15);   // slightly red

    if (!extraHues) {
        return mix(white, warm, h2.y);
    }

    if (h2.z < 0.7)      return mix(white, warm,  h2.y);       // 70%: white/yellow
    else if (h2.z < 0.8) return blue;
    else if (h2.z < 0.9) return green;
    else                 return red;
}

// One layer of the star field. Stars are placed by hashing a 3D grid built
// from the (scaled) view direction, so they stay fixed in world space
// regardless of head rotation -- checking the 3x3x3 neighborhood avoids
// stars/halos getting clipped at cell boundaries.
//
// cellSize   - higher = more, smaller/denser stars
// threshold  - fraction of cells that contain a star (density)
// coreSize   - radius of the sharp point, in cell-space units
// haloSize   - radius of the faint glow around it
// brightness - overall intensity of this layer
// twinkleSpd - how fast stars drift in brightness (kept low = slow/subtle)
// twinkleAmt - how deep the dip goes, 0 = no twinkle, 1 = fades to black
// extraHues  - false: white/slightly-yellow only. true: mostly white/yellow,
//              with a chance of slightly blue, green, or red.
vec3 starLayer(vec3 dir, float cellSize, float threshold, float coreSize,
               float haloSize, float brightness, float twinkleSpd, float twinkleAmt,
               bool extraHues) {
    vec3 p = dir * cellSize;
    vec3 cell = floor(p);
    vec3 result = vec3(0.0);

    // Anti-aliasing width, computed ONCE and unconditionally, before any
    // data-dependent branching below. fwidth() needs uniform control flow
    // across a 2x2 pixel quad -- calling it per-candidate-star inside the
    // loop (behind the threshold "continue") desyncs neighboring pixels
    // that land in different cells, and the derivative comes back garbage.
    // That was the cause of the streaky line artifacts.
    float aa = max(length(fwidth(p)), 1e-4);

    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec3 offset = vec3(x, y, z);
        vec3 h  = hash33(cell + offset);
        vec3 h2 = hash33(cell + offset + 19.19);

        // Only some cells contain a star at all
        if (h.x > threshold) continue;

        vec3 starPos = cell + offset + h; // jittered position within its cell
        float d = length(starPos - p);

        // Slow per-star twinkle; each star gets its own phase. Oscillates
        // between (1 - twinkleAmt) and 1.0, so twinkleAmt controls how
        // visible the dip is independently of speed.
        float osc = 0.5 + 0.5 * sin(uTime * twinkleSpd + h2.x * 6.2831);
        float twinkle = mix(1.0 - twinkleAmt, 1.0, osc);

        float core = 1.0 - smoothstep(coreSize - aa, coreSize + aa, d);
        float halo = (1.0 - smoothstep(haloSize - aa, haloSize + aa, d)) * 0.12;

        vec3 tint = starTint(h2, extraHues);

        result += (core + halo) * brightness * twinkle * tint;
    }

    return result;
}

void main() {
    // Reconstruct a world-space ray direction for this pixel from the
    // inverse view-projection matrix (far-plane point minus camera position).
    vec4 worldFar = invViewProj * vec4(ndc, 1.0, 1.0);
    worldFar /= worldFar.w;
    vec3 dir = normalize(worldFar.xyz - camPos);

    vec3 small = starLayer(dir, 100.0, 0.1, 0.03, 0.02, 0.55, 1.9, 0.75, false);
    vec3 big   = starLayer(dir, 55.0,  0.05, 0.085, 0.02, 1.00, 1.15, 0.6, true);

    vec3 col = small + big;
    FragColor = vec4(col, 1.0);
}
