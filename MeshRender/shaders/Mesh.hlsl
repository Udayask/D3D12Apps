
struct UniformBuffer
{
    float4x4 mvp;
};

struct MeshInfo
{
    uint indexOffset;
    uint meshletOffset;
};

struct VertexIn
{
    float4 pos;
    float4 color;
    float2 uv;
};

struct Meshlet
{
    uint vertCount;
    uint vertOffset;
    uint primCount;
    uint primOffset;
};

struct VertexOut
{
    float4 pos   : SV_Position;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

ConstantBuffer<UniformBuffer> ubo      : register(b0);
ConstantBuffer<MeshInfo>      minfo    : register(b1);

StructuredBuffer<VertexIn> Vertices    : register(t0);
StructuredBuffer<Meshlet>  Meshlets    : register(t1);
ByteAddressBuffer UniqueVertexIndices  : register(t3);


uint3 PrimitiveAt(Meshlet m, uint localID)
{
    return uint3(0, 1, 2);
}

VertexOut VerticesAt(Meshlet m, uint localID)
{
    uint index = m.vertOffset + localID;
    uint uniqueVertexIndex = UniqueVertexIndices.Load(index * 4);
    
    VertexIn vIn = Vertices[uniqueVertexIndex];
    VertexOut vOut;
    
    vOut.pos   = mul(vIn.pos, ubo.mvp);
    vOut.color = vIn.color;
    vOut.uv    = vIn.uv;
    
    return vOut;
}

[NumThreads(128,1,1)]
[OutputTopology("triangle")]
void main(uint groupThreadID : SV_GroupThreadID, uint groupID : SV_GroupID, out indices uint3 tris[126], out vertices VertexOut verts[64])
{
    Meshlet m = Meshlets[minfo.meshletOffset + groupID];
    
    SetMeshOutputCounts(m.vertCount, m.primCount);
    
    if( groupThreadID < m.primCount )
    {
        tris[groupThreadID] = PrimitiveAt(m, groupThreadID);
    }
    
    if( groupThreadID < m.vertCount )
    {
        verts[groupThreadID] = VerticesAt(m, groupThreadID);
    }
}

