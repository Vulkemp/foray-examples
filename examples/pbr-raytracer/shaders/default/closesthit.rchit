#version 460
#extension GL_KHR_vulkan_glsl : enable // Vulkan-specific syntax
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing
#extension GL_EXT_nonuniform_qualifier : enable // Required for asserting that some array indexing is done with non-uniform indices

// Include structs and bindings

#include "../../../../foray/src/shaders/rt_common/bindpoints.glsl" // Bindpoints (= descriptor set layout)
#include "../../../../foray/src/shaders/common/materialbuffer.glsl" // Material buffer for material information and texture array
#include "../../../../foray/src/shaders/rt_common/geometrymetabuffer.glsl" // GeometryMeta information
#include "../../../../foray/src/shaders/rt_common/geobuffers.glsl" // Vertex and index buffer aswell as accessor methods
#include "../../../../foray/src/shaders/common/normaltbn.glsl" // Normal calculation in tangent space
#include "../../../../foray/src/shaders/common/lcrng.glsl"
#include "../../../../foray/src/shaders/rt_common/tlas.glsl" // Binds Top Level Acceleration Structure

#define BIND_SIMPLIFIEDLIGHTARRAY 11
#include "../../../../foray/src/shaders/rt_common/simplifiedlights.glsl"

#include "../../../../foray/src/shaders/pbr/constants.glsl"
#include "../../../../foray/src/shaders/pbr/specularbrdf.glsl"

// Declare hitpayloads

#define HITPAYLOAD_IN
#define HITPAYLOAD_OUT
#include "../../../../foray/src/shaders/rt_common/payload.glsl"
#define VISIPAYLOAD_OUT
#include "../visibilitytest/payload.glsl"

hitAttributeEXT vec2 attribs; // Barycentric coordinates


// vec3 conductorFresnel(in vec3 f0, in vec3 bsdf, in HitSample hitSample)
// {
// 	float vDotH = dot(hitSample.OutDir, hitSample.Half);
// 	float t = pow(1 - abs(vDotH), 5);
// 	return bsdf * (f0 + (1 - f0) * t);
// }

// vec3 fresnelMix(float ior, vec3 base, vec3 layer, in HitSample hitSample, in MaterialBufferObject material) {
// 	float vDotH = dot(hitSample.OutDir, hitSample.Half);
// 	float t = pow(1 - abs(vDotH), 5);
// 	float iorTemp = ((1-material.IndexOfRefraction)/(1+material.IndexOfRefraction));
// 	float f0 = pow(iorTemp, 2);
// 	float fr = f0 + (1 - f0)*t;
// 	return mix(base, layer, fr);
// }

// vec3 metallicBrdf(in MaterialProbe probe, in HitSample hitSample)
// {
// 	vec3 f0 = probe.BaseColor.rgb;
// 	float alpha = probe.MetallicRoughness.g * probe.MetallicRoughness.g;
// 	vec3 bsdf = specularBrdf(alpha);
// 	return conductorFresnel(f0, bsdf, hitSample);
// }

struct HitSample
{
	vec3 Normal;
	vec3 wIn;
	vec3 wOut;
	vec3 wHalf;
};

float heaviside(float v)
{
	return v > 0 ? 1.0 : 0.0;
}

float specularBrdf(float alpha, in HitSample hit)
{
	float alpha2 = alpha * alpha;
	float nDotH = dot(hit.Normal, hit.wHalf);
	float nDotL = dot(hit.wIn, hit.Normal);
	float nDotV = dot(hit.wOut, hit.Normal);
	float D = (alpha2 * heaviside(nDotH)) / (PI * pow((nDotH * nDotH) * (alpha2 - 1) + 1, 2));
	float V0 = heaviside(dot(hit.wIn, hit.wHalf)) / (abs(nDotL) + sqrt(alpha2 + (1 - alpha2) * nDotL * nDotL));
	float V1 = heaviside(dot(hit.wHalf, hit.wOut)) / (abs(nDotV) + sqrt(alpha2 + (1 - alpha2) * nDotV * nDotV));
	return V0 * V1 * D;
}

vec3 diffuseBrdf(vec3 rgb)
{
	return (1 / PI) * rgb;
}

vec3 EvaluateMaterial(in HitSample hit, in MaterialProbe probe)
{
	vec3 halfVec = normalize(hit.wIn + hit.wOut);

	float tempVDotH = pow(1 - abs(dot(hit.wOut, hit.wHalf)), 5);

	vec4 baseColor = probe.BaseColor;
	float alpha = probe.MetallicRoughness.g * probe.MetallicRoughness.g;

	float t = 0;

	vec3 metalBrdf = specularBrdf(alpha, hit) * (baseColor.rgb + (1 - baseColor.rgb) * tempVDotH);

	vec3 dielectricBrdf = mix(diffuseBrdf(baseColor.rgb), baseColor.rgb * specularBrdf(alpha, hit), 0.04 + (1 - 0.04) * tempVDotH);
	return mix(dielectricBrdf, metalBrdf, probe.MetallicRoughness.r);
}

