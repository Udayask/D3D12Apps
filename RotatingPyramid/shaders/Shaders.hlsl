struct VsInput
{
    float3 position : POSITION;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD;
};

struct VsOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD;
};

struct UniformBuffer0
{
    float4x4 mvp;
};

ConstantBuffer<UniformBuffer0> cb0 : register(b0);

VsOutput VsMain(VsInput v)
{
    VsOutput output;
    
    output.position = mul(float4(v.position, 1.0f), cb0.mvp);
    output.color    = v.color;
    output.uv       = v.uv;
    
    return output;
}

Texture2D<float4> colorTexture : register(t0);
SamplerState      colorSampler : register(s0);

float4 PsMain(VsOutput vo) : SV_Target
{
    return float4(vo.color, 1.0f);
}
