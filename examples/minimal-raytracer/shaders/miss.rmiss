#version 460
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing

// Declare hitpayloads

#define HITPAYLOAD_IN // This defines the payload as coming from a parent shader invocation (input variable and return variable of this shader)
#include "payload.glsl"

void main()
{
    // The hit shader is invoked, when no geometry has been hit. We could render a skybox in here
    ReturnPayload.HitColor = vec3(0.0, 0.0, 0.0);
}