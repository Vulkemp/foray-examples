#version 460
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing

// Declare hitpayloads

#define HITPAYLOAD_IN
#include "payload.glsl"

void main()
{
    ReturnPayload.HitColor = vec3(0.0, 0.0, 0.0);
}