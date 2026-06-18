/*

@be-material: memory-sleeve-material {
    GlowColor: float3 = (1.0, 1.0, 1.0)
    GlowStrength: float = 1.45
    SunDirection: float3 = (-0.45, -1.0, -0.25)
}

@be-shader memory-sleeve {
    topology triangle-list
    rasterizer none-solid
    blend additive
    depth less-equal

    vertex VertexFunction(position, normal, color4, uv0, tangent)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 geometry-object object-material-for-geometry-pass
    bind s2 geometry-main memory-sleeve-material

    target s0 HDR float4
}
*/

#include "uniform-material.hlsl"
#include "objectMaterial.hlsl"

struct memory_sleeve_material {
    float3 GlowColor;
    float GlowStrength;
    float3 SunDirection;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    object_material_for_geometry_pass _GeometryObject;
};

cbuffer CBuffer_2 : register(b0, space2) {
    memory_sleeve_material _GeometryMain;
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
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD1;
};

struct PixelOutput {
    float4 HDR : SV_Target0;
};

Interpolators VertexFunction(VertexInput input) {
    float4 worldPosition = mul(float4(input.Position, 1.0), _GeometryObject.Model);
    float3x3 normalMatrix = (float3x3)_GeometryObject.Model;

    Interpolators output;
    output.Position = mul(worldPosition, _GeometryObject.ProjectionView);
    output.WorldPosition = worldPosition.xyz;
    output.Normal = normalize(mul(input.Normal, normalMatrix));
    output.Tangent = float4(normalize(mul(input.Tangent.xyz, normalMatrix)), input.Tangent.w);
    output.UV = input.UV;
    return output;
}

PixelOutput PixelFunction(Interpolators input) {
    float3 normal = normalize(input.Normal);
    float3 tangent = normalize(input.Tangent.xyz);
    float3 bitangent = normalize(cross(normal, tangent) * input.Tangent.w);
    float3 viewDir = normalize(_Frame.CameraPosition - input.WorldPosition);
    float3 sunDir = normalize(-_GeometryMain.SunDirection);
    float3 halfDir = normalize(viewDir + sunDir);

    float facing = saturate(abs(dot(normal, viewDir)));
    float fresnel = pow(1.0 - facing, 3.0);
    float sunFacing = saturate(dot(normal, sunDir));
    float specFacing = pow(saturate(dot(normal, halfDir)), 18.0);

    float2 uv = saturate(input.UV);
    float2 halfLocal = float2(dot(halfDir, tangent), dot(halfDir, bitangent));
    float2 lightAxis = normalize(halfLocal + float2(0.002, -0.001));
    float2 crossAxis = float2(-lightAxis.y, lightAxis.x);
    float2 centeredUV = uv - float2(0.5, 0.5);

    float alongLight = dot(centeredUV, lightAxis);
    float acrossLight = dot(centeredUV, crossAxis);
    float sheetOffset = dot(halfLocal, crossAxis) * 0.12;

    float broadSheet = exp(-pow((acrossLight - sheetOffset) / 0.58, 2.0));
    float fullSurfaceVeil = 0.18 + broadSheet * 0.42;
    float lightFalloff = 0.74 + 0.26 * saturate(0.5 + alongLight * 0.9);
    float broadSheen = fullSurfaceVeil * lightFalloff;

    float softSpecular = pow(saturate(1.0 - abs(acrossLight - sheetOffset) / 0.72), 2.2);

    float surfaceGrain =
        sin(uv.x * 31.0 + uv.y * 11.0) * 0.018 +
        sin(uv.x * 17.0 - uv.y * 23.0) * 0.014;

    float edgeFade =
        smoothstep(0.00, 0.08, uv.x) *
        smoothstep(0.00, 0.08, uv.y) *
        smoothstep(0.00, 0.08, 1.0 - uv.x) *
        smoothstep(0.00, 0.08, 1.0 - uv.y);

    float sunResponse = 0.18 + sunFacing * 0.48 + specFacing * 1.05;
    float glow = (broadSheen * 0.54 + softSpecular * 0.22 + fresnel * 0.16 + surfaceGrain) * edgeFade * sunResponse;
    glow = max(glow, 0.0) * _GeometryMain.GlowStrength;

    PixelOutput output;
    output.HDR = float4(_GeometryMain.GlowColor * glow, 1.0);
    return output;
}
