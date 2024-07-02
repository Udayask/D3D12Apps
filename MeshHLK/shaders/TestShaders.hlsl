#include "ShaderAndAppCommon.h"
#define RootSig      "RootFlags(0)," \
                     "RootConstants(num32BitConstants=3,b0)," /* [Root slot 0] RTWidth, RTHeight, RTArrayDepth */ \
                     "UAV(u0),"                               /* [Root slot 1] Log UAV (only some tests use this) */

cbuffer dim : register(b0)
{
    uint RTWidth;
    uint RTHeight;
    uint RTDepth;
};

#define VERT_BLOAT_SIZE 24
#define VERT_BLOAT_SIZE_VRS 20
struct MSvertNoBloat
{
    float2 tex : TEX;
    float4 pos : SV_POSITION;
};

struct MSvert
{
    float bloat[VERT_BLOAT_SIZE] : BLOAT;
    float2 tex : TEX;
    float4 pos : SV_POSITION;
};
struct VSvert
{
    float4 pos : SV_POSITION;
    float2 tex : TEX;
    nointerpolation uint   rtIndex : SV_RenderTargetArrayIndex;
    float bloat[VERT_BLOAT_SIZE] : BLOAT;
    nointerpolation uint   colorOffset : COLOROFFSET;
};
struct VSvert_VRS
{
    float4 pos : SV_POSITION;
    float2 tex : TEX;
    nointerpolation uint   rtIndex : SV_RenderTargetArrayIndex;
    float bloat[VERT_BLOAT_SIZE_VRS] : BLOAT;
    nointerpolation uint   colorOffset : COLOROFFSET;
    nointerpolation uint shadingRate : SV_SHADINGRATE;
};

#define VERT_BLOAT_SIZE_VIEW_INSTANCING 8

struct MSvertViewInstancing
{
    float bloat[VERT_BLOAT_SIZE_VIEW_INSTANCING] : BLOAT;
    float2 tex : TEX;
    float4 pos : SV_POSITION;
};
struct VSvertViewInstancing
{
    float4 pos : SV_POSITION;
    float2 tex : TEX;
    nointerpolation uint   rtIndex : SV_RenderTargetArrayIndex;
    float bloat[VERT_BLOAT_SIZE_VIEW_INSTANCING] : BLOAT;
    nointerpolation uint   colorOffset : COLOROFFSET;
};


//=================================================================================================================================
// Helper: CalcPos: Make a quad vertex position and UV for one corner of one quad that is part of a grid of quads.
//
// vertInQuadClockwiseFromTopLeft specifies which vertex:
//
//   0---------1
//   |         |
//   |         |
//   |         |
//   |         |
//   3---------2
//
//
// quadX specifies how far to shift the quad to the right based on renderTargetWidth RTWidth (one quad per pixel)
// quadY specifies how far to shift the quad down based on renderTargetHeight RTHeight (one quad per pixel)
// if bViewInstancing is true, divide RTWidth into VIEW_INSTANCING_TEST_NUM_VIEWS columns and put quad into column[viewID]
//=================================================================================================================================
float4 CalcPos(uint quadX, uint quadY, uint vertInQuadClockwiseFromTopLeft, out float2 tex, bool bViewInstancing = false, uint viewID = 0)
{
    uint perViewWidth = bViewInstancing ? RTWidth / VIEW_INSTANCING_TEST_NUM_VIEWS : RTWidth;
    float delX = 2.0f / (float)RTWidth;
    float delY = -2.0f / (float)RTHeight;

    // Initialize to top left
    uint2 quadCoord = uint2(quadX % perViewWidth, (quadX / perViewWidth) + (quadY % RTHeight));

    switch(vertInQuadClockwiseFromTopLeft)
    {
    case 0:
        // nothing to do        
        break;
    case 1:
        quadCoord.x += 1;
        break;
    case 2:
        quadCoord.x += 1;
        quadCoord.y += 1;
        break;
    case 3:
        quadCoord.y += 1;
        break;
    }
    if(bViewInstancing)
    {
        quadCoord.x += viewID*perViewWidth;
    }
    float4 pos = float4(-1.0 + delX*quadCoord.x,
                        1.0 + delY*quadCoord.y,
                        0.5,1);
    float u = (pos.x + 1.0) / 2;
    float v = -(pos.y - 1.0) / 2;
    tex = float2(u,v);
    return pos;
}

#define CalcPosWorker() \
{ \
    vtx.pos = CalcPos(quadX,quadY,vertInQuadClockwiseFromTopLeft,vtx.tex,bViewInstancing,viewID); \
}

void CalcPos(uint quadX, uint quadY, uint vertInQuadClockwiseFromTopLeft, out MSvertNoBloat vtx, bool bViewInstancing, uint viewID)
{
    CalcPosWorker();
}

void CalcPos(uint quadX, uint quadY, uint vertInQuadClockwiseFromTopLeft, out VSvert vtx, bool bViewInstancing, uint viewID)
{
    CalcPosWorker();
}

// CalcPosLine() generates endpoints of a line starting at the center of a pixel and
// ending on the bottom right corner of the pixel.  Since this line "exits" the virtual
// diamond within the pixel (touching middle of 4 pixel edges), in the line rasterization's 
// diamond-exit rule, the pixel is covered.
float4 CalcPosLine(uint lineStartX, uint lineStartY, uint vertInLine, out float2 tex)
{
    float delX = 2.0f / (float)RTWidth;
    float delY = -2.0f / (float)RTHeight;

    // Initialize to center of top left pixel
    float4 pos = float4(-1.0 + delX*(float)(lineStartX % RTWidth) + delX*0.5,
                         1.0 + delY*(float)((lineStartX / RTWidth) + (lineStartY % RTHeight)) + delY*0.5,
                        0.5,1);

    if(vertInLine)
    {
        // End of line is on bottom right corner of line
        pos.x += delX*0.5;
        pos.y += delY*0.5;
    }
    float u = (pos.x + 1.0) / 2;
    float v = -(pos.y - 1.0) / 2;
    tex = float2(u,v);
    return pos;
}
//=================================================================================================================================
// Helper: Make a part of a mesh
//=================================================================================================================================
struct MSprim
{
    uint   colorOffset_ : COLOROFFSET;
    uint   rtIndex : SV_RenderTargetArrayIndex;
    bool   cullPrimitive : SV_CullPrimitive;
};

