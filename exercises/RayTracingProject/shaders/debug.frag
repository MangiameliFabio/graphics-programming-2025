#version 450

layout(location = 64) out vec4 fragColor;

layout(std430, binding = 0) buffer Triangles {
    vec3 triangles[];
};

uniform mat4 InvProjMatrix;  // inverse projection matrix
uniform vec2 resolution;     // screen size (e.g., vec2(1024, 1024))

struct Ray {
    vec3 origin;
    vec3 direction;
};

bool RayTriangleIntersection(vec3 ro, vec3 rd, vec3 v0, vec3 v1, vec3 v2, out float t) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rd, edge2);
    float a = dot(edge1, h);

    if (abs(a) < 1e-5) return false;

    float f = 1.0 / a;
    vec3 s = ro - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return false;

    vec3 q = cross(s, edge1);
    float v = f * dot(rd, q);
    if (v < 0.0 || u + v > 1.0) return false;

    t = f * dot(edge2, q);
    return t > 0.0;
}

void main()
{
    // 1. Get screen UV [-1, 1]
    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    vec4 clip = vec4(uv, -1.0, 1.0);

    // 2. Reconstruct view-space ray direction
    vec4 view = InvProjMatrix * clip;
    vec3 dir = normalize(view.xyz);

    Ray ray;
    ray.origin = vec3(0.0);  // camera is at origin in view space
    ray.direction = dir;

    // 3. Get first triangle
    vec3 v0 = triangles[0];
    vec3 v1 = triangles[1];
    vec3 v2 = triangles[2];

    float t;
    bool hit = RayTriangleIntersection(ray.origin, ray.direction, v0, v1, v2, t);

    fragColor = hit ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(0.0);
}
