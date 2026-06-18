/*

@be-material: memory-floor-material {
    BaseColor: float3 = (0.10, 0.11, 0.12)
    WalkwayColor: float3 = (0.16, 0.165, 0.16)
    GridColor: float3 = (0.23, 0.235, 0.225)
    WalkwayWidth: float = 2.75
    GridScale: float = 1.0
    GridLineWidth: float = 0.025
}

@be-shader memory-floor {
    topology triangle-list
    rasterizer back-solid
    blend disable
    depth less

    vertex VertexFunction(position, normal, color4, uv0, tangent)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main memory-floor-material

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 ORM_RGB float4
    target s3 EmissiveRGB float3
}
*/

#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct memory_floor_material {
    float3 BaseColor;
    float3 WalkwayColor;
    float3 GridColor;
    float WalkwayWidth;
    float GridScale;
    float GridLineWidth;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    memory_floor_material _GeometryMain;
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
    float3 WorldPosition : TEXCOORD0;
    float3 Normal : NORMAL;
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
    output.WorldPosition = worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, normalMatrix));
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float2 world = input.WorldPosition.xz;
    float halfWalkway = _GeometryMain.WalkwayWidth * 0.5;
    float absX = abs(world.x);

    float walkwayMask = 1.0 - smoothstep(halfWalkway - 0.12, halfWalkway + 0.18, absX);
    float edgeBand = 1.0 - smoothstep(0.0, 0.11, abs(absX - halfWalkway));

    float2 tileUV = float2(world.x * 0.62, world.y * 0.34) * _GeometryMain.GridScale;
    float2 tileCell = abs(frac(tileUV) - 0.5);
    float tileLine = step(0.5 - _GeometryMain.GridLineWidth, max(tileCell.x, tileCell.y));

    float panelPulse = 0.5 + 0.5 * sin(world.y * 1.15);
    float centerWear = 1.0 - smoothstep(0.0, halfWalkway, absX);
    float fineVariation =
        sin(world.x * 8.3 + world.y * 1.7) * 0.018 +
        sin(world.x * 2.1 - world.y * 4.9) * 0.012;

    float3 sideFloor = _GeometryMain.BaseColor + fineVariation;
    float3 walkway = _GeometryMain.WalkwayColor + centerWear * 0.055 + panelPulse * 0.018;
    float3 color = lerp(sideFloor, walkway, walkwayMask);
    color = lerp(color, _GeometryMain.GridColor, tileLine * lerp(0.26, 0.40, walkwayMask));

    float seam = edgeBand * 0.10;
    color += float3(seam, seam, seam) * walkwayMask;

    PixelOutput output;
    output.Albedo_RGB = color;
    output.WorldNormal_XYZ = float4(normalize(input.Normal), 0.0);
    output.ORM_RGB = float4(1.0, 0.72, 0.0, 0.0);
    output.EmissiveRGB = float3(0.0, 0.0, 0.0);
    return output;
}