struct MSprim_VRS
{
    uint   colorOffset_ : COLOROFFSET;
    uint   rtIndex : SV_RenderTargetArrayIndex;
    uint   shadingRate : SV_ShadingRate;
};

groupshared MSvertNoBloat gs_verts[LARGE_TEST_NUM_VERTS_PER_GROUP]; // using these for all test sizes (since more than big enough)
groupshared MSprim gs_prims[LARGE_TEST_NUM_PRIMS_PER_GROUP];
groupshared MSprim_VRS gs_prims_VRS[LARGE_TEST_NUM_PRIMS_PER_GROUP];
groupshared uint3 gs_idx[LARGE_TEST_NUM_PRIMS_PER_GROUP];

//   CalcMesh: Generate a row of quads (one pixel per quad) as indexed triangle list into group shared memory.
//   numQuads: How many quads to generate
//   y : How far down in pixels to generate the row
//   arraySlice : which RT array slice to send the quad to
//   numDispatchGroups : used for color calculations when arraySlice > 0
//   threadInGroup:  current thread in group.  If y == 0, generate verts (x dictates which range),
//                                             If y == 1, generate indices and primitive data (x dictates which range)
//                                             exception, if group height is only 1, same threads do verts, indices and prims
//   threadsWidth/Height:    total number of threads that will help, to inform how to split up the work
//             
//   0---------2---------4---------6---------8----
//   |       / |       / |       / |       / | 
//   |     /   |     /   |     /   |     /   |     ... etc
//   |   /     |   /     |   /     |   /     |   /
//   | /       | /       | /       | /       | /
//   1---------3---------5---------7---------9----
void CalcMesh(
    uint numQuads,
    uint y,
    uint arraySlice,
    uint numDispatchGroups,
    uint3 threadInGroup,
    uint threadsWidth,
    uint threadsHeight,
    bool bViewInstancing = false,
    uint viewID = 0,
    bool bVariableRateShading = false,
    bool bCullAll = false) 
{
    if((threadInGroup.y) == 0 && (threadInGroup.x == 0)) // generate top left and bottom left verts for leftmost quad
    {
        CalcPos(0,y,0,gs_verts[0],bViewInstancing,viewID);
        CalcPos(0,y,3,gs_verts[1],bViewInstancing,viewID);
    }

    uint quadsPerThread = max(1,(uint)ceil((float)numQuads / (float)threadsWidth));
    for(uint currQuad = 0; currQuad < quadsPerThread; currQuad++)
    {
        uint quadIndex = threadInGroup.x * quadsPerThread + currQuad;
        if(quadIndex >= numQuads)
        {
            break;
        }
        uint startVertex = quadIndex*2;
        if(threadInGroup.y == 0)
        {
            for(uint i = 0; i < 2; i++) // generate top right and bottom right verts for current quad
            {
                CalcPos(quadIndex,y,i+1,gs_verts[startVertex+2+i],bViewInstancing,viewID);
            }
        }
        if((threadsHeight == 1) || (threadInGroup.y == 1))
        {
            uint startPrimitive = quadIndex*2;
            bool bCullPrimitive = (bViewInstancing && (viewID == 2)) ? true : false;
            if (bCullAll)
            {
                bCullPrimitive = true;
            }
            gs_idx[startPrimitive]   = uint3(startVertex + 0, startVertex + 2, startVertex + 1);
            gs_idx[startPrimitive+1] = uint3(startVertex + 2, startVertex + 3, startVertex + 1);
            if (!bVariableRateShading)
            {
                gs_prims[startPrimitive].colorOffset_ = quadIndex + y + arraySlice * numDispatchGroups;
                gs_prims[startPrimitive].rtIndex = arraySlice;
                gs_prims[startPrimitive].cullPrimitive = bCullPrimitive;
                gs_prims[startPrimitive + 1].colorOffset_ = quadIndex + y + arraySlice * numDispatchGroups;
                gs_prims[startPrimitive + 1].rtIndex = arraySlice;
                gs_prims[startPrimitive + 1].cullPrimitive = bCullPrimitive;
            }
            else {
                gs_prims_VRS[startPrimitive].colorOffset_ = quadIndex + y + arraySlice * numDispatchGroups;
                gs_prims_VRS[startPrimitive].rtIndex = arraySlice;
                gs_prims_VRS[startPrimitive].shadingRate = 0x5;
                gs_prims_VRS[startPrimitive + 1].colorOffset_ = quadIndex + y + arraySlice * numDispatchGroups;
                gs_prims_VRS[startPrimitive + 1].rtIndex = arraySlice;
                gs_prims_VRS[startPrimitive + 1].shadingRate = 0x5;
            }
        }
    }
    GroupMemoryBarrierWithGroupSync();
}

//=================================================================================================================================
// Helper: CalcOverlappingTriPos: Make a tri vertex position
//
// vertInQuadClockwiseFromTopLeft specifies which vertex:
//
//       0
//       ..  
//      .  .
//     .    .
//    .      .
//   2........1
//
//=================================================================================================================================
float4 CalcOverlappingTriPos(uint vertInTriClockwiseFromTop)
{
    float delX = 2.0f / (float)RTWidth;
    float delY = -2.0f / (float)RTHeight;

    // Initialize to top left
    float4 pos = float4(-1.0,1.0,0.5,1);

    switch(vertInTriClockwiseFromTop)
    {
    case 0:
        pos.x += (delX*0.5);
        break;
    case 1:
        pos.x += delX;
        pos.y += delY;
        break;
    case 2:
        pos.y += delY;
        break;
    }
    return pos;
}

//   CalcMesh2: Generate a set of overlapping triangles, each covering one pixel, as indexed triangle list into group shared memory.
//   numTris: How many quads to generate
//   threadInGroup:  current thread in group.  If y == 0, generate verts (x dictates which range),
//                                             If y == 1, generate indices and primitive data (x dictates which range)
//                                             exception, if group height is only 1, same threads do verts, indices and prims
//   threadsWidth/Height:    total number of threads that will help, to inform how to split up the work
//             
//       0,3,6...
//       ..   
//      .  .  
//     .    . 
//    .      .
//   2........1,4,7,...
void CalcOverlappingMesh(uint numTris, uint3 threadInGroup, uint threadsWidth)
{
    if(threadInGroup.x < 3) 
    {
        gs_verts[threadInGroup.x].pos = CalcOverlappingTriPos(threadInGroup.x);
    }

    uint trisPerThread = max(1,(uint)ceil((float)numTris / (float)threadsWidth));
    for(uint currTri = 0; currTri < trisPerThread; currTri++)
    {
        uint triIndex = threadInGroup.x * trisPerThread + currTri;
        if(triIndex >= numTris)
        {
            break;
        }
        gs_idx[triIndex]   = uint3(0,1,2);
    }
    GroupMemoryBarrierWithGroupSync();
}


