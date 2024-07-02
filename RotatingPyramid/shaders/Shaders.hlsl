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
    matrix mvp;
};

ConstantBuffer<UniformBuffer0> cb0 : register(b0);

VsOutput VsMain(VsInput v)
{
    VsOutput output;
    
    output.position = mul(cb0.mvp, float4(v.position, 1.0f));
    output.color    = v.color;
    output.uv       = v.uv;
    
    return output;
}

Texture2D<float4> colorTexture : register(t0);
SamplerState      colorSampler : register(s0);

float4 PsMain(VsOutput vo) : SV_Target
{
    float4 tex = colorTexture.Sample(colorSampler, vo.uv);
    return tex + float4(vo.color, 1.0f);
}
