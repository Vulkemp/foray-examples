#version 460
#extension GL_GOOGLE_include_directive : enable // Include files
#extension GL_EXT_ray_tracing : enable // Raytracing

#define VISIPAYLOAD_IN
#include "payload.glsl"

void main()
{
    ReturnPayload.Hit = false;
}