// Generate a line per pixel for a row of pixels.
void CalcLineMesh(uint numLines, uint y, uint arraySlice, uint numDispatchGroups, uint3 threadInGroup, uint threadsWidth, uint threadsHeight)
{
    uint linesPerThread = max(1,(uint)ceil((float)numLines / (float)threadsWidth));
    for(uint currLine = 0; currLine < linesPerThread; currLine++)
    {
        uint lineIndex = threadInGroup.x * linesPerThread + currLine;
        if(lineIndex >= numLines)
        {
            break;
        }
        uint startVertex = lineIndex*2;
        if(threadInGroup.y == 0)
        {
            for(uint i = 0; i < 2; i++) 
            {
                gs_verts[startVertex + i].pos = CalcPosLine(lineIndex,y,i,gs_verts[startVertex + i].tex);
            }
        }
        if((threadsHeight == 1) || (threadInGroup.y == 1))
        {
            uint startPrimitive = lineIndex;
            gs_idx[startPrimitive] = uint3(startVertex + 0, startVertex + 1,0);
            gs_prims[startPrimitive].colorOffset_ = lineIndex + y + arraySlice*numDispatchGroups;
            gs_prims[startPrimitive].rtIndex = arraySlice;
            gs_prims[startPrimitive].cullPrimitive = false;
        }
    }
    GroupMemoryBarrierWithGroupSync();
}

//=================================================================================================================================
// Helper: Dump geometry from shared mem to mesh shader output.
// Macros because functions can't pass arrays by reference.
//=================================================================================================================================
// DumpGeometryOneToOne() uses thread x to write output arrays indexed as vertex[x], and index[x], prim[x] only.
// This simple scenario is easiest for some known hardware to handle.

#define DumpVert(vtx,off,bigBloat) \
{ \
    vtx[off].pos = gs_verts[off].pos; \
    vtx[off].tex = gs_verts[off].tex; \
    uint arraySize = bigBloat ? VERT_BLOAT_SIZE : VERT_BLOAT_SIZE_VIEW_INSTANCING; \
    for(uint i = 0; i < arraySize; i++) /*generate vertex attribute bloat*/\
    { \
        vtx[off].bloat[i] = gs_verts[off].pos.x + gs_verts[off].pos.y + (float)i; \
    } \
}

#define DumpGeometryOneToOne(groupWidth, bigBloat) \
{ \
    const uint numVertices = groupWidth; \
    const uint numPrimitives = groupWidth-2; \
    SetMeshOutputCounts(numVertices,numPrimitives); \
    if(threadInGroup.y == 0 && threadInGroup.z == 0) \
    { \
        if(threadInGroup.x < numVertices) \
        { \
            DumpVert(verts,threadInGroup.x, bigBloat); \
        } \
        if(threadInGroup.x < numPrimitives) \
        { \
            prims[threadInGroup.x] = gs_prims[threadInGroup.x]; \
            idx[threadInGroup.x] = gs_idx[threadInGroup.x]; \
        } \
    } \
}

#define DumpGeometryOneToOne_VRS(groupWidth) \
{ \
    const uint numVertices = groupWidth; \
    const uint numPrimitives = groupWidth-2; \
    SetMeshOutputCounts(numVertices,numPrimitives); \
    if(threadInGroup.y == 0 && threadInGroup.z == 0) \
    { \
        if(threadInGroup.x < numVertices) \
        { \
            DumpVert(verts,threadInGroup.x, true); \
        } \
        if(threadInGroup.x < numPrimitives) \
        { \
            prims_VRS[threadInGroup.x] = gs_prims_VRS[threadInGroup.x]; \
            idx[threadInGroup.x] = gs_idx[threadInGroup.x]; \
        } \
    } \
}

#define DumpLineGeometryOneToOne(groupWidth) \
{ \
    const uint numVertices = groupWidth; \
    const uint numPrimitives = groupWidth/2; \
    SetMeshOutputCounts(numVertices,numPrimitives); \
    if(threadInGroup.y == 0 && threadInGroup.z == 0) \
    { \
        if(threadInGroup.x < numVertices) \
        { \
            DumpVert(verts,threadInGroup.x,true); \
        } \
        if(threadInGroup.x < numPrimitives) \
        { \
            prims[threadInGroup.x] = gs_prims[threadInGroup.x]; \
            idx[threadInGroup.x] = (uint2)gs_idx[threadInGroup.x]; \
        } \
    } \
}

// DumpGeometryLarge() writes an arbitrary amount of vertices and indices/prim data out
#define DumpGeometryLarge(groupWidth,numVertices,numPrimitives) \
{ \
    SetMeshOutputCounts(numVertices,numPrimitives); \
    if((threadInGroup.y == 0) && (threadInGroup.z == 0)) \
    { \
        for(uint currVertRangeStart = 0; currVertRangeStart < numVertices; currVertRangeStart += groupWidth ) \
        { \
            uint currVert = currVertRangeStart + threadInGroup.x; \
            if(currVert >= numVertices) \
            { \
                break; \
            } \
            DumpVert(verts,currVert,true); \
        } \
        for(uint currPrimRangeStart = 0; currPrimRangeStart < numPrimitives; currPrimRangeStart += groupWidth ) \
        { \
            uint currPrim = currPrimRangeStart + threadInGroup.x; \
            if(currPrim >= numPrimitives) \
            { \
                break; \
            } \
            prims[currPrim] = gs_prims[currPrim]; \
            idx[currPrim] = gs_idx[currPrim]; \
        } \
    } \
}

//=================================================================================================================================
// Amplification Shader ASSmallPayload
//=================================================================================================================================
struct smallPayload
{
    uint arraySlice;
    uint numGroups;
};

[numthreads(1,1,1)]
void ASSmallPayload(in uint3 groupID : SV_GroupID)   
{
    smallPayload p;
    p.arraySlice = groupID.z;
    p.numGroups = SMALL_TEST_NUM_GROUPS;
    DispatchMesh(1, SMALL_TEST_NUM_GROUPS,1,p);
}                  

