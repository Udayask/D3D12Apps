Texture2D<float4>   SourceTexture       : register(t0);
RWTexture2D<float4> DestinationTexture  : register(u0);
SamplerState        BilinearClamp       : register(s0);

cbuffer CB : register(b0)
{
    float2 texSize;
}

[numthreads(8, 8, 1)]
void GenMips(uint3 tid : SV_DispatchThreadID)
{
    float2 uv  = texSize * (tid.xy + 0.5f);
    float4 col = SourceTexture.Sample(BilinearClamp, uv, 0);
    
    DestinationTexture[tid.xy] = col;
}
