#version 460 core

in vec3 Normal;
in vec3 FragPos;

uniform vec3 sunDir;
uniform vec3 sunColor;
uniform float noiseScale;   // higher = smaller/more numerous "continents"
uniform vec3 noiseOffset;   // shifts the noise field so different planets don't look identical
uniform vec3 bandColor1;    // darkest band (valleys)
uniform vec3 bandColor2;
uniform vec3 bandColor3;
uniform vec3 bandColor4;    // lightest band (highlands)

out vec4 FragColor;

// Gradient hash for classic Perlin noise. Returns a pseudo-random direction
// per grid cell corner (each hashed component remapped to [-1, 1]).
vec3 hash3(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

// Classic (gradient) 3D Perlin noise, roughly in [-1, 1].
float perlin3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    // Quintic smoothing curve (Perlin's improved fade function) avoids the
    // visible grid-aligned creasing that plain linear/cubic blending gives.
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    return mix(
        mix(
            mix(dot(hash3(i + vec3(0,0,0)), f - vec3(0,0,0)),
                dot(hash3(i + vec3(1,0,0)), f - vec3(1,0,0)), u.x),
            mix(dot(hash3(i + vec3(0,1,0)), f - vec3(0,1,0)),
                dot(hash3(i + vec3(1,1,0)), f - vec3(1,1,0)), u.x), u.y),
        mix(
            mix(dot(hash3(i + vec3(0,0,1)), f - vec3(0,0,1)),
                dot(hash3(i + vec3(1,0,1)), f - vec3(1,0,1)), u.x),
            mix(dot(hash3(i + vec3(0,1,1)), f - vec3(0,1,1)),
                dot(hash3(i + vec3(1,1,1)), f - vec3(1,1,1)), u.x), u.y),
        u.z);
}

// Fractal Brownian Motion: stacks several octaves of Perlin noise at
// increasing frequency and decreasing amplitude, which is what gives
// noise a "continent-like" look (broad shapes with finer detail on top)
// instead of one layer of uniform blobs.
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; i++) {
        value += amplitude * perlin3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

void main() {
    vec3 norm = normalize(Normal);

    // Sampling by surface normal (not FragPos) means the pattern is fixed
    // to the sphere's surface and unaffected by the planet's position or
    // scale in the world -- no seams, no stretching.
    float n = fbm(norm * noiseScale + noiseOffset);
    n = clamp(n * 0.5 + 0.5, 0.0, 1.0); // remap roughly [-1,1] -> [0,1]

    vec3 surfaceColor;
    if (n < 0.35)      surfaceColor = mix(bandColor1, bandColor2, smoothstep(0.0, 0.35, n));
    else if (n < 0.6)  surfaceColor = mix(bandColor2, bandColor3, smoothstep(0.35, 0.6, n));
    else if (n < 0.8)  surfaceColor = mix(bandColor3, bandColor4, smoothstep(0.6, 0.8, n));
    else               surfaceColor = bandColor4;

    // Same diffuse/ambient lighting model as fragment_lit.glsl
    vec3 lightDir = normalize(-sunDir);
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 ambient = 0.2 * surfaceColor;
    vec3 diffuse = diff * sunColor * surfaceColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}