//=================================================================================================================================
// Amplification Shader ASLargePayload
//=================================================================================================================================
#define PAYLOAD_BLOAT_SIZE 4094 // picked to make largePayload's total size 16K (maximum allowed)
struct largePayload
{
    float bloat[PAYLOAD_BLOAT_SIZE];
    uint arraySlice;
    uint numGroups;
};

[numthreads(8,8,2)]
void ASLargePayload(in uint3 groupID : SV_GroupID)   
{
    largePayload p;
    for(uint i = 0; i < PAYLOAD_BLOAT_SIZE; i++)
    {
        p.bloat[i] = i + RTWidth;
    }
    p.arraySlice = groupID.z;
    p.numGroups = MEDIUM_AND_LARGE_TEST_NUM_GROUPS;
    DispatchMesh(1,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,1,p);
}                  

// Do something with payload so it hopefully doesn't get optimized away
// Macro to avoid passing a large array into a function (HLSL can't pass by ref for now)
#define CalcArraySliceFromLargePayload() \
{ \
    uint accum = 1; \
    for(uint i = 0; i < PAYLOAD_BLOAT_SIZE; i++) \
    { \
        accum += min(10,p.bloat[i] * accum); \
    } \
    if(accum > PAYLOAD_BLOAT_SIZE) /* will be true */ \
    { \
        arraySlice = p.arraySlice; \
    } \
}

