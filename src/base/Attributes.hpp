#pragma once
#include "Vector.hpp"

struct Attributes 
{
    float4 positionOS; // : POSITION;
    float3 normalOS; // : NORMAL;
    float4 tangentOS; // : TANGENT;
    float2 texcoord; // : TEXCOORD0;
    float2 staticLightmapUV;
};
