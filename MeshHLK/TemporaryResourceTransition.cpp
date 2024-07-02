#include "TemporaryResourceTransition.h"

inline void
ResourceBarrier (
    ID3D12GraphicsCommandList *List,
    ID3D12Resource *Resource,
    UINT StateBefore,
    UINT StateAfter,
    UINT Subresource
)
{
    D3D12_RESOURCE_BARRIER Barrier;

    ZeroMemory(&Barrier, sizeof(Barrier));
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource = Resource;
    Barrier.Transition.Subresource = Subresource;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATES( StateBefore );
    Barrier.Transition.StateAfter = D3D12_RESOURCE_STATES( StateAfter );
    List->ResourceBarrier(1, &Barrier);
}

TemporaryResourceTransition::TemporaryResourceTransition(
    ID3D12GraphicsCommandList *List,
    ID3D12Resource *Resource,
    D3D12_RESOURCE_STATES DestinationUsage,
    D3D12_RESOURCE_STATES OriginalUsage,
    UINT Subresource
    ) :
    m_List(List),
    m_Resource(Resource),
    m_OriginalUsage(OriginalUsage),
    m_DestUsage(DestinationUsage),
    m_Subresource(Subresource)
{
    if (m_OriginalUsage != m_DestUsage)
    {
        ResourceBarrier(m_List, m_Resource, m_OriginalUsage, m_DestUsage, m_Subresource);
    }
}

TemporaryResourceTransition::~TemporaryResourceTransition()
{
    if (m_List && (m_OriginalUsage != m_DestUsage))
    {
        ResourceBarrier(m_List, m_Resource, m_DestUsage, m_OriginalUsage, m_Subresource);
    }
}