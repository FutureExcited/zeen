/*

@be-material: memory-button-material {
    BaseColor: float3 = (0.08, 0.08, 0.09)
    EmissiveColor: float3 = (0.0, 0.0, 0.0)
    EmissiveStrength: float = 0.0
}

@be-shader memory-button {
    topology triangle-list
    rasterizer back-solid
    blend disable
    depth less

    vertex VertexFunction(position, normal, color4, uv0, tangent)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main memory-button-material

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 ORM_RGB float4
    target s3 EmissiveRGB float3
}
*/

#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct memory_button_material {
    float3 BaseColor;
    float3 EmissiveColor;
    float EmissiveStrength;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    memory_button_material _GeometryMain;
};

struct VertexInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float4 Tangent : TANGENT;
};

struct Interpolators {
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 LocalPosition : TEXCOORD0;
};

struct PixelOutput {
    float3 Albedo_RGB : SV_Target0;
    float4 WorldNormal_XYZ : SV_Target1;
    float4 ORM_RGB : SV_Target2;
    float3 EmissiveRGB : SV_Target3;
};

Interpolators VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);
    float3x3 normalMatrix = (float3x3)_GeometryObject.Model;

    Interpolators output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.Normal = normalize(mul(input.Normal, normalMatrix));
    output.LocalPosition = input.Position;
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float3 normal = normalize(input.Normal);
    float radial = length(input.LocalPosition.xz);
    float rim = smoothstep(0.40, 0.50, radial);
    float capEdge = smoothstep(0.38, 0.50, abs(input.LocalPosition.y));
    float bevel = saturate(max(rim, capEdge * 0.72));
    float topLight = saturate(normal.y * 0.5 + 0.5);
    float centerLift = 1.0 - smoothstep(0.0, 0.40, radial);
    float rimLine = 1.0 - smoothstep(0.0, 0.045, abs(radial - 0.42));
    float3 color = lerp(_GeometryMain.BaseColor * 1.16, _GeometryMain.BaseColor * 0.56, bevel);
    color += _GeometryMain.BaseColor * (centerLift * 0.20 + topLight * 0.12);
    color += _GeometryMain.EmissiveColor * rimLine * 0.12;

    PixelOutput output;
    output.Albedo_RGB = color;
    output.WorldNormal_XYZ = float4(normal, 0.0);
    output.ORM_RGB = float4(1.0, 0.48, 0.0, 0.0);
    output.EmissiveRGB = _GeometryMain.EmissiveColor * _GeometryMain.EmissiveStrength * (0.42 + centerLift * 0.58 + rimLine * 0.32);
    return output;
}
