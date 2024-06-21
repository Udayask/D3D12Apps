struct VsInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VsOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD;
};

struct VsConstant0
{
    float4x4 mvpMatrix;
};

ConstantBuffer<VsConstant0> vsCB : register(b0);

VsOutput VsMain(VsInput v)
{
    VsOutput output;
    
    output.position = mul(float4(v.position, 1.0f), vsCB.mvpMatrix);
    output.uv = v.uv;
    
    return output;
}

Texture2D<float4> colorTexture : register(t0);
SamplerState      colorSampler : register(s0);

float4 PsMain(VsOutput vx) : SV_Target
{
    return colorTexture.Sample(colorSampler, vx.uv);
}