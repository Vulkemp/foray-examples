#version 460
#extension GL_KHR_vulkan_glsl : enable // Vulkan-specific syntax
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing
#extension GL_EXT_nonuniform_qualifier : enable // Required for asserting that some array indexing is done with non-uniform indices

// Include structs and bindings

#include "../../../foray/src/shaders/rt_common/bindpoints.glsl" // Bindpoints (= descriptor set layout)
#include "../../../foray/src/shaders/common/materialbuffer.glsl" // Material buffer for material information and texture array
#include "../../../foray/src/shaders/rt_common/geometrymetabuffer.glsl" // GeometryMeta information
#include "../../../foray/src/shaders/rt_common/geobuffers.glsl" // Vertex and index buffer aswell as accessor methods
// #include "../../../foray/src/shaders/common/normaltbn.glsl" // Normal calculation in tangent space

// Declare hitpayloads

#define HITPAYLOAD_IN
#include "./payload.glsl"

hitAttributeEXT vec2 attribs; // Barycentric coordinates

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

	// Simply write out the base color as the hit color
    ReturnPayload.HitColor = probe.BaseColor.rgb;
}
