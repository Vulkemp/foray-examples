#version 460
#extension GL_KHR_vulkan_glsl : enable // Vulkan-specific syntax
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing
#extension GL_EXT_nonuniform_qualifier : enable // Required for asserting that some array indexing is done with non-uniform indices

// Include structs and bindings

#include "../../../foray/src/shaders/rt_common/bindpoints.glsl" // Bindpoints (= descriptor set layout)
// #include "../../../foray/src/shaders/common/materialbuffer.glsl" // Material buffer for material information and texture array
// #include "../../../foray/src/shaders/rt_common/geometrymetabuffer.glsl" // GeometryMeta information
// #include "../../../foray/src/shaders/rt_common/geobuffers.glsl" // Vertex and index buffer aswell as accessor methods
// #include "../../../foray/src/shaders/common/normaltbn.glsl" // Normal calculation in tangent space

// Declare hitpayloads

#define HITPAYLOAD_IN
#include "./payload.glsl"

hitAttributeEXT vec2 attribs; // Barycentric coordinates

void main()
{
    ReturnPayload.HitColor = vec3(1.0, 0.0, 0.0);
    return;

	// // Calculate barycentric coords from hitAttribute values
	// const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	
	// // Get geometry meta info
	// GeometryMeta geometa = GetGeometryMeta(uint(gl_InstanceCustomIndexEXT), uint(gl_GeometryIndexEXT));

	// // get primitive indices
	// const uvec3 indices = GetIndices(geometa, uint(gl_PrimitiveID));

	// // get primitive vertices
	// Vertex v0, v1, v2;
	// GetVertices(indices, v0, v1, v2);

	// // calculate uv
    // const vec2 uv = v0.Uv * barycentricCoords.x + v1.Uv * barycentricCoords.y + v2.Uv * barycentricCoords.z;

	// // get material 
	// MaterialBufferObject material = GetMaterialOrFallback(geometa.MaterialIndex);
	// MaterialProbe probe = ProbeMaterial(material, uv);

    // ReturnPayload.HitColor = probe.BaseColor.rgb;

	// // Calculate model and worldspace positions
	// const vec3 posModelSpace = v0.Pos * barycentricCoords.x + v1.Pos * barycentricCoords.y + v2.Pos * barycentricCoords.z;
	// const vec3 posWorldSpace = vec3(gl_ObjectToWorldEXT * vec4(posModelSpace, 1.f));
    
    // ReturnPayload.Distance = length(posWorldSpace - gl_WorldRayOriginEXT);

	// // Interpolate normal of hitpoint
	// const vec3 normalModelSpace = v0.Normal * barycentricCoords.x + v1.Normal * barycentricCoords.y + v2.Normal * barycentricCoords.z;
	// const vec3 tangentModelSpace = v0.Tangent * barycentricCoords.x + v1.Tangent * barycentricCoords.y + v2.Tangent * barycentricCoords.z;
	// const mat3 modelMatTransposedInverse = transpose(mat3(mat4x3(gl_WorldToObjectEXT)));
	// vec3 normalWorldSpace = normalize(modelMatTransposedInverse * normalModelSpace);
	// const vec3 tangentWorldSpace = normalize(tangentModelSpace);
	
	// const mat3 TBN = CalculateTBN(normalWorldSpace, tangentWorldSpace);

	// normalWorldSpace = ApplyNormalMap(TBN, probe);

}
