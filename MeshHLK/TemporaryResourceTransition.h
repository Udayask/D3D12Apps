#pragma once

#include "directx/d3d12.h"

class TemporaryResourceTransition
{
public:
    TemporaryResourceTransition() {}

    TemporaryResourceTransition(
        ID3D12GraphicsCommandList *List,
        ID3D12Resource *Resource,
        D3D12_RESOURCE_STATES DestinationUsage,
        D3D12_RESOURCE_STATES OriginalUsage,
        UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
        );

    ~TemporaryResourceTransition();

private:
    ID3D12GraphicsCommandList *m_List     = nullptr;
    ID3D12Resource* m_Resource            = nullptr;
    D3D12_RESOURCE_STATES m_OriginalUsage = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_DestUsage     = D3D12_RESOURCE_STATE_COMMON;
    UINT m_Subresource                    = 0;
};