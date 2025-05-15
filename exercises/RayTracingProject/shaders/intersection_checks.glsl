
// Uniforms
uniform vec3 SphereColor = vec3(0, 0, 1);
uniform vec3 SphereCenter = vec3(-3, 0, 0);
uniform float SphereRadius = 1.25f;

uniform vec3 BoxColor = vec3(1, 0, 0);
uniform mat4 BoxMatrix = mat4(1,0,0,0,   0,1,0,0,   0,0,1,0,   3,0,0,1);
uniform vec3 BoxSize = vec3(1, 1, 1);
uniform mat4 MeshMatrix = mat4(1,0,0,0,   0,1,0,0,   0,0,1,0,   3,0,0,1);

uniform vec3 MeshColor = vec3(1, 0, 0);

uniform float SphereRoughness;
uniform float BoxRoughness;
uniform float MeshRoughness;

uniform float SphereMetalness;
uniform float BoxMetalness;
uniform float MeshMetalness;

uniform vec3 LightColor = vec3(1.0f);
uniform float LightIntensity = 4.0f;
uniform vec2 LightSize = vec2(3.0f);

uniform sampler2D WoodTexture;
uniform sampler2D WallTexture;
uniform sampler2D FloorTexture;
uniform sampler2D MonaTexture;

const vec3 CornellBoxSize = vec3(10.0f);

uniform mat4 ViewMatrix;

// Materials

struct Material
{
	uint materialId;
	float roughness;
	float metalness;
	float ior;
	vec4 albedo;
	vec4 emissive;
};

layout(binding = 3, std430) readonly buffer MaterialBuffer {
    Material Materials[];
};

uniform sampler2DArray TextureArray;

Material SphereMaterial = Material(99, SphereRoughness, SphereMetalness, 0.f, vec4(SphereColor, 0.f), vec4(0.0f));
Material BoxMaterial = Material(100, BoxRoughness, BoxMetalness, 1.1f, vec4(BoxColor, 0.f),  vec4(0.0f));
Material MeshMaterial = Material(101, MeshRoughness, MeshMetalness, 0.f, vec4(MeshColor, 0.f), vec4(0.0f));

Material CornellMaterial = Material(102, 0.75f, 0.0f, 0.0f, vec4(1.0f), vec4(0.0f));
Material LightMaterial = Material(103, 0.0f, 0.0f, 0.0f, vec4(0.0f), vec4(LightIntensity * LightColor, 0.f));

// Forward declare ProcessOutput function
vec3 ProcessOutput(Ray ray, float distance, vec3 normal, Material material);

vec4 GetColorFromTexture(sampler2D sampler, vec2 uv) {
    uv = clamp(uv, vec2(0.0), vec2(1.0));
    
    return texture(sampler, uv);
}

vec4 GetColorFromTextureArray(vec2 uv, uint layer) {
	
    uv = clamp(uv, vec2(0.0), vec2(1.0));
    
    return texture(TextureArray, vec3(uv, layer));
}

// Main function for casting rays: Defines the objects in the scene
vec3 CastRay(Ray ray, inout float distance)
{
	Material material;
	vec3 normal;
	vec2 uv;
	uint materialId;

	// Sphere
	if (RaySphereIntersection(ray, SphereCenter, SphereRadius, distance, normal))
	{
		material = LightMaterial;
	}

	// Mesh
	if (RayMeshIntersection(ray, ViewMatrix, distance, normal, uv, materialId))
	{
		material = Materials[materialId];

		if (materialId == 1) 
		{
			material.albedo *= GetColorFromTexture(WallTexture, uv);
		}
		if (materialId == 2) 
		{
			material.albedo *= GetColorFromTexture(FloorTexture, uv);
		}
		if (materialId == 3)
		{
			material.albedo *= GetColorFromTexture(MonaTexture, uv);
		}
	}

	// We check if normal == vec3(0) to detect if there was a hit
	return dot(normal, normal) > 0 ? ProcessOutput(ray, distance, normal, material) : vec3(0.0f);
}

// Forward declare helper functions
vec3 GetAlbedo(Material material);
vec3 GetReflectance(Material material);
vec3 FresnelSchlick(vec3 f0, vec3 viewDir, vec3 halfDir);
vec3 GetDiffuseReflectionDirection(Ray ray, vec3 normal);
vec3 GetSpecularReflectionDirection(Ray ray, vec3 normal);
vec3 GetRefractedDirection(Ray ray, vec3 normal, float f);

// Creates a new derived ray using the specified position and direction
Ray GetDerivedRay(Ray ray, vec3 position, vec3 direction)
{
	return Ray(position, direction, ray.colorFilter, ray.ior);
}

// Produce a color value after computing the intersection
vec3 ProcessOutput(Ray ray, float distance, vec3 normal, Material material)
{
	if (distance < 0.001f) { return vec3(0.f); }

	normal = normalize(normal);

	// Find the position where the ray hit the surface
	vec3 contactPosition = ray.point + distance * ray.direction;

	// Compute the fresnel
	vec3 fresnel = FresnelSchlick(GetReflectance(material), -ray.direction, normal);

	//PushRay(shadowRay);

	// Compute transparency
	bool isTransparent = material.ior != 0.0f;
	bool isExit = ray.ior != 1.0f;
	float ior = mix(1.0f, material.ior, isTransparent && !isExit);
	vec3 refractedDirection = GetRefractedDirection(ray, normal, ray.ior / ior);

	// Add a ray to compute the diffuse lighting
	vec3 diffuseDirection = GetDiffuseReflectionDirection(ray, normal);
	Ray diffuseRay = GetDerivedRay(ray, contactPosition, isTransparent ? refractedDirection : diffuseDirection);
	if (!isExit)
	{
		diffuseRay.colorFilter *= GetAlbedo(material);
	}
	diffuseRay.colorFilter *= (1.0f - fresnel);
	diffuseRay.ior = ior;
	PushRay(diffuseRay);

	// Add a ray to compute the specular lighting
	float roughness = material.roughness * material.roughness;
	vec3 reflectedDirection = GetSpecularReflectionDirection(ray, normal);
	vec3 specularDirection = mix(reflectedDirection, diffuseDirection, roughness);
	Ray specularRay = GetDerivedRay(ray, contactPosition, specularDirection);
	specularRay.colorFilter *= fresnel;
	PushRay(specularRay);

	// Return emissive light, after applying the ray color filter
	return max(vec3(0.f), ray.colorFilter * material.emissive.xyz);
}

// Configure ray tracer
void GetRayTracerConfig(out uint maxRays)
{
	maxRays = 24u;
}
