/*

@be-material: memory-card-material {
    Tint: float3 = (1.0, 1.0, 1.0)
    EmissiveStrength: float = 1.0
    CardTexture: texture2d = white
    InputSampler: sampler = linear-clamp
}

@be-shader memory-card {
    topology triangle-list
    rasterizer none-solid
    blend disable
    depth less

    vertex VertexFunction(position, normal, color4, uv0, tangent)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main memory-card-material

    target s0 Albedo_RGB float3
    target s1 WorldNormal_XYZ float4
    target s2 ORM_RGB float4
    target s3 EmissiveRGB float3
}
*/

#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct memory_card_material {
    float3 Tint;
    float EmissiveStrength;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    memory_card_material _GeometryMain;
};

SamplerState InputSampler : register(s1, space2);
Texture2D CardTexture : register(t2, space2);

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
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
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
    output.Color = input.Color;
    output.UV = input.UV;
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float4 card = CardTexture.Sample(InputSampler, input.UV);
    if (card.a < 0.05) discard;

    float3 normal = normalize(input.Normal);
    float3 color = card.rgb * _GeometryMain.Tint;

    PixelOutput output;
    output.Albedo_RGB = color * 0.05;
    output.WorldNormal_XYZ = float4(normal, 0.0);
    output.ORM_RGB = float4(1.0, 0.8, 0.0, 0.0);
    output.EmissiveRGB = color * _GeometryMain.EmissiveStrength;
    return output;
}