//=================================================================================================================================
// Mesh Shader MSSmall
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(SMALL_TEST_GROUP_X_DIMENSION,SMALL_TEST_GROUP_Y_DIMENSION,1)]
void MSSmall(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[SMALL_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[SMALL_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[SMALL_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcMesh(SMALL_TEST_NUM_QUADS_PER_GROUP,groupID.y,0,SMALL_TEST_NUM_GROUPS,threadInGroup,SMALL_TEST_GROUP_X_DIMENSION,SMALL_TEST_GROUP_Y_DIMENSION);
    DumpGeometryOneToOne(SMALL_TEST_GROUP_X_DIMENSION,true);
}                  

//=================================================================================================================================
// Mesh Shader MSMedium 
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION,1)]
void MSMedium(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[MEDIUM_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[MEDIUM_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcMesh(MEDIUM_TEST_NUM_QUADS_PER_GROUP,groupID.y,0,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION);
    DumpGeometryOneToOne(MEDIUM_TEST_GROUP_X_DIMENSION,true);
}                  

//=================================================================================================================================
// Mesh Shader MSMediumLine
//=================================================================================================================================
[outputtopology("line")]
[numthreads(MEDIUM_LINE_TEST_GROUP_X_DIMENSION,MEDIUM_LINE_TEST_GROUP_Y_DIMENSION,1)]
void MSMediumLine(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[MEDIUM_LINE_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_LINE_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint2 idx[MEDIUM_LINE_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcLineMesh(MEDIUM_LINE_TEST_NUM_PRIMS_PER_GROUP,groupID.y,0,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,MEDIUM_LINE_TEST_GROUP_X_DIMENSION,MEDIUM_LINE_TEST_GROUP_Y_DIMENSION);
    DumpLineGeometryOneToOne(MEDIUM_LINE_TEST_GROUP_X_DIMENSION);
}

//=================================================================================================================================
// Mesh Shader MSViewInstancing
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION,1)]
void MSViewInstancing(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in uint viewID : SV_ViewID,
    out vertices MSvertViewInstancing verts[MEDIUM_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[MEDIUM_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcMesh(MEDIUM_TEST_NUM_QUADS_PER_GROUP,groupID.y,0,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION,true,viewID);
    DumpGeometryOneToOne(MEDIUM_TEST_GROUP_X_DIMENSION,false);
}

//=================================================================================================================================
// Mesh Shader MSVariableRateShading
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(MEDIUM_TEST_GROUP_X_DIMENSION, MEDIUM_TEST_GROUP_Y_DIMENSION, 1)]
void MSVariableRateShading(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[MEDIUM_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim_VRS prims_VRS[MEDIUM_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[MEDIUM_TEST_NUM_PRIMS_PER_GROUP]
)
{
    CalcMesh(MEDIUM_TEST_NUM_QUADS_PER_GROUP, groupID.y, 0, MEDIUM_AND_LARGE_TEST_NUM_GROUPS, threadInGroup, MEDIUM_TEST_GROUP_X_DIMENSION, MEDIUM_TEST_GROUP_Y_DIMENSION, false, 0, true);
    DumpGeometryOneToOne_VRS(MEDIUM_TEST_GROUP_X_DIMENSION);
}

//=================================================================================================================================
// Mesh Shader MSLarge
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION,1)]
void MSLarge(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[LARGE_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[LARGE_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[LARGE_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcMesh(LARGE_TEST_NUM_QUADS_PER_GROUP,groupID.y,0,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION);
    DumpGeometryLarge(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_NUM_VERTS_PER_GROUP,LARGE_TEST_NUM_PRIMS_PER_GROUP);
}                  

//=================================================================================================================================
// Mesh Shader MSSmallFromAS
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(SMALL_TEST_GROUP_X_DIMENSION,SMALL_TEST_GROUP_Y_DIMENSION,1)]
void MSSmallFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in payload smallPayload p, 
    out vertices MSvert verts[SMALL_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[SMALL_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[SMALL_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcMesh(SMALL_TEST_NUM_QUADS_PER_GROUP,groupID.y,p.arraySlice,SMALL_TEST_NUM_GROUPS,threadInGroup,SMALL_TEST_GROUP_X_DIMENSION,SMALL_TEST_GROUP_Y_DIMENSION);
    DumpGeometryOneToOne(SMALL_TEST_GROUP_X_DIMENSION,true);
}                  

//=================================================================================================================================
// Mesh Shader MSMediumFromAS
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION,1)] 
void MSMediumFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in payload largePayload p, 
    out vertices MSvert verts[MEDIUM_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[MEDIUM_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    uint arraySlice = 0;
    CalcArraySliceFromLargePayload();
    CalcMesh(MEDIUM_TEST_NUM_QUADS_PER_GROUP,groupID.y,arraySlice,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION);
    DumpGeometryOneToOne(MEDIUM_TEST_GROUP_X_DIMENSION,true);
}                  

//=================================================================================================================================
// Mesh Shader MSMediumLineFromAS
//=================================================================================================================================
[outputtopology("line")]
[numthreads(MEDIUM_LINE_TEST_GROUP_X_DIMENSION,MEDIUM_LINE_TEST_GROUP_Y_DIMENSION,1)]
void MSMediumLineFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in payload largePayload p, 
    out vertices MSvert verts[MEDIUM_LINE_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_LINE_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint2 idx[MEDIUM_LINE_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    uint arraySlice = 0;
    CalcArraySliceFromLargePayload();
    CalcLineMesh(MEDIUM_LINE_TEST_NUM_PRIMS_PER_GROUP,groupID.y,arraySlice,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,MEDIUM_LINE_TEST_GROUP_X_DIMENSION,MEDIUM_LINE_TEST_GROUP_Y_DIMENSION);
    DumpLineGeometryOneToOne(MEDIUM_LINE_TEST_GROUP_X_DIMENSION);
}

//=================================================================================================================================
// Mesh Shader MSLargeViewInstancingFromAS
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION,1)] 
void MSLargeViewInstancingFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in uint viewID : SV_ViewID,
    in payload largePayload p, 
    out vertices MSvertViewInstancing verts[MEDIUM_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[MEDIUM_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    uint arraySlice = 0;
    CalcArraySliceFromLargePayload();
    CalcMesh(MEDIUM_TEST_NUM_QUADS_PER_GROUP,groupID.y,arraySlice,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,MEDIUM_TEST_GROUP_X_DIMENSION,MEDIUM_TEST_GROUP_Y_DIMENSION,true,viewID);
    DumpGeometryOneToOne(MEDIUM_TEST_GROUP_X_DIMENSION,false);
}                  

//=================================================================================================================================
// Mesh Shader MSSmallViewInstancingFromAS
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(SMALL_TEST_GROUP_X_DIMENSION, SMALL_TEST_GROUP_Y_DIMENSION, 1)]
void MSSmallViewInstancingFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in uint viewID : SV_ViewID,
    in payload smallPayload p,
    out vertices MSvertViewInstancing verts[SMALL_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[SMALL_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[SMALL_TEST_NUM_PRIMS_PER_GROUP]
)
{
    CalcMesh(SMALL_TEST_NUM_QUADS_PER_GROUP, groupID.y, p.arraySlice, SMALL_TEST_NUM_GROUPS, threadInGroup, SMALL_TEST_GROUP_X_DIMENSION, SMALL_TEST_GROUP_Y_DIMENSION, true, viewID);
    DumpGeometryOneToOne(SMALL_TEST_GROUP_X_DIMENSION, false);
}

//=================================================================================================================================
// Mesh Shader MSLargeFromAS
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION,1)] 
void MSLargeFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in payload largePayload p, 
    out vertices MSvert verts[LARGE_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[LARGE_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[LARGE_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    uint arraySlice = 0;
    CalcArraySliceFromLargePayload();
    CalcMesh(LARGE_TEST_NUM_QUADS_PER_GROUP,groupID.y,arraySlice,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION);
    DumpGeometryLarge(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_NUM_VERTS_PER_GROUP,LARGE_TEST_NUM_PRIMS_PER_GROUP);
}                  

//=================================================================================================================================
// Mesh Shader MSCullAllPrimitives
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(MEDIUM_TEST_GROUP_X_DIMENSION, MEDIUM_TEST_GROUP_Y_DIMENSION, 1)]
void MSCullAllPrimitives(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[MEDIUM_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[MEDIUM_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[MEDIUM_TEST_NUM_PRIMS_PER_GROUP]
)
{
    CalcMesh(
        MEDIUM_TEST_NUM_QUADS_PER_GROUP,    // numQuads
        groupID.y,                          // y
        0,                                  // arraySize
        MEDIUM_AND_LARGE_TEST_NUM_GROUPS,   // numDispatchGroups
        threadInGroup,                      // threadInGroup
        MEDIUM_TEST_GROUP_X_DIMENSION,      // threadWidth
        MEDIUM_TEST_GROUP_Y_DIMENSION,      // threadsHeight
        false,                              // bViewInstancing
        0,                                  // viewID
        false,                              // bVariableRateShading
        true);                              // bCullAll
    DumpGeometryOneToOne(MEDIUM_TEST_GROUP_X_DIMENSION, true);
}

//=================================================================================================================================
// Shaders for reference image generation:
//=================================================================================================================================
#define VtxBloat() \
    for(uint i = 0; i < VERT_BLOAT_SIZE; i++) /*generate vertex attribute bloat*/ \
    { \
        vtx.bloat[i] = vtx.pos.x + vtx.pos.y + (float)i; \
    }
#define VtxBloatViewInstancing() \
    for(uint i = 0; i < VERT_BLOAT_SIZE_VIEW_INSTANCING; i++) /*generate vertex attribute bloat*/ \
    { \
        vtx.bloat[i] = vtx.pos.x + vtx.pos.y + (float)i; \
    }
#define VtxBloatVRS() \
    for(uint i = 0; i < VERT_BLOAT_SIZE_VRS; i++) /*generate vertex attribute bloat*/ \
    { \
        vtx.bloat[i] = vtx.pos.x + vtx.pos.y + (float)i; \
    }

//=================================================================================================================================
// Vertex Shader VS
//=================================================================================================================================
[RootSignature(RootSig)]
VSvert VS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSvert vtx;

    uint quadIndex = vertexID / 6;
    uint vertInQuad = vertexID % 6;
    uint uniqueVertInQuadClockwiseFromTopLeft = 0;
    switch(vertInQuad)
    {
    case 0:
        uniqueVertInQuadClockwiseFromTopLeft = 0;
        break;
    case 1:
    case 3:
        uniqueVertInQuadClockwiseFromTopLeft = 1;
        break;
    case 2:
    case 5:
        uniqueVertInQuadClockwiseFromTopLeft = 3;
        break;
    case 4:
        uniqueVertInQuadClockwiseFromTopLeft = 2;
        break;
    }

    vtx.pos = CalcPos(quadIndex,instanceID % RTHeight,uniqueVertInQuadClockwiseFromTopLeft,vtx.tex);

    VtxBloat();

    vtx.rtIndex = instanceID / RTHeight;
    vtx.colorOffset = quadIndex + instanceID;
    return vtx;
}

//=================================================================================================================================
// Vertex Shader VS
//=================================================================================================================================
VSvert_VRS VSVRS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSvert_VRS vtx;

    uint quadIndex = vertexID / 6;
    uint vertInQuad = vertexID % 6;
    uint uniqueVertInQuadClockwiseFromTopLeft = 0;
    switch (vertInQuad)
    {
    case 0:
        uniqueVertInQuadClockwiseFromTopLeft = 0;
        break;
    case 1:
    case 3:
        uniqueVertInQuadClockwiseFromTopLeft = 1;
        break;
    case 2:
    case 5:
        uniqueVertInQuadClockwiseFromTopLeft = 3;
        break;
    case 4:
        uniqueVertInQuadClockwiseFromTopLeft = 2;
        break;
    }

    vtx.pos = CalcPos(quadIndex, instanceID % RTHeight, uniqueVertInQuadClockwiseFromTopLeft, vtx.tex);

    VtxBloatVRS();

    vtx.rtIndex = instanceID / RTHeight;
    vtx.colorOffset = quadIndex + instanceID;
    vtx.shadingRate = 0x5;
    return vtx;
}

//=================================================================================================================================
// Vertex Shader VSLine
//=================================================================================================================================
[RootSignature(RootSig)]
VSvert VSLine(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSvert vtx;

    uint lineIndex = vertexID / 2;
    uint vertInLine = vertexID % 2;

    vtx.pos = CalcPosLine(lineIndex,instanceID % RTHeight,vertInLine,vtx.tex);

    VtxBloat();

    vtx.rtIndex = instanceID / RTHeight;
    vtx.colorOffset = lineIndex + instanceID;
    return vtx;
}

//=================================================================================================================================
// Vertex Shader VSViewInstancing
//=================================================================================================================================
[RootSignature(RootSig)]
VSvertViewInstancing VSViewInstancing(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID, uint viewID : SV_ViewID)
{
    VSvertViewInstancing vtx;

    uint quadIndex = vertexID / 6;
    uint vertInQuad = vertexID % 6;
    uint uniqueVertInQuadClockwiseFromTopLeft = 0;
    switch(vertInQuad)
    {
    case 0:
        uniqueVertInQuadClockwiseFromTopLeft = 0;
        break;
    case 1:
    case 3:
        uniqueVertInQuadClockwiseFromTopLeft = 1;
        break;
    case 2:
    case 5:
        uniqueVertInQuadClockwiseFromTopLeft = 3;
        break;
    case 4:
        uniqueVertInQuadClockwiseFromTopLeft = 2;
        break;
    }

    vtx.pos = CalcPos(quadIndex,instanceID % RTHeight,uniqueVertInQuadClockwiseFromTopLeft,vtx.tex,true,viewID);

    VtxBloatViewInstancing();

    if(viewID == 2)
    {
        vtx.pos.x += 500; // push out of frustum.  Using SV_CullPrimitive for this in mesh shader.
    }
    vtx.rtIndex = instanceID / RTHeight;
    vtx.colorOffset = quadIndex + instanceID;
    return vtx;
}

//=================================================================================================================================
// Pixel Shader PS
//=================================================================================================================================
struct psout
{
    float4 col : SV_Target0;
    float4 tex : SV_Target1;
};

float4 PickColor(uint index)
{
    float4 col = float4(0, 0, 0, 0);
    switch (index)
    {
    case 0:
        col = float4(0, 0, 1, 1);
        break;
    case 1:
        col = float4(0, 1, 0, 1);
        break;
    case 2:
        col = float4(0, 1, 1, 1);
        break;
    case 3:
        col = float4(1, 0, 0, 1);
        break;
    }
    return col;
}

[RootSignature(RootSig)]
psout PS(VSvert psin)
{
    psout ret;
    float useBloat = 0;
    for(uint i = 0; i < VERT_BLOAT_SIZE; i++)
    {
        useBloat += max(1.0,psin.bloat[i]);
    }
    ret.col = PickColor(psin.colorOffset % 4);
    ret.col.a = max(ret.col.a,useBloat); // will get clamped on output
    ret.tex = float4(psin.tex,0,0);
    return ret;
}

psout PSVRS(VSvert_VRS psin)
{
    psout ret;
    float useBloat = 0;
    for (uint i = 0; i < VERT_BLOAT_SIZE_VRS; i++)
    {
        useBloat += max(1.0, psin.bloat[i]);
    }
    ret.col = PickColor(psin.colorOffset % 4);
    ret.col.a = max(ret.col.a, useBloat);
    ret.tex = float4(psin.tex, 0, 0);
    return ret;
}

//=================================================================================================================================
// Pixel Shader PS view instancing
//=================================================================================================================================
[RootSignature(RootSig)]
psout PSViewInstancing(VSvertViewInstancing psin)
{
    psout ret;
    float useBloat = 0;
    for(uint i = 0; i < VERT_BLOAT_SIZE_VIEW_INSTANCING; i++)
    {
        useBloat += max(1.0,psin.bloat[i]);
    }
    ret.col = PickColor(psin.colorOffset % 4);
    ret.col.a = max(ret.col.a,useBloat); // will get clamped on output
    ret.tex = float4(psin.tex,0,0);
    return ret;
}

RWStructuredBuffer<LogLayout> LogUAV : register(u0);

//=================================================================================================================================
// Amplification Shader ASNoPS
//=================================================================================================================================
[numthreads(1,NoPSTestThreads,1)]
void ASNoPS(in uint3 groupID : SV_GroupID)   
{
    InterlockedAdd(LogUAV[0].ASInvocations,1);
    smallPayload p;
    p.arraySlice = groupID.z;
    p.numGroups = NoPSDispatchMeshGroups;
    DispatchMesh(1,1,NoPSDispatchMeshGroups,p);
}                  

//=================================================================================================================================
// Mesh Shader MSNoPS
//=================================================================================================================================
[outputtopology("triangle")]
[numthreads(1,1,NoPSTestThreads)]
void MSNoPS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID, 
    in payload smallPayload p)   
{
    InterlockedAdd(LogUAV[0].MSInvocations,1);
}                  


//=================================================================================================================================
// Mesh Shader MSSysVal
//=================================================================================================================================
struct MSvertSysVal
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float clip : SV_ClipDistance;
    float cull : SV_CullDistance;
};

struct MSprimSysVal
{
    uint vp : SV_ViewportArrayIndex;
};

struct VSvertSysVal
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float clip : SV_ClipDistance;
    float cull : SV_CullDistance;
    uint vp : SV_ViewportArrayIndex;
};

float CalcClipDistance(uint testCase)
{
    switch(testCase)
    {
    case 0:
    default:
        return -1; // clip
    case 1:
    case 2:
    case 3:
        return 1;
    }
}

float CalcCullDistance(uint testCase)
{
    switch(testCase)
    {
    case 1:
    default:
        return -1; // cull
    case 0:
    case 2:
    case 3:
        return 1;
    }
}

float CalcVPIndex(uint testCase)
{
    switch(testCase)
    {
    case 2:
    default:
        return 0; // off RT
    case 0:
    case 1:
    case 3:
        return 1;
    }
}

[outputtopology("triangle")]
[numthreads(PS_SYSTEM_VALUES_TEST_GROUP_X_DIMENSION,PS_SYSTEM_VALUES_TEST_GROUP_Y_DIMENSION,1)]
void MSSysVal(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvertSysVal verts[PS_SYSTEM_VALUES_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprimSysVal prims[PS_SYSTEM_VALUES_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[PS_SYSTEM_VALUES_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcMesh(PS_SYSTEM_VALUES_TEST_NUM_QUADS_PER_GROUP,groupID.y,0,PS_SYSTEM_VALUES_TEST_NUM_GROUPS,threadInGroup,PS_SYSTEM_VALUES_TEST_GROUP_X_DIMENSION,PS_SYSTEM_VALUES_TEST_GROUP_Y_DIMENSION);
    const uint numVertices = PS_SYSTEM_VALUES_TEST_GROUP_X_DIMENSION;
    const uint numPrimitives = PS_SYSTEM_VALUES_TEST_GROUP_X_DIMENSION-2;
    SetMeshOutputCounts(numVertices,numPrimitives);
    if(threadInGroup.y == 0 && threadInGroup.z == 0)
    {
        uint off = threadInGroup.x;
        if(off < numVertices)
        {
            verts[off].pos = gs_verts[off].pos;
            verts[off].clip = CalcClipDistance(groupID.y);
            verts[off].cull = CalcCullDistance(groupID.y);
        }
        if(off < numPrimitives)
        {
            idx[off] = gs_idx[off];
            prims[off].vp = CalcVPIndex(groupID.y);
        }
    }
}                

//=================================================================================================================================
// Vertex Shader VSSysVal
//=================================================================================================================================
[RootSignature(RootSig)]
VSvertSysVal VSSysVal(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VSvertSysVal vtx;

    uint quadIndex = vertexID / 6;
    uint vertInQuad = vertexID % 6;
    uint uniqueVertInQuadClockwiseFromTopLeft = 0;
    switch(vertInQuad)
    {
    case 0:
        uniqueVertInQuadClockwiseFromTopLeft = 0;
        break;
    case 1:
    case 3:
        uniqueVertInQuadClockwiseFromTopLeft = 1;
        break;
    case 2:
    case 5:
        uniqueVertInQuadClockwiseFromTopLeft = 3;
        break;
    case 4:
        uniqueVertInQuadClockwiseFromTopLeft = 2;
        break;
    }

    vtx.pos = CalcPos(quadIndex,instanceID % RTHeight,uniqueVertInQuadClockwiseFromTopLeft,vtx.tex);
    vtx.clip = CalcClipDistance(instanceID);
    vtx.cull = CalcCullDistance(instanceID);
    vtx.vp = CalcVPIndex(instanceID);
    return vtx;
}



//=================================================================================================================================
// Pixel Shader PSSysVal
//=================================================================================================================================
float4 PSSysVal(VSvertSysVal psin) : SV_Target0
{
    return float4(1,1,1,1);
}


//=================================================================================================================================
// Mesh Shader MSPrimitiveID
//=================================================================================================================================
struct MSvertPrimitiveID
{
    float4 pos : SV_POSITION;
};

struct MSprimPrimitiveID
{
    uint id : SV_PrimitiveID;
    uint vp : SV_ViewportArrayIndex;
};

[outputtopology("triangle")]
[numthreads(PRIMITIVEID_TEST_GROUP_X_DIMENSION,PRIMITIVEID_TEST_GROUP_Y_DIMENSION,1)]
void MSPrimitiveID(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvertPrimitiveID verts[PRIMITIVEID_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprimPrimitiveID prims[PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcOverlappingMesh(PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP,threadInGroup,PRIMITIVEID_TEST_GROUP_X_DIMENSION);
    const uint numVertices = PRIMITIVEID_TEST_NUM_VERTS_PER_GROUP;
    const uint numPrimitives = PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP;
    SetMeshOutputCounts(numVertices,numPrimitives);

    if(threadInGroup.y == 0 && threadInGroup.z == 0)
    {
        for(uint currVertRangeStart = 0; currVertRangeStart < numVertices; currVertRangeStart += PRIMITIVEID_TEST_GROUP_X_DIMENSION )
        {
            uint currVert = currVertRangeStart + threadInGroup.x;
            if(currVert >= numVertices)
            {
                break;
            }
            verts[currVert].pos = gs_verts[currVert].pos;
        }
        for(uint currPrimRangeStart = 0; currPrimRangeStart < numPrimitives; currPrimRangeStart += PRIMITIVEID_TEST_GROUP_X_DIMENSION )
        {
            uint currPrim = currPrimRangeStart + threadInGroup.x;
            if(currPrim >= numPrimitives)
            {
                break;
            }
            idx[currPrim] = gs_idx[currPrim];
            uint flattenedGroupID = groupID.y;
            prims[currPrim].id = flattenedGroupID * PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP + currPrim;
            prims[currPrim].vp = 1;
        }
    }
}                

//=================================================================================================================================
// Amplification Shader ASPrimitiveID
//=================================================================================================================================
struct primPayload
{
    uint startID;
};

[numthreads(1,1,1)]
void ASPrimitiveID(in uint3 groupID : SV_GroupID)   
{
    primPayload p;
    p.startID = groupID.x * PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP * PRIMITIVEID_TEST_NUM_GROUPS;
    DispatchMesh(1,PRIMITIVEID_TEST_NUM_GROUPS,1,p);
}                  


//=================================================================================================================================
// Mesh Shader MSPrimitiveIDFromAS
//=================================================================================================================================

[outputtopology("triangle")]
[numthreads(PRIMITIVEID_TEST_GROUP_X_DIMENSION,PRIMITIVEID_TEST_GROUP_Y_DIMENSION,1)]
void MSPrimitiveIDFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in payload primPayload p,
    out vertices MSvertPrimitiveID verts[PRIMITIVEID_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprimPrimitiveID prims[PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    CalcOverlappingMesh(PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP,threadInGroup,PRIMITIVEID_TEST_GROUP_X_DIMENSION);
    const uint numVertices = PRIMITIVEID_TEST_NUM_VERTS_PER_GROUP;
    const uint numPrimitives = PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP;
    SetMeshOutputCounts(numVertices,numPrimitives);

    if(threadInGroup.y == 0 && threadInGroup.z == 0)
    {
        for(uint currVertRangeStart = 0; currVertRangeStart < numVertices; currVertRangeStart += PRIMITIVEID_TEST_GROUP_X_DIMENSION )
        {
            uint currVert = currVertRangeStart + threadInGroup.x;
            if(currVert >= numVertices)
            {
                break;
            }
            verts[currVert].pos = gs_verts[currVert].pos;
        }
        for(uint currPrimRangeStart = 0; currPrimRangeStart < numPrimitives; currPrimRangeStart += PRIMITIVEID_TEST_GROUP_X_DIMENSION )
        {
            uint currPrim = currPrimRangeStart + threadInGroup.x;
            if(currPrim >= numPrimitives)
            {
                break;
            }
            idx[currPrim] = gs_idx[currPrim];
            uint flattenedGroupID = groupID.y;
            prims[currPrim].id = flattenedGroupID * PRIMITIVEID_TEST_NUM_PRIMS_PER_GROUP + currPrim + p.startID;
            prims[currPrim].vp = 1;
        }
    }
}                

//=================================================================================================================================
// Pixel Shader PSPrimitiveID
//=================================================================================================================================
RasterizerOrderedByteAddressBuffer PrimitiveIDLog : register(u0);

float4 PSPrimitiveID(MSvertPrimitiveID vert, MSprimPrimitiveID prim) : SV_Target0
{
    uint loc;
    PrimitiveIDLog.InterlockedAdd(0,1,loc);
    loc+=1;
    PrimitiveIDLog.Store(loc*4,prim.id);
    return float4(1,1,1,1);
}

//=================================================================================================================================
// Amplification Shader ASRayQuery
//=================================================================================================================================
#define RayQueryRootSig      "RootFlags(0)," \
                             "RootConstants(num32BitConstants=3,b0)," /* [Root slot 0] RTWidth, RTHeight, RTArrayDepth */ \
                             "UAV(u0),"                               /* [Root slot 1] Log UAV (only some tests use this) */ \
                             "SRV(t0),"                               /* [Root slot 2] Acceleration structure */

struct rqPayload
{
    uint arraySlice;
    uint numGroups;
};

RaytracingAccelerationStructure AccelStruct : register(t0);
#define FLT_MAX         3.402823466e+38F        // max value

RayDesc MakeRayDesc()
{
    float3 rayDir = float3(0.0, 0.0, 1);
    float3 origin = float3(0.0,0.0,-2);

    RayDesc ray = { origin,
        0.0f,
        rayDir,
        FLT_MAX };
    
    return ray;
}

[RootSignature(RayQueryRootSig)]
[numthreads(8,8,2)]
void ASRayQuery(in uint3 groupID : SV_GroupID)   
{
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(
        AccelStruct,
        RAY_FLAG_NONE,
        0xff,
        MakeRayDesc());

    q.Proceed();
    if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        InterlockedAdd(LogUAV[0].ASInvocations,1);
    }

    rqPayload p;
    p.arraySlice = groupID.x;
    p.numGroups = MEDIUM_AND_LARGE_TEST_NUM_GROUPS;
    DispatchMesh(1,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,1,p);
}                  

//=================================================================================================================================
// Mesh Shader MSRayQueryFromAS
//=================================================================================================================================
[RootSignature(RayQueryRootSig)]
[outputtopology("triangle")]
[numthreads(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION,1)] 
void MSRayQueryFromAS(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    in payload rqPayload p, 
    out vertices MSvert verts[LARGE_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[LARGE_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[LARGE_TEST_NUM_PRIMS_PER_GROUP]
)   
{
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(
        AccelStruct,
        RAY_FLAG_NONE,
        0xff,
        MakeRayDesc());

    q.Proceed();
    if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        InterlockedAdd(LogUAV[0].MSInvocations,1);
    }

    CalcMesh(LARGE_TEST_NUM_QUADS_PER_GROUP,groupID.y,p.arraySlice,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION);
    DumpGeometryLarge(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_NUM_VERTS_PER_GROUP,LARGE_TEST_NUM_PRIMS_PER_GROUP);
}                  



//=================================================================================================================================
// Mesh Shader MSRayQuery
//=================================================================================================================================

[RootSignature(RayQueryRootSig)]
[outputtopology("triangle")]
[numthreads(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION,1)]
void MSRayQuery(
    in uint3 groupID : SV_GroupID,
    in uint3 threadInGroup : SV_GroupThreadID,
    out vertices MSvert verts[LARGE_TEST_NUM_VERTS_PER_GROUP],
    out primitives MSprim prims[LARGE_TEST_NUM_PRIMS_PER_GROUP],
    out indices uint3 idx[LARGE_TEST_NUM_PRIMS_PER_GROUP]
)   
{   
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(
        AccelStruct,
        RAY_FLAG_NONE,
        0xff,
        MakeRayDesc());

    q.Proceed();
    if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        InterlockedAdd(LogUAV[0].MSInvocations,1);
    }

    CalcMesh(LARGE_TEST_NUM_QUADS_PER_GROUP,groupID.y,0,MEDIUM_AND_LARGE_TEST_NUM_GROUPS,threadInGroup,LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_GROUP_Y_DIMENSION);
    DumpGeometryLarge(LARGE_TEST_GROUP_X_DIMENSION,LARGE_TEST_NUM_VERTS_PER_GROUP,LARGE_TEST_NUM_PRIMS_PER_GROUP);
}                  

//=================================================================================================================================
// Pixel Shader PSRayQuery
//=================================================================================================================================
[RootSignature(RayQueryRootSig)]
psout PSRayQuery(VSvert psin)
{
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(
        AccelStruct,
        RAY_FLAG_NONE,
        0xff,
        MakeRayDesc());

    q.Proceed();
    if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        InterlockedAdd(LogUAV[0].PSInvocations,1);
    }

    psout ret;
    ret.col = PickColor(psin.colorOffset % 4);
    ret.tex = float4(psin.tex,0,0);
    return ret;
}