void main()
{
	// The closesthit shader is invoked with hit information on the geometry intersect closest to the ray origin
	
	// STEP #1 Get meta information on the intersected geometry (material) and the vertex information

	// Get geometry meta info
	GeometryMeta geometa = GetGeometryMeta(uint(gl_InstanceCustomIndexEXT), uint(gl_GeometryIndexEXT));
	MaterialBufferObject material = GetMaterialOrFallback(geometa.MaterialIndex);

	// get primitive indices
	const uvec3 indices = GetIndices(geometa, uint(gl_PrimitiveID));

	// get primitive vertices
	Vertex v0, v1, v2;
	GetVertices(indices, v0, v1, v2);

	// STEP #2 Calculate UV coordinates and probe the material

	// Calculate barycentric coords from hitAttribute values
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	// calculate uv
    const vec2 uv = v0.Uv * barycentricCoords.x + v1.Uv * barycentricCoords.y + v2.Uv * barycentricCoords.z;

	// Get material information at the current hitpoint
	MaterialProbe probe = ProbeMaterial(material, uv);

	// Calculate model and worldspace positions
	const vec3 posModelSpace = v0.Pos * barycentricCoords.x + v1.Pos * barycentricCoords.y + v2.Pos * barycentricCoords.z;
	const vec3 posWorldSpace = vec3(gl_ObjectToWorldEXT * vec4(posModelSpace, 1.f));

	// Interpolate normal of hitpoint
	const vec3 normalModelSpace = v0.Normal * barycentricCoords.x + v1.Normal * barycentricCoords.y + v2.Normal * barycentricCoords.z;
	const vec3 tangentModelSpace = v0.Tangent * barycentricCoords.x + v1.Tangent * barycentricCoords.y + v2.Tangent * barycentricCoords.z;
	const mat3 modelMatTransposedInverse = transpose(mat3(mat4x3(gl_WorldToObjectEXT)));
	vec3 normalWorldSpace = normalize(modelMatTransposedInverse * normalModelSpace);
	const vec3 tangentWorldSpace = normalize(tangentModelSpace);
	
	const mat3 TBN = CalculateTBN(normalWorldSpace, tangentWorldSpace);

	normalWorldSpace = ApplyNormalMap(TBN, probe);

	uint lightTestCount = min(5, SimplifiedLights.Count);

	for (uint i = 0; i < lightTestCount; i++)
	{
		lcgUint(ReturnPayload.Seed);
		uint lightIdx = ReturnPayload.Seed % SimplifiedLights.Count;

		SimplifiedLight light = SimplifiedLights.Array[lightIdx];

		vec3 origin = posWorldSpace;
		vec3 dir = light.PosOrDir - origin;
		float len = length(dir);
		float ndotl = dot(dir, normalWorldSpace);
		dir = normalize(dir);
		if (ndotl > 0)
		{
			VisiPayload.Hit = true;

		    traceRayEXT(MainTlas, // Top Level Acceleration Structure
				gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT, // RayFlags (Possible use: skip AnyHit, ClosestHit shaders etc.)
				0xff, // Culling Mask (Possible use: Skip intersection which don't have a specific bit set)
				0, // SBT record offset
				0, // SBT record stride
				1, // Miss Index
				origin, // Ray origin in world space
				0.001, // Minimum ray travel distance
				dir, // Ray direction in world space
				len, // Maximum ray travel distance
				2 // Payload index (outgoing payload bound to location 0 in payload.glsl)
			);

			if (!VisiPayload.Hit)
			{
				HitSample hit;
				hit.Normal = normalWorldSpace;
				hit.wOut = -gl_WorldRayDirectionEXT;
				hit.wIn = dir;
				hit.wHalf = normalize(hit.wOut + hit.wIn);

				vec3 bounce = EvaluateMaterial(hit, probe);

				ReturnPayload.Radiance += (ReturnPayload.Attenuation * light.Intensity * light.Color * bounce) / (len * len * lightTestCount);
			}
		}
		// ReturnPayload.Radiance = vec3(len - 7);
	}

	float cutoff = 2.f;

	if (dot(ReturnPayload.Attenuation, ReturnPayload.Attenuation) > cutoff * cutoff)
	{
		uint seed = ReturnPayload.Seed;
		for (uint i = 0; i < 2; i++)
		{
			seed += 1;
			float alpha = probe.MetallicRoughness.g * probe.MetallicRoughness.g;
			vec3 dir = sampleGGX(seed, TBN, alpha);
			vec3 origin = posWorldSpace;

			HitSample hit;
			hit.Normal = normalWorldSpace;
			hit.wOut = -gl_WorldRayDirectionEXT;
			hit.wIn = dir;
			hit.wHalf = normalize(hit.wOut + hit.wIn);

			ConstructHitPayload();
			ChildPayload.Seed = ReturnPayload.Seed + i;
			ChildPayload.Attenuation = EvaluateMaterial(hit, probe) * probe.BaseColor.rgb;

		    traceRayEXT(MainTlas, // Top Level Acceleration Structure
				0, // RayFlags (Possible use: skip AnyHit, ClosestHit shaders etc.)
				0xff, // Culling Mask (Possible use: Skip intersection which don't have a specific bit set)
				0, // SBT record offset
				0, // SBT record stride
				0, // Miss Index
				origin, // Ray origin in world space
				0.001, // Minimum ray travel distance
				dir, // Ray direction in world space
				INFINITY, // Maximum ray travel distance
				0 // Payload index (outgoing payload bound to location 0 in payload.glsl)
			);

			ReturnPayload.Radiance += ChildPayload.Radiance;
		}
	}
}
