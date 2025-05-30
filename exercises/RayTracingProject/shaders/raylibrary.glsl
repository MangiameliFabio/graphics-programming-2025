struct Triangle {
    vec4 v0;
    vec4 v1;
    vec4 v2;

    vec4 normal0;  
    vec4 normal1;  
    vec4 normal2;   

    vec2 uv0;
    vec2 uv1;
    vec2 uv2;

    uint materialId;
    uint transformId;
};

layout(binding = 1, std430) readonly buffer Triangles {
    Triangle triangles[];
};

layout(binding = 2, std430) readonly buffer MeshTransforms {
    mat4 meshTransforms[];
};

// Test intersection between a ray and a sphere
bool RaySphereIntersection(Ray ray, vec3 center, float radius, inout float distance, inout vec3 normal)
{
	bool hit = false;

	//vec3 m = TransformToLocalPoint(ray.point, center);
	vec3 m = ray.point - center;

	float b = dot(m, ray.direction); 
	float c = dot(m, m) - radius * radius; 

	if (c <= 0.0f || b <= 0.0f)
	{
		float discr = b * b - c;
		if (discr >= 0.0f)
		{
			float sqrtDiscr = sqrt(discr);
			float d = -b - sqrtDiscr;
			float flipNormal = 1.0f;
			if (d < 0.0f)
			{
				d = -b + sqrtDiscr;
				flipNormal = -1.0f;
			}

			if (d < distance)
			{
				distance = d;

				vec3 point = m + d * ray.direction;
				normal = flipNormal * normalize(point);

				hit = true;
			}
		}
	}

	return hit;
}


// Test intersection between a ray and an axis aligned box
bool RayAABBIntersection(Ray ray, vec3 size, inout float distance, inout vec3 normal)
{
	bool hit = false;

	vec3 dirSign = mix(vec3(-1), vec3(1), greaterThanEqual(ray.direction, vec3(0)));
	vec3 distancesMin = (-dirSign * size - ray.point) / ray.direction;
	vec3 distancesMax = ( dirSign * size - ray.point) / ray.direction;

	float distanceMin = max(distancesMin.x, max(distancesMin.y, distancesMin.z));
	float distanceMax = min(distancesMax.x, min(distancesMax.y, distancesMax.z));

	if (distanceMin < distanceMax)
	{
		if (distanceMin < 0)
		{
			distanceMin = distanceMax;
			distancesMin = distancesMax;
		}

		if (distanceMin > 0 && distanceMin < distance)
		{
			distance = distanceMin;
			normal = mix(vec3(0.0f), -dirSign, equal(distancesMin, vec3(distance)));

			hit = true;
		}
	}
	return hit;
}

// Test intersection between a ray and a box
bool RayBoxIntersection(Ray ray, mat4 matrix, vec3 size, inout float distance, inout vec3 normal)
{
	ray.point = (inverse(matrix) * vec4(ray.point, 1)).xyz;
	ray.direction = (inverse(matrix) * vec4(ray.direction, 0)).xyz;
	bool hit = RayAABBIntersection(ray, size, distance, normal);
	if (hit)
	{
		normal = (matrix * vec4(normal, 0)).xyz;
	}

	return hit;
}

bool RayTriangleIntersection( vec3 ro, vec3 rd, vec3 v0, vec3 v1, vec3 v2, inout float distance , inout float hitU, inout float hitV)
{
    vec3 v1v0 = v1-v0, v2v0 = v2-v0, rov0 = ro-v0;
 
    vec3  n = cross( v1v0, v2v0 );
    vec3  q = cross( rov0, rd );
    float d = 1.0/dot(  n, rd );
    float u =   d*dot( -q, v2v0 );
    float v =   d*dot(  q, v1v0 );
    float t =   d*dot( -n, rov0 );

	if (u < 0.0 || v < 0.0 || (u + v) > 1.0) return false;
    if (t < 0.0) return false;

	distance = t;
	hitU = u;
	hitV = v;

    return true;
}

bool RayMeshIntersection(Ray ray, mat4 view, inout float distance, inout vec3 normal, inout vec2 uv, inout uint material)
{
	uint lastTransformId = uint(-1);
	mat4 modelMatrix, modelViewMatrix, invModelViewMatrix;
	Ray localRay;
	bool hit = false;

	for (int i = 0; i < triangles.length(); ++i) {
		uint currentTransformId = triangles[i].transformId;

		if (currentTransformId != lastTransformId) {
			lastTransformId = currentTransformId;
			modelMatrix = meshTransforms[currentTransformId];
			modelViewMatrix = view * modelMatrix;
			invModelViewMatrix = inverse(modelViewMatrix);

			localRay.point = (invModelViewMatrix * vec4(ray.point, 1.0)).xyz;
			localRay.direction = normalize((invModelViewMatrix * vec4(ray.direction, 0.0)).xyz);
		}

		vec3 v0 = triangles[i].v0.xyz;
		vec3 v1 = triangles[i].v1.xyz;
		vec3 v2 = triangles[i].v2.xyz;

		float t, u, v;
		if (RayTriangleIntersection(localRay.point, localRay.direction, v0, v1, v2, t, u, v) && t < distance) {
			distance = t;
			hit = true;

			vec3 n0 = triangles[i].normal0.xyz;
			vec3 n1 = triangles[i].normal1.xyz;
			vec3 n2 = triangles[i].normal2.xyz;

			vec3 localNormal = normalize(n0 * (1.0 - u - v) + n1 * u + n2 * v);

			if (ray.ior != 1.0f && dot(localNormal, ray.direction) > 0.0)
			{
				localNormal = -localNormal;
			}

			vec2 uv0 = triangles[i].uv0;
			vec2 uv1 = triangles[i].uv1;
			vec2 uv2 = triangles[i].uv2;

			normal = normalize((modelViewMatrix * vec4(localNormal, 0.f)).xyz);
			uv = uv0 * (1.0 - u - v) + uv1 * u + uv2 * v;
			material = triangles[i].materialId;
		}
	}

	return hit;
}
