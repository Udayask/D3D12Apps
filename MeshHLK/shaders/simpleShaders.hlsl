//=================================================================================================================================
// Vertex Shader VS_simple
//=================================================================================================================================
struct Vtx 
{ 
    float4 pos : SV_Position; 
};

Vtx MakeVertex() 
{ 
    Vtx result = 
    { 
        float4(0,0,0,0) 
    }; 
    return result; 
}

Vtx VS_simple (uint id : SV_VERTEXID) 
{ 
    Vtx result; 
    result.pos = float4 ( id , 0 , 0 , 1 ); 
    return result; 
}

//=================================================================================================================================
// Pixel Shader PS_simple
//=================================================================================================================================

float4 PS_simple() : SV_TARGET0 
{ 
    return float4(1,1,1,1); 
}

//=================================================================================================================================
// Geometry Shader GS_simple
//=================================================================================================================================

[maxvertexcount(3)] 
void GS_simple (triangle Vtx input[3], inout TriangleStream<Vtx> result) 
{ 
    result.Append( input[0] ); 
}

//=================================================================================================================================
// Hull Shader HS_simple
//=================================================================================================================================

struct PatchConstants 
{ 
    float TessFactor[3] : SV_TessFactor; 
    float InsideTess    : SV_InsideTessFactor; 
};
    
PatchConstants hs_patch ( InputPatch<Vtx, 3> patch) 
{ 
    PatchConstants ret = {{0, 0, 0}, 0}; 
    return ret;
}

[patchconstantfunc("hs_patch")] 
[domain("tri")] 
[partitioning("integer")]
[outputtopology("triangle_cw")] 
[maxtessfactor(64.0)] 
[outputcontrolpoints(3)]
Vtx HS_simple(InputPatch<Vtx, 3> patch, uint cpid : SV_OutputControlPointID) 
{ 
    return patch[cpid]; 
}

//=================================================================================================================================
// Domain Shader DS_simple
//=================================================================================================================================

[domain("tri")] 
Vtx DS_simple(OutputPatch<Vtx, 3> patch, PatchConstants patchConst, float3 location : SV_DomainLocation) 
{ 
    return MakeVertex(); 
}

//=================================================================================================================================
// Compute Shader CS_simple
//=================================================================================================================================    

[numthreads(1, 1, 1)] 
void CS_simple(uint3 tid : SV_DispatchThreadID) 
{ 

}