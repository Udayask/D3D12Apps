#define ENABLE_WINDOW_OUTPUT 0

#pragma once

class ResourceManager;

namespace D3D12Helper
{
#define FRAND_UNORM (float)((rand() % 100) / 100.0f)

    enum D3D12_SHADER_STAGE {
        D3D12_SHADER_STAGE_VERTEX = 0,
        D3D12_SHADER_STAGE_HULL,
        D3D12_SHADER_STAGE_DOMAIN,
        D3D12_SHADER_STAGE_GEOMETRY,
        D3D12_SHADER_STAGE_PIXEL,
        D3D12_SHADER_STAGE_COMPUTE,
        D3D12_SHADER_STAGE_AMPLIFICATION,
        D3D12_SHADER_STAGE_MESH,
    };

    // Shader creation needs to change for the next versions of the API.
    struct ID3D12_SHADER
    {
        D3D12_SHADER_STAGE Type;
        const BYTE* ByteCode;
        size_t ShaderSize;
    };

    HRESULT
    InitD3D12Device (
        ID3D12Device** Device,
        ID3D12CommandQueue** CommandQueue,
        IDXGISwapChain** SwapChain = NULL,
        HWND Window = 0,
        bool bWarpOverride = false,
        D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0,
        IUnknown* adapter = NULL
        );

    void
    CheckEnableDebugLayer (
        );
    
    void
    SetupDebugLayer (
        ID3D12Device* Device
        );

    HWND
    CreateOutputWindow (
        UINT Width,
        UINT Height,
        UINT Left = 0,
        UINT Top = 0
        );

    void
    ResourceBarrier (
        ID3D12GraphicsCommandList *List,
        ID3D12Resource *Resource,
        UINT StateBefore,
        UINT StateAfter,
        UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
        );

    void
    FullResourceBarrier (
        ID3D12GraphicsCommandList *List
        );

    HRESULT
    CreateRenderTarget (
        ID3D12Device *Device,
        ID3D12CommandQueue *CommandQueue,        
        IDXGISwapChain *SwapChain,
        ID3D12Resource **BackBuffer,
        D3D12_CPU_DESCRIPTOR_HANDLE &RenderTarget,
        UINT Width,
        UINT Height,
        D3D12_RESOURCE_BARRIER *RTVBarrier = NULL,
        D3D12_RESOURCE_BARRIER *PresentBarrier = NULL,
        DXGI_FORMAT RenderTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
        D3D12_CLEAR_VALUE *OptimizedClearValue = NULL
        );

    void
    SetRenderTarget (
        ID3D12GraphicsCommandList *List,
        D3D12_CPU_DESCRIPTOR_HANDLE *RenderTarget,
        D3D12_CPU_DESCRIPTOR_HANDLE *DepthStencil = NULL
        );

    void
    SetViewportAndScissor (
        ID3D12GraphicsCommandList *List,
        UINT Width,
        UINT Height,
        UINT Left = 0,
        UINT Top = 0
        );

    void
    Present (
        ID3D12Device *Device,
        ID3D12GraphicsCommandList *List,
        ID3D12CommandQueue *CommandQueue,
        IDXGISwapChain *SwapChain,
        bool WaitForFinish = false
        );

    DWORD
    WaitForCommandListExecution (
        ID3D12Device *Device,        
        ID3D12CommandQueue *CommandQueue
        );

    void
    CreatePipelineState (
        ID3D12Device *Device,
        ID3D12_SHADER Shaders[],
        UINT ShaderCount,
        D3D12_GRAPHICS_PIPELINE_STATE_DESC *PSODescriptor,
        ID3D12PipelineState** PipelineState = NULL
        );

    void
    CreatePipelineState(
        ID3D12Device *Device,
        ID3D12_SHADER Shaders[],
        UINT ShaderCount,
        D3D12_COMPUTE_PIPELINE_STATE_DESC *PSODescriptor,
        ID3D12PipelineState** PipelineState = NULL
        );

    void
    CreateRootSignature (
        ID3D12Device* Device,
        D3D12_ROOT_PARAMETER *Entries,
        UINT EntryCount,
        ID3D12RootSignature **RootSignature,
        ID3DBlob **SerializedLayout
        );

    void
    CreateCommittedResourceSafe (
        ID3D12Device *Device,
        D3D12_HEAP_PROPERTIES *HeapProperties,
        D3D12_HEAP_FLAGS HeapMiscFlags,
        D3D12_RESOURCE_STATES ResourceUsage,
        ID3D12Resource **Resource,
        D3D12_RESOURCE_DESC *ResourceDescriptor,
        D3D12_CLEAR_VALUE *OptimizedCleaValue = NULL
        );

    CComPtr<ID3D12Resource>
    CreateUploadBuffer(
        ID3D12Device *Device,
        const void* pData,
        UINT DataSize
        );

    void
    PushDebugLayerIgnoreSettings (
        ID3D12Device* Device,
        D3D12_MESSAGE_ID* MessageIDs,
        UINT NumIds
        );

    void
    PopDebugLayerIgnoreSettings (
        ID3D12Device* Device
        );

};

// Constructor: Creates a command list
// Caller enqueues arbitrary operations into the command list
// Destructor: Submit command list, and wait for it to finish execution
class SynchrousOperation
{
public:
    SynchrousOperation(ID3D12CommandQueue* pCommandQueue);
    ~SynchrousOperation();

    ID3D12GraphicsCommandList* CommandList();

private:
    CComPtr<ID3D12Device> m_pDevice;
    CComPtr<ID3D12CommandQueue> m_pCommandQueue;
    CComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
    CComPtr<ID3D12GraphicsCommandList> m_pCommandList;
};

class ScopedResourceBarrier
{
public:
    ScopedResourceBarrier(ID3D12GraphicsCommandList *pCommandList, ID3D12Resource *pResource, D3D12_RESOURCE_STATES originalState, D3D12_RESOURCE_STATES tempState) :
        m_pCommandList(pCommandList), m_pResource(pResource), m_originalState(originalState), m_tempState(tempState)
    {
        D3D12Helper::ResourceBarrier(pCommandList, pResource, originalState, tempState);
    }

    ~ScopedResourceBarrier()
    {
        D3D12Helper::ResourceBarrier(m_pCommandList, m_pResource, m_tempState, m_originalState);
    }

private:
    ID3D12GraphicsCommandList *m_pCommandList;
    ID3D12Resource *m_pResource;
    D3D12_RESOURCE_STATES m_originalState, m_tempState;
};