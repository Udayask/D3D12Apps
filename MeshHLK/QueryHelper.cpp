#include "QueryHelper.h"

#include <iostream>
#include <cassert>
using namespace std;

#define VERIFY_SUCCEEDED(x)   \
    if(FAILED((x))) {   throw std::runtime_error("Failed!"); }    \

void
QueryHelper::Initialize (
    ID3D12Device *Device,
    UINT QueryCount,
    D3D12_QUERY_TYPE QueryType
    )
{
    m_Device = Device;
    m_QueryCount = QueryCount;
    m_QueryType = QueryType;
    D3D12_QUERY_HEAP_TYPE QueryHeapType = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
    switch (QueryType) {
        case D3D12_QUERY_TYPE_OCCLUSION:
            QueryHeapType = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
            m_ElementSize = sizeof(UINT64);
        break;
        case D3D12_QUERY_TYPE_BINARY_OCCLUSION:
            QueryHeapType = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
            m_ElementSize = sizeof(UINT64);
        break;
        case D3D12_QUERY_TYPE_TIMESTAMP:
            QueryHeapType = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            m_ElementSize = sizeof(UINT64);
        break;
        case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
            QueryHeapType = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
            m_ElementSize = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
        break;
        case D3D12_QUERY_TYPE_PIPELINE_STATISTICS1:
            QueryHeapType = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1;
            m_ElementSize = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS1);
        break;
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3:
            QueryHeapType = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS;
            m_ElementSize = sizeof(D3D12_QUERY_DATA_SO_STATISTICS);
        break;
        default:
            assert(false);
        break;
    }

    D3D12_QUERY_HEAP_DESC QueryHeapDesc = {QueryHeapType, QueryCount};
    m_QueryHeap = nullptr;
    VERIFY_SUCCEEDED(m_Device->CreateQueryHeap(
        &QueryHeapDesc,
        IID_PPV_ARGS(&m_QueryHeap)
        ) );

    CD3DX12_RESOURCE_DESC BufferDescriptor = CD3DX12_RESOURCE_DESC::Buffer(
            QueryCount * m_ElementSize
            );

    CD3DX12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    m_QueryReadBackBuffer = nullptr;
    VERIFY_SUCCEEDED(m_Device->CreateCommittedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &BufferDescriptor,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_QueryReadBackBuffer)
        ));
}

void
QueryHelper::BeginQuery (
    ID3D12GraphicsCommandList *CommandList,
    UINT QueryIndex
    )
{
    CommandList->BeginQuery(m_QueryHeap.Get(), m_QueryType, QueryIndex);
}

void
QueryHelper::EndQuery (
    ID3D12GraphicsCommandList *CommandList,
    UINT QueryIndex
    )
{
    CommandList->EndQuery(m_QueryHeap.Get(), m_QueryType, QueryIndex);
}

void
QueryHelper::ResolveQuery (
    ID3D12GraphicsCommandList *CommandList,
    UINT Begin,
    UINT End,
    UINT Alignment
    )
{
    CommandList->ResolveQueryData(m_QueryHeap.Get(), m_QueryType, Begin, End, m_QueryReadBackBuffer.Get(), Alignment);
}

void
QueryHelper::ResolveQuery (
    ID3D12GraphicsCommandList *CommandList
    )
{
    CommandList->ResolveQueryData(m_QueryHeap.Get(), m_QueryType, 0, m_QueryCount, m_QueryReadBackBuffer.Get(), 0);
}

void
QueryHelper::GetData (
    PVOID Data,
    UINT DataLength
    )
{
    void *Mapped = nullptr;
    CD3DX12_RANGE ReadRange(0, min(m_QueryCount * m_ElementSize, DataLength));
    VERIFY_SUCCEEDED(m_QueryReadBackBuffer->Map(0, &ReadRange, &Mapped));
    memcpy(Data, Mapped, ReadRange.End);
    m_QueryReadBackBuffer->Unmap(0, nullptr);
}
