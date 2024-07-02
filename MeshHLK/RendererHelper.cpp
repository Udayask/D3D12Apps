#include "D3D12TestCommonHeader.h"

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace std;

static const D3D12_COMMAND_QUEUE_DESC s_CommandQueueDesc = {};

HRESULT D3D12CreateDeviceHelper(
    _In_opt_ IDXGIAdapter* pAdapter,
    D3D_FEATURE_LEVEL MinimumFeatureLevel,
    _In_opt_ CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    _In_ REFIID riidSwapchain,
    _COM_Outptr_opt_ void** ppSwapChain,
    _In_ REFIID riidQueue,
    _COM_Outptr_opt_ void** ppQueue,    
    _In_ REFIID riidDevice,
    _COM_Outptr_opt_ void** ppDevice 
    )
{
    CComPtr<ID3D12Device> pDevice;
    CComPtr<ID3D12CommandQueue> pQueue;
    CComPtr<IDXGIFactory> pDxgiFactory;
    CComPtr<IDXGISwapChain> pDxgiSwapChain;

    CComPtr<IUnknown> pDeviceOut;
    CComPtr<IUnknown> pQueueOut;
    CComPtr<IUnknown> pDxgiSwapChainOut;

    HRESULT hr = D3D12CreateDevice(
        pAdapter,
        MinimumFeatureLevel,
        IID_PPV_ARGS(&pDevice)
        );
    if(FAILED(hr)) { return hr; }
    LeakTrackDevice(pDevice);

    hr = CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory));
    if(FAILED(hr)) { return hr; }
    LeakTrackFactory(pDxgiFactory);

    hr = pDevice->CreateCommandQueue(&s_CommandQueueDesc, IID_PPV_ARGS(&pQueue));
    if(FAILED(hr)) { return hr; }
    
    DXGI_SWAP_CHAIN_DESC LocalSCD = *pSwapChainDesc;
    hr = pDxgiFactory->CreateSwapChain(
        pQueue,
        &LocalSCD,
        &pDxgiSwapChain
        );
    if(FAILED(hr)) { return hr; }

    hr = pDevice->QueryInterface(riidDevice, reinterpret_cast<void**>(&pDeviceOut));
    if(FAILED(hr)) { return hr; }

    hr = pQueue->QueryInterface(riidQueue, reinterpret_cast<void**>(&pQueueOut));
    if(FAILED(hr)) { return hr; }

    hr = pDxgiSwapChain->QueryInterface(riidSwapchain, reinterpret_cast<void**>(&pDxgiSwapChainOut));
    if(FAILED(hr)) { return hr; }

    *ppDevice = pDeviceOut.Detach();
    *ppQueue = pQueueOut.Detach();
    *ppSwapChain = pDxgiSwapChain.Detach();

    return S_OK;
}

namespace D3D12Helper
{
    HRESULT
    InitD3D12Device (
        ID3D12Device **Device,
        ID3D12CommandQueue **CommandQueue,
        IDXGISwapChain **SwapChain,
        HWND Window,
        bool bWarpOverride,
        D3D_FEATURE_LEVEL fl,
        IUnknown* adapter
        )
    {
        CComPtr<IDXGIAdapter> spWarpAdapter;
        CheckEnableDebugLayer();

        // Switch to WARP if requested by user.
        bool WarpRT;
        HRESULT ResultRTParams = WEX::TestExecution::RuntimeParameters::TryGetValue(L"Warp", WarpRT);
        bool WarpTestData;
        HRESULT ResultTestData = WEX::TestExecution::TestData::TryGetValue(L"Warp", WarpTestData);

        if (bWarpOverride || (ResultRTParams == S_OK && WarpRT != false) || (ResultTestData == S_OK && WarpTestData != false)) {
            CComPtr<IDXGIFactory4> spFactory;
            VERIFY_SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&spFactory)));
            LeakTrackFactory(spFactory);
            VERIFY_SUCCEEDED(spFactory->EnumWarpAdapter(IID_PPV_ARGS(&spWarpAdapter)));
        }

        if (Window == 0) {
            VERIFY_SUCCEEDED(D3D12CreateDevice(
                adapter ? adapter : spWarpAdapter,
                fl,
                IID_PPV_ARGS(Device)
                ));
            LeakTrackDevice(*Device);

            if (CommandQueue != nullptr)
            {
                VERIFY_SUCCEEDED((*Device)->CreateCommandQueue(&s_CommandQueueDesc, IID_PPV_ARGS(CommandQueue)));
            }
        } else if (CommandQueue != nullptr && SwapChain != nullptr) {
#if ENABLE_WINDOW_OUTPUT
            RECT ClientRect;

            GetClientRect(Window, &ClientRect);

            UINT Width = ClientRect.right;
            UINT Height = ClientRect.bottom;

            DXGI_SWAP_CHAIN_DESC ChainDescriptor;
            ZeroMemory(&ChainDescriptor, sizeof(ChainDescriptor));

            ChainDescriptor.BufferDesc.Width = Width;
            ChainDescriptor.BufferDesc.Height = Height;
            ChainDescriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            ChainDescriptor.SampleDesc.Count = 1;
            ChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            ChainDescriptor.BufferCount = 1;
            ChainDescriptor.OutputWindow = Window;
            ChainDescriptor.Windowed = TRUE;

            VERIFY_SUCCEEDED(D3D12CreateDeviceHelper(
                spWarpAdapter,
                D3D_FEATURE_LEVEL_11_0,
                &ChainDescriptor,
                IID_PPV_ARGS(SwapChain),
                IID_PPV_ARGS(CommandQueue),
                IID_PPV_ARGS(Device)
                ));
#endif
        }

        if (*Device) {
            SetupDebugLayer(*Device);
        }

        return S_OK;
    }

    void CheckEnableDebugLayer()
    {
        bool DebugLayer = false;
        WEX::TestExecution::RuntimeParameters::TryGetValue(L"DebugLayer", DebugLayer);
        if (DebugLayer)
        {
            CComPtr<ID3D12Debug> spDebug;
            VERIFY_SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&spDebug)));
            spDebug->EnableDebugLayer();
        }
    }    

    void
    SetupDebugLayer(
        ID3D12Device* Device
        )
    {
        bool DebugLayer = false;
        WEX::TestExecution::RuntimeParameters::TryGetValue(L"DebugLayer", DebugLayer);
        // Force a debug break when running with layers on
        if (Device && DebugLayer) {
            CComPtr<ID3D12InfoQueue> pIQ;
            if (SUCCEEDED(Device->QueryInterface(&pIQ)))
            {
                pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, true);
                pIQ->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE, true);

                pIQ->ClearStoredMessages();

                D3D12_MESSAGE_ID Ignore[] = {
                    D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_GETHEAPPROPERTIES_INVALIDRESOURCE,
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_GETPRIVATEDATA_MOREDATA,
                    };
                PushDebugLayerIgnoreSettings(Device, Ignore, _countof(Ignore));

            }
        }
    }

    HWND
    CreateOutputWindow (
        UINT Width,
        UINT Height,
        UINT Left,
        UINT Top
        )
    {
#if ENABLE_WINDOW_OUTPUT
        HWND Window = CreateWindowW(
            L"STATIC",
            L"DX12",
            WS_VISIBLE | WS_OVERLAPPEDWINDOW,
            Left,
            Top,
            Width,
            Height,
            NULL,
            NULL,
            NULL,
            NULL
            );

        return Window;
#else
        UNREFERENCED_PARAMETER(Width);
        UNREFERENCED_PARAMETER(Height);
        UNREFERENCED_PARAMETER(Left);
        UNREFERENCED_PARAMETER(Top);
        return NULL;
#endif
    }

    void
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

    void
    FullResourceBarrier (
        ID3D12GraphicsCommandList *List
        )
    {
        D3D12_RESOURCE_BARRIER Barriers[2];
        Barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        Barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barriers[0].UAV.pResource = nullptr;
        Barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        Barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barriers[1].Aliasing.pResourceBefore = nullptr;
        Barriers[1].Aliasing.pResourceAfter = nullptr;
        List->ResourceBarrier(2, Barriers);
    }

    HRESULT
    CreateRenderTarget (
        ID3D12Device *Device,
        ID3D12CommandQueue*,
        IDXGISwapChain *SwapChain,
        ID3D12Resource **BackBuffer,
        D3D12_CPU_DESCRIPTOR_HANDLE &RenderTarget,
        UINT Width,
        UINT Height,
        D3D12_RESOURCE_BARRIER *RTVBarrier,
        D3D12_RESOURCE_BARRIER *PresentBarrier,
        DXGI_FORMAT RenderTargetFormat,
        D3D12_CLEAR_VALUE *OptimizedClearValue
        )
    {
        // TODO: Move this function into the ResourceManager.
        // It is responsible for creating an RT in the Heap.

        if (SwapChain == NULL) {

            D3D12_RESOURCE_DESC Descriptor;

            RtlZeroMemory(&Descriptor, sizeof(Descriptor));
            Descriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            Descriptor.MipLevels = 1;
            Descriptor.Width = Width;
            Descriptor.Height = Height;
            Descriptor.DepthOrArraySize = 1;
            Descriptor.Format = RenderTargetFormat;
            Descriptor.SampleDesc.Count = 1;
            Descriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

            CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
            VERIFY_SUCCEEDED(Device->CreateCommittedResource(
                &HeapProps,
                D3D12_HEAP_FLAG_NONE,
                &Descriptor,
                D3D12_RESOURCE_STATE_COMMON,
                OptimizedClearValue,
                IID_PPV_ARGS(BackBuffer)
                ));

            D3D12_RENDER_TARGET_VIEW_DESC RtvDescriptor;
            RtlZeroMemory(&RtvDescriptor, sizeof(RtvDescriptor));
            RtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            RtvDescriptor.Format = RenderTargetFormat;
            RtvDescriptor.Texture2D.MipSlice = 0;
            Device->CreateRenderTargetView(*BackBuffer, &RtvDescriptor, RenderTarget);
        } else {
            VERIFY_SUCCEEDED(SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer)));
            Device->CreateRenderTargetView(*BackBuffer, NULL, RenderTarget);
            //RenderTarget->m_Format = (*BackBuffer)->GetDesc().Format;
        }

        if (RTVBarrier != NULL) {
            ZeroMemory(RTVBarrier, sizeof(*RTVBarrier));
            RTVBarrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            RTVBarrier->Transition.pResource = *BackBuffer;
            RTVBarrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            RTVBarrier->Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            RTVBarrier->Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        if (PresentBarrier != NULL) {
            ZeroMemory(PresentBarrier, sizeof(*PresentBarrier));
            PresentBarrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            PresentBarrier->Transition.pResource = *BackBuffer;
            PresentBarrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            PresentBarrier->Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            PresentBarrier->Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        }

        return S_OK;
    }

    void
    SetRenderTarget (
        ID3D12GraphicsCommandList *List,
        D3D12_CPU_DESCRIPTOR_HANDLE *RenderTarget,
        D3D12_CPU_DESCRIPTOR_HANDLE *DepthStencil
        )
    {
        List->OMSetRenderTargets(
            1,
            RenderTarget,
            true,
            DepthStencil
            );
    }

    void
    SetViewportAndScissor (
        ID3D12GraphicsCommandList *List,
        UINT Width,
        UINT Height,
        UINT Left,
        UINT Top
        )
    {
        // Setup scissor rect.
        D3D12_RECT ScissorRect = {
            static_cast<LONG>(Left),
            static_cast<LONG>(Top),
            static_cast<LONG>(Left + Width),
            static_cast<LONG>(Top + Height),
        };

        D3D12_VIEWPORT Viewport = {
            static_cast<float>(Left),
            static_cast<float>(Top),
            static_cast<float>(Width),
            static_cast<float>(Height),
            0.0f,
            1.0f
        };

        List->RSSetViewports(1, &Viewport);
        List->RSSetScissorRects(1, &ScissorRect);
    }

    void
    Present (
        ID3D12Device *Device,
        ID3D12GraphicsCommandList *List,
        ID3D12CommandQueue *CommandQueue,
        IDXGISwapChain *SwapChain,
        bool WaitForFinish
        )
    {
        UNREFERENCED_PARAMETER(List);

        if (SwapChain != NULL) {
            SwapChain->Present(0, 0);
        }

        if (WaitForFinish) {
            WaitForCommandListExecution(Device, CommandQueue);
        }
    }

    DWORD
    WaitForCommandListExecution (
        ID3D12Device *Device,
        ID3D12CommandQueue *CommandQueue
        )
    {
        SafeHANDLE EventHandle;
        EventHandle.m_h = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
        CComPtr<ID3D12Fence> Fence;
        VERIFY_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
        VERIFY_SUCCEEDED(CommandQueue->Signal(Fence, 1));
        VERIFY_SUCCEEDED(Fence->SetEventOnCompletion(1, EventHandle));
        return WaitForSingleObject(EventHandle, INFINITE);
    }

    void
    CreatePipelineState (
        ID3D12Device *Device,
        ID3D12_SHADER Shaders[],
        UINT ShaderCount,
        D3D12_GRAPHICS_PIPELINE_STATE_DESC *PSODescriptor,
        ID3D12PipelineState** PipelineState
        )
    {
        for (UINT Index = 0; Index < ShaderCount; Index += 1) {
            D3D12_SHADER_BYTECODE *Shader = NULL;
            switch (Shaders[Index].Type) {
            case D3D12_SHADER_STAGE_VERTEX:
                Shader = &PSODescriptor->VS;
                break;
            case D3D12_SHADER_STAGE_HULL:
                Shader = &PSODescriptor->HS;
                break;
            case D3D12_SHADER_STAGE_DOMAIN:
                Shader = &PSODescriptor->DS;
                break;
            case D3D12_SHADER_STAGE_GEOMETRY:
                Shader = &PSODescriptor->GS;
                break;
            case D3D12_SHADER_STAGE_PIXEL:
                Shader = &PSODescriptor->PS;
                break;
            case D3D12_SHADER_STAGE_COMPUTE:
                assert(false);
                break;
            default:
                break;
            }

            if (Shader != NULL) {
                Shader->BytecodeLength = Shaders[Index].ShaderSize;
                Shader->pShaderBytecode = Shaders[Index].ByteCode;
            }
        }

        if (Device != NULL && PipelineState != NULL) {
            HRESULT Result = Device->CreateGraphicsPipelineState(PSODescriptor, IID_PPV_ARGS(PipelineState));
            VERIFY_SUCCEEDED(Result);
        }
    }

    void
    CreatePipelineState (
        ID3D12Device *Device,
        ID3D12_SHADER Shaders[],
        UINT ShaderCount,
        D3D12_COMPUTE_PIPELINE_STATE_DESC *PSODescriptor,
        ID3D12PipelineState** PipelineState
        )
    {
        for (UINT Index = 0; Index < ShaderCount; Index += 1) {
            D3D12_SHADER_BYTECODE *Shader = NULL;
            switch (Shaders[Index].Type) {
            case D3D12_SHADER_STAGE_COMPUTE:
                Shader = &PSODescriptor->CS;
                break;
            case D3D12_SHADER_STAGE_VERTEX:
            case D3D12_SHADER_STAGE_HULL:
            case D3D12_SHADER_STAGE_DOMAIN:
            case D3D12_SHADER_STAGE_GEOMETRY:
            case D3D12_SHADER_STAGE_PIXEL:
                assert(false);
                break;
            default:
                break;
            }

            if (Shader != NULL) {
                Shader->BytecodeLength = Shaders[Index].ShaderSize;
                Shader->pShaderBytecode = Shaders[Index].ByteCode;
            }
        }

        if (Device != NULL && PipelineState != NULL) {
            HRESULT Result = Device->CreateComputePipelineState(PSODescriptor, IID_PPV_ARGS(PipelineState));
            VERIFY_SUCCEEDED(Result);
        }
    }

    void
    CreateRootSignature (
        ID3D12Device* Device,
        D3D12_ROOT_PARAMETER *Entries,
        UINT EntryCount,
        ID3D12RootSignature **RootSignature,
        ID3DBlob **SerializedLayout
        )
    {
        CComPtr<ID3DBlob> ErrorBlob;
        CD3DX12_ROOT_SIGNATURE_DESC RTLayout(EntryCount,
                                      Entries,
                                      0,
                                      NULL,
                                      (D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                       D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT
                                       )
                                      );

        HRESULT hr = D3D12SerializeRootSignature(&RTLayout, D3D_ROOT_SIGNATURE_VERSION_1, SerializedLayout, &ErrorBlob);

        if (hr != S_OK){

            String ErrorMessage((char*)ErrorBlob->GetBufferPointer());
            Log::Error(ErrorMessage);
            VERIFY_IS_TRUE(false);
        }

        VERIFY_SUCCEEDED(Device->CreateRootSignature(
            GetFullNodeMask(Device),
            (*SerializedLayout)->GetBufferPointer(),
            (*SerializedLayout)->GetBufferSize(),
            __uuidof(ID3D12RootSignature),
            (void**)RootSignature));
    }

    void
    CreateCommittedResourceSafe (
        ID3D12Device *Device,
        D3D12_HEAP_PROPERTIES *HeapProperties,
        D3D12_HEAP_FLAGS HeapMiscFlags,
        D3D12_RESOURCE_STATES ResourceUsage,
        ID3D12Resource **Resource,
        D3D12_RESOURCE_DESC *ResourceDescriptor,
        D3D12_CLEAR_VALUE* OptimizedClearValue
        )
    {
        HRESULT Result;

        Result = Device->CreateCommittedResource(
            HeapProperties,
            HeapMiscFlags,
            ResourceDescriptor,
            ResourceUsage,
            OptimizedClearValue,
            IID_PPV_ARGS(Resource));

        // Fallback to creation with "resource usage initial".
        if (Result != S_OK) {
            VERIFY_SUCCEEDED(Device->CreateCommittedResource(
                HeapProperties,
                HeapMiscFlags,
                ResourceDescriptor,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(Resource)));


            CComPtr<ID3D12CommandAllocator> CommandAllocator;
            VERIFY_SUCCEEDED(Device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&CommandAllocator)));

            CComPtr<ID3D12GraphicsCommandList> CommandList;
            VERIFY_SUCCEEDED(Device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                CommandAllocator,
                NULL,
                IID_PPV_ARGS(&CommandList)));

            ResourceBarrier(CommandList, *Resource, D3D12_RESOURCE_STATE_COMMON, ResourceUsage);

            VERIFY_SUCCEEDED(CommandList->Close());
            CComPtr<ID3D12CommandQueue> CommandQueue;
            VERIFY_SUCCEEDED(Device->CreateCommandQueue(&s_CommandQueueDesc, IID_PPV_ARGS(&CommandQueue.p)));
            CommandQueue->ExecuteCommandLists(1, CommandListCast(&CommandList.p));

            SafeHANDLE Event;
            Event.m_h = CreateEvent(NULL, FALSE, FALSE, NULL);
            VERIFY_IS_NOT_NULL(Event.m_h);
            CComPtr<ID3D12Fence> Fence;
            VERIFY_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
            VERIFY_SUCCEEDED(CommandQueue->Signal(Fence, 1));
            VERIFY_SUCCEEDED(Fence->SetEventOnCompletion(1, Event));

            Log::Warning(L"The driver does not support the new HEAP DDIs and the runtime used fallbacks for resource creation.");
        }
    }

    CComPtr<ID3D12Resource>
    CreateUploadBuffer(
        ID3D12Device *Device,
        const void* pData,
        UINT DataSize
        )
    {
        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(DataSize);

        CComPtr<ID3D12Resource> spResource;
        CreateCommittedResourceSafe(
            Device,
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            &spResource,
            &resourceDesc
            );

        void* pMapped = nullptr;
        VERIFY_SUCCEEDED(spResource->Map(0, nullptr, &pMapped));

        memcpy(pMapped, pData, DataSize);

        spResource->Unmap(0, nullptr);

        return spResource;
    }

    void
    PushDebugLayerIgnoreSettings (
        ID3D12Device* Device,
        D3D12_MESSAGE_ID* MessageIDs,
        UINT NumIds
        )
    {
        bool DebugLayer = false;
        WEX::TestExecution::RuntimeParameters::TryGetValue(L"DebugLayer", DebugLayer);

        if (Device && DebugLayer) {
            CComPtr<ID3D12InfoQueue> pIQ = nullptr;
            if (SUCCEEDED(Device->QueryInterface(&pIQ)))
            {
                D3D12_INFO_QUEUE_FILTER Filter;
                ZeroMemory(&Filter, sizeof(Filter));

                D3D12_MESSAGE_SEVERITY Severity = D3D12_MESSAGE_SEVERITY_INFO;
                Filter.DenyList.NumSeverities = 1;
                Filter.DenyList.pSeverityList = &Severity;
                Filter.DenyList.NumIDs = NumIds;
                Filter.DenyList.pIDList = MessageIDs;

                pIQ->PushStorageFilter(&Filter);
            }
        }
    }

    void
    PopDebugLayerIgnoreSettings (
        ID3D12Device* Device
        )
    {
        bool DebugLayer = false;
        WEX::TestExecution::RuntimeParameters::TryGetValue(L"DebugLayer", DebugLayer);

        if (Device && DebugLayer) {
            CComPtr<ID3D12InfoQueue> pIQ = nullptr;
            if (SUCCEEDED(Device->QueryInterface(&pIQ)))
            {
                pIQ->PopStorageFilter();
            }
        }
    }
};

SynchrousOperation::SynchrousOperation(
    ID3D12CommandQueue* pCommandQueue
    ) : m_pCommandQueue(pCommandQueue)
{
    VERIFY_SUCCEEDED(m_pCommandQueue->GetDevice(IID_PPV_ARGS(&m_pDevice)));

    VERIFY_SUCCEEDED(m_pDevice->CreateCommandAllocator(
        pCommandQueue->GetDesc().Type,
        IID_PPV_ARGS( &m_pCommandAllocator )
        ));

    VERIFY_SUCCEEDED(m_pDevice->CreateCommandList(
        pCommandQueue->GetDesc().NodeMask,
        pCommandQueue->GetDesc().Type,
        m_pCommandAllocator,
        nullptr,
        IID_PPV_ARGS(&m_pCommandList)
        ));
}

SynchrousOperation::~SynchrousOperation()
{
    // Execute the command list, wait for it to finish
    VERIFY_SUCCEEDED(m_pCommandList->Close());

    m_pCommandQueue->ExecuteCommandLists(1, CommandListCast(&m_pCommandList.p));
    D3D12Helper::WaitForCommandListExecution(m_pDevice, m_pCommandQueue);
}

ID3D12GraphicsCommandList* SynchrousOperation::CommandList()
{
    return m_pCommandList;
}
