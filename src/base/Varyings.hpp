#pragma once
#include "Vector.hpp"

struct Varyings 
{
    float2 uv; // : TEXCOORD0;
    float3 positionWS; // : TEXCOORD1;    // xyz: posWS
    float3 positionOS;
    float4 positionCS;// : SV_POSITION;
    half4 normalWS;// : TEXCOORD2;    // xyz: normal, w: viewDir.x
    half4 tangentWS;// : TEXCOORD3;    // xyz: tangent, w: viewDir.y
    half4 bitangentWS;// : TEXCOORD4;    // xyz: bitangent, w: viewDir.z
    half  fogFactor;// : TEXCOORD5;
};
