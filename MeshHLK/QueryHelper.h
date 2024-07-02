#pragma once 

#include "directx/d3d12.h"
#include "directx/d3dx12.h"

#include <wrl.h>
using Microsoft::WRL::ComPtr;

class QueryHelper
{
    ID3D12Device *m_Device;
    ComPtr<ID3D12QueryHeap> m_QueryHeap;
    ComPtr<ID3D12Resource> m_QueryReadBackBuffer;
    UINT m_QueryCount;
    D3D12_QUERY_TYPE m_QueryType;
    UINT m_ElementSize;

public:
    void
    Initialize (
        ID3D12Device *Device,
        UINT QueryCount,
        D3D12_QUERY_TYPE QueryType = D3D12_QUERY_TYPE_OCCLUSION
        );

    void
    BeginQuery (
        ID3D12GraphicsCommandList *CommandList,
        UINT QueryIndex = 0
        );

    void
    EndQuery (
        ID3D12GraphicsCommandList *CommandList,
        UINT QueryIndex = 0
        );

    void
    ResolveQuery (
        ID3D12GraphicsCommandList *CommandList
        );

    void
    ResolveQuery (
        ID3D12GraphicsCommandList *CommandList,
        UINT StartIndex,
        UINT EndIndex,
        UINT Alignment
        );

    void
    GetData (
        PVOID Data,
        UINT DataLength
        );
};