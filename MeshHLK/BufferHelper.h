using namespace WEX::Logging;
using namespace WEX::TestExecution;

#pragma once

/* BufferHelper:
 * This class creates D3D12 Buffers using the desired method (Committed, Placed, Reserved)
 * using memory from a desired Heap type (Default, Upload, CustomCPU).
 * 
 * The usage pattern is:
 * Init()
 * CreateBuffer(&pResource)
 *  - creates resource in GENERIC_READ
 *  - does not support READBACK, but CUSTOM uses GetCustomHeapProperties for READBACK-type
 * CopyToResource(pResource)
 * pResource->Release() // ensure you release your references
 * 
 * CreateBuffer uses the Suballocate function to allocate memory,
 * which in turn use the AllocateNew function to create new memory of the desired type when necessary.
*/

class BufferHelper {

    ID3D12Device* m_Device;
    ID3D12CommandQueue* m_CmdQueue; // used for updating tile mappings for reserved resources during BufferCreate() and performing a synchronous UpdateSubresources() during CopyToResource()
    UINT64 m_MaxSize; // size of heaps created for the placement scenario
    bool m_InitCalled;
    std::vector<SafeHANDLE> sharedHandles; // strictly for cleanup, all created shared handles end up in here

public:
    BufferHelper ()
        : m_InitCalled(false)
        , m_Device(nullptr)
        , m_CmdQueue(nullptr)
        , m_MaxSize(0)
        , m_UploadBegin(NULL)
        , m_UploadCurrent(NULL)
        , m_UploadEnd(NULL)
        , m_DefaultBegin(NULL)
        , m_DefaultCurrent(NULL)
        , m_DefaultEnd(NULL)
        , m_DefaultBeginShared(NULL)
        , m_DefaultCurrentShared(NULL)
        , m_DefaultEndShared(NULL)
        , m_CustomCPUBegin(NULL)
        , m_CustomCPUCurrent(NULL)
        , m_CustomCPUEnd(NULL)
     {}

    void
    Init(
        ID3D12Device *Device,
        ID3D12CommandQueue * CmdQueue,
        UINT64 MaxSize = (4 * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
        )
    {
        m_InitCalled = true;
        m_Device = Device;
        m_CmdQueue = CmdQueue;
        m_MaxSize = MaxSize;
    }

    void
    CreateBuffer(
        D3D12_HEAP_TYPE HeapType,
        CreationMethod eCreationMethod,
        UINT64 Size,
        D3D12_RESOURCE_FLAGS MiscFlags,
        __out ID3D12Resource **ppResource,
        bool bUseShared = false,
        D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON
        )
    {
        VERIFY_IS_TRUE(m_InitCalled, L"Users of this API must call the Init function first!");
        VERIFY_IS_NOT_NULL(ppResource, L"ppResource is required.");

        // Setup variables based on heap type
        PBYTE* pBegin   = nullptr;
        PBYTE* pCurrent = nullptr;
        PBYTE* pEnd     = nullptr;
        std::vector<CComPtr<ID3D12Heap>>* pHeaps = nullptr;
        D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(HeapType);
        switch (HeapType)
        {
        case D3D12_HEAP_TYPE_UPLOAD:
            VERIFY_IS_FALSE(bUseShared, L"It is invalid to create a shared CPU-Accessible Heap.");
            HeapProps = m_Device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
            pBegin   = &m_UploadBegin;
            pCurrent = &m_UploadCurrent;
            pEnd     = &m_UploadEnd;
            pHeaps   = &m_UploadHeaps;
            break;
        case D3D12_HEAP_TYPE_DEFAULT:
            if (bUseShared)
            {
                pBegin   = &m_DefaultBeginShared;
                pCurrent = &m_DefaultCurrentShared;
                pEnd     = &m_DefaultEndShared;
                pHeaps   = &m_DefaultHeapsShared;
            }
            else
            {
                pBegin   = &m_DefaultBegin;
                pCurrent = &m_DefaultCurrent;
                pEnd     = &m_DefaultEnd;
                pHeaps   = &m_DefaultHeaps;
            }
            break;
        case D3D12_HEAP_TYPE_CUSTOM: // emulates READBACK
            VERIFY_IS_FALSE(bUseShared, L"It is invalid to create a shared CPU-Accessible Heap.");
            HeapProps = m_Device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_READBACK);
            pBegin    = &m_CustomCPUBegin;
            pCurrent  = &m_CustomCPUCurrent;
            pEnd      = &m_CustomCPUEnd;
            pHeaps    = &m_CustomCPUHeaps;
            break;
        default:
            VERIFY_FAIL(L"HeapType is currently unsupported.");
            return;
        }

        CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Size, MiscFlags);
        if (eCreationMethod == CreationMethod::CreationMethodPlaced)
        {
            // Get required memory
            ID3D12Heap* pHeap = nullptr;
            UINT64 HeapOffset = 0;
            Suballocate(Size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, *pBegin, *pCurrent, *pEnd, HeapProps, *pHeaps, bUseShared, &pHeap, &HeapOffset);

            // Create resource
            VERIFY_SUCCEEDED(
                m_Device->CreatePlacedResource(
                    pHeap,
                    HeapOffset,
                    &BufferDesc,
                    InitialState,
                    nullptr,
                    IID_PPV_ARGS(ppResource)
                    ));
        }
        else if (eCreationMethod == CreationMethod::CreationMethodComitted)
        {
            const D3D12_HEAP_FLAGS MiscHeapFlags = bUseShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE;
            CComPtr<ID3D12Resource> pResource = nullptr;
            VERIFY_SUCCEEDED(m_Device->CreateCommittedResource(
                &HeapProps,
                MiscHeapFlags,
                &BufferDesc,
                InitialState,
                nullptr,
                IID_PPV_ARGS(&pResource)
                ));

            if (bUseShared)
            {
                sharedHandles.push_back(SafeHANDLE());
                VERIFY_SUCCEEDED(m_Device->CreateSharedHandle(
                    pResource,
                    nullptr,
                    GENERIC_ALL,
                    nullptr,
                    &sharedHandles.back().m_h));
                VERIFY_SUCCEEDED(m_Device->OpenSharedHandle(sharedHandles.back(), IID_PPV_ARGS(ppResource)));
            }
            else
            {
                *ppResource = pResource.Detach();
            }
        }
        else if (eCreationMethod == CreationMethod::CreationMethodReserved)
        {
            // Get required memory
            ID3D12Heap* pHeap = nullptr;
            UINT64 HeapOffset = 0;
            Suballocate(Size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, *pBegin, *pCurrent, *pEnd, HeapProps, *pHeaps, bUseShared, &pHeap, &HeapOffset);

            // Create resource
            VERIFY_SUCCEEDED(
                m_Device->CreateReservedResource(
                    &BufferDesc,
                    InitialState,
                    nullptr,
                    IID_PPV_ARGS(ppResource)
                    ));

            // Associate resource and memory
            D3D12_TILED_RESOURCE_COORDINATE StartCoord = { 0, 0, 0, 0 };
            D3D12_TILE_REGION_SIZE RegionSize = { 0, false, 0, 0, 0 };
            m_Device->GetResourceTiling(*ppResource, &RegionSize.NumTiles, nullptr, nullptr, nullptr, 0, nullptr);
            VERIFY_ARE_EQUAL(0, HeapOffset % 0x10000, L"Assumes HeapOffset is aligned to 64K tiles");
            UINT TileOffsetIntoHeap = static_cast<UINT>(HeapOffset / 0x10000);
            D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
            m_CmdQueue->UpdateTileMappings(*ppResource, 1, &StartCoord, &RegionSize, pHeap, 1, &RangeFlag, &TileOffsetIntoHeap, &RegionSize.NumTiles, D3D12_TILE_MAPPING_FLAG_NONE);
        }
        else
        {
            VERIFY_FAIL(L"CreationMethod not recognized");
        }

        ResourceInfo[*ppResource] = {HeapType, eCreationMethod};
    }

    /*******************************************/
    /* Copy CPU-data to Resource functionality */
    /*******************************************/

    void CopyToResource(ID3D12Resource* pResource, const VOID* pData, SIZE_T Size, D3D12_RESOURCE_STATES CurrentState)
    {
        VERIFY_IS_TRUE(m_InitCalled, L"Users of this API must call the Init function first!");

        auto ResInfoIterator = ResourceInfo.find(pResource);
        VERIFY_ARE_NOT_EQUAL(ResourceInfo.end(), ResInfoIterator, L"pResource was not added to the ResourceInfo map.");
        ResourceData ResData = ResInfoIterator->second;

        if (ResData.HeapType == D3D12_HEAP_TYPE_DEFAULT || ResData.CreationMethod == CreationMethodReserved)
        {
            // Create temporary command list
            CComPtr<ID3D12CommandAllocator> pDCLCommandAllocator;
            CComPtr<ID3D12GraphicsCommandList> pCommandList;
            VERIFY_SUCCEEDED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pDCLCommandAllocator)));
            VERIFY_SUCCEEDED(
                m_Device->CreateCommandList(
                    0,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    pDCLCommandAllocator,
                    NULL,
                    IID_PPV_ARGS(&pCommandList)
                    ));

            // Create temporary staging resource
            CComPtr<ID3D12Resource> pStagingResource;
            CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_UPLOAD);
            UINT64 IntermediateSize = GetRequiredIntermediateSize(pResource, 0, 1);
            VERIFY_ARE_EQUAL(Size, IntermediateSize, L"Provided Size of pData needs to account for the whole buffer (i.e. equal to GetRequiredIntermediateSize() output)");
            CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IntermediateSize);
            VERIFY_SUCCEEDED(
                m_Device->CreateCommittedResource(
                    &HeapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &BufferDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&pStagingResource)
                    ));

            bool NeedTransition = (CurrentState & D3D12_RESOURCE_STATE_COPY_DEST) == 0;
            D3D12_RESOURCE_BARRIER BarrierDesc; ZeroMemory(&BarrierDesc, sizeof(BarrierDesc));

            if (NeedTransition)
            {
                // Transition to COPY_DEST
                BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                BarrierDesc.Transition.pResource = pResource;
                BarrierDesc.Transition.Subresource = 0;
                BarrierDesc.Transition.StateBefore = CurrentState;
                BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                pCommandList->ResourceBarrier(1, &BarrierDesc);
                std::swap(BarrierDesc.Transition.StateBefore, BarrierDesc.Transition.StateAfter); // ensure StateBefore represents current state
            }

            // Execute upload
            D3D12_SUBRESOURCE_DATA SubResourceData = { pData, static_cast<LONG_PTR>(Size), static_cast<LONG_PTR>(Size) };
            VERIFY_ARE_EQUAL(Size, UpdateSubresources(pCommandList, pResource, pStagingResource.p, 0, 0, 1, &SubResourceData), L"UpdateSubresources returns the number of bytes updated, so 0 if nothing was updated");

            if (NeedTransition)
            {
                // Transition back to whatever the app had
                pCommandList->ResourceBarrier(1, &BarrierDesc);
                std::swap(BarrierDesc.Transition.StateBefore, BarrierDesc.Transition.StateAfter); // ensure StateBefore represents current state
            }

            // Finish Upload
            VERIFY_SUCCEEDED(pCommandList->Close());
            m_CmdQueue->ExecuteCommandLists(1, CommandListCast(&pCommandList.p));
            D3D12Helper::WaitForCommandListExecution(m_Device, m_CmdQueue);
        }
        else if (ResData.HeapType == D3D12_HEAP_TYPE_UPLOAD || ResData.HeapType == D3D12_HEAP_TYPE_CUSTOM)
        {
            PVOID pResourceData = nullptr;
            VERIFY_SUCCEEDED(pResource->Map(0, nullptr, &pResourceData));
            memcpy(pResourceData, pData, Size);
            pResource->Unmap(0, nullptr);
        }
        else
        {
            VERIFY_FAIL(L"Unrecognized heap type.");
        }
    }

private:

    /************************************/
    /* Generic Allocation functionality */
    /************************************/

    void
    Suballocate(
        UINT64 Size,
        UINT Alignment,
        PBYTE& Begin,
        PBYTE& Current,
        PBYTE& End,
        D3D12_HEAP_PROPERTIES HeapProps,
        std::vector<CComPtr<ID3D12Heap>>& Heaps,
        bool bUseShared,
        __out ID3D12Heap **Heap,
        __out UINT64 *HeapOffset
        )
    {
        AlignValue(Current, Alignment);
        if (Current + Size > End) {
            Heaps.push_back(NULL);
            AllocateNewHeap(
                Size,
                HeapProps,
                Alignment,
                D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS | (bUseShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE),
                bUseShared,
                &Heaps.back(),
                Begin,
                Current,
                End
                );
        }

        *HeapOffset = UINT64(Current - Begin);
        Current += Size;

        *Heap = Heaps.back();
    }

    void
    AllocateNewHeap(
        UINT64 Size,
        D3D12_HEAP_PROPERTIES HeapProps,
        UINT Alignment,
        D3D12_HEAP_FLAGS MiscFlags,
        bool bUseShared,
        __out ID3D12Heap** ppHeap,
        __out PBYTE& Begin,
        __out PBYTE& Current,
        __out PBYTE& End
        )
    {
        UINT64 ActualSize = max(Size, m_MaxSize); // use Max Size, unless the requested resource is larger than that
        // Heap attribution is needed for D3D12_RESOURCE_HEAP_TIER_1, use a buffer heap
        CD3DX12_HEAP_DESC HeapDesc(ActualSize, HeapProps, Alignment, MiscFlags);
        if (bUseShared)
        {
            CComPtr<ID3D12Heap> pHeap = nullptr;
            VERIFY_ARE_NOT_EQUAL(0, MiscFlags & D3D12_HEAP_FLAG_SHARED, L"Shared Heaps require this flag");
            VERIFY_SUCCEEDED(m_Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(&pHeap)));
            sharedHandles.push_back(SafeHANDLE());
            VERIFY_SUCCEEDED(m_Device->CreateSharedHandle(
                pHeap,
                nullptr,
                GENERIC_ALL,
                nullptr,
                &sharedHandles.back().m_h));
            VERIFY_SUCCEEDED(m_Device->OpenSharedHandle(sharedHandles.back(), IID_PPV_ARGS(ppHeap)));
        }
        else
        {
            VERIFY_SUCCEEDED(m_Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(ppHeap)));
        }

        Begin = 0;
        Current = 0;
        End = Begin + ActualSize;
    }

    /**************************/
    /* Resource Data Tracking */
    /**************************/
    struct ResourceData
    {
        D3D12_HEAP_TYPE HeapType;
        CreationMethod CreationMethod;
    };
    std::map<ID3D12Resource*, ResourceData> ResourceInfo;

    /***************/
    /* Upload Heap */
    /***************/

    std::vector<CComPtr<ID3D12Heap>> m_UploadHeaps;
    PBYTE m_UploadBegin;
    PBYTE m_UploadCurrent;
    PBYTE m_UploadEnd;

    /****************/
    /* Default Heap */
    /****************/

    std::vector<CComPtr<ID3D12Heap>> m_DefaultHeaps;
    PBYTE m_DefaultBegin;
    PBYTE m_DefaultCurrent;
    PBYTE m_DefaultEnd;
    std::vector<CComPtr<ID3D12Heap>> m_DefaultHeapsShared;
    PBYTE m_DefaultBeginShared;
    PBYTE m_DefaultCurrentShared;
    PBYTE m_DefaultEndShared;

    /***************/
    /* Custom Heap */
    /***************/

    D3D12_HEAP_PROPERTIES m_CustomCPUHeapProps;
    std::vector<CComPtr<ID3D12Heap>> m_CustomCPUHeaps;
    PBYTE m_CustomCPUBegin;
    PBYTE m_CustomCPUCurrent;
    PBYTE m_CustomCPUEnd;
};
