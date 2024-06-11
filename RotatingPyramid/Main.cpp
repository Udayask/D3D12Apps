#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <array>
#include <optional>
#include <functional>
#include <algorithm>
#include <fstream>
#include <map>
#include <chrono>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#define APPLICATION_NAME        "Rotating Pyramid"
#define WINDOW_WIDTH            1920
#define WINDOW_HEIGHT           1080

class DeletionQueue {
public:
    using Fn    = std::function<void()>;
    using Queue = std::deque<Fn>;

    template<typename Fn>
    void Append(Fn&& fn) {
        q.emplace_back(fn);
    }

    void Finalize() {
        for (auto it = q.rbegin(); it != q.rend(); ++it) {
            (*it)();
        }

        q.clear();
    }

private:
    Queue q;
};

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct UniformBuffer
{
    DirectX::XMFLOAT4X4 mvp;
    char padding[192];
};

static_assert(sizeof(UniformBuffer) % 256 == 0);

#pragma region ClassDecl

class alignas(64) Harmony
{
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    static constexpr uint32_t DEFAULT_CHUNK_SIZE   = 128 * 1024 * 1024;
    static constexpr uint32_t UPLOAD_CHUNK_SIZE    = 64 * 1024 * 1024;

    bool Init(HINSTANCE inst);
    void Run();
    void Shutdown();
    void Resize();

private:
    inline uint32_t GetSizeInMB(uint64_t sizeInBytes) {
        return (sizeInBytes >> 20) & 0xffffffff;
    }

    void OpenWindow(HINSTANCE instance);
    void CreateAdapter();
    void CreateDevice();
    void CreateCommandQueue();
    void CreateSwapChain();
    void CreateHeaps();
    void CreateResourcesAndViews();
    void CreatePipelines();
    void CreateCommandLists();
    void CreateSyncObjects();

    void PopulateCommandList();
    void MoveToNextFrame();
    void WaitForGpu();
    void Render();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
        LPARAM lParam);

    DeletionQueue              delQ;

    IDXGIFactory7*             pFactory7         = nullptr;
    IDXGIAdapter1*             pAdapter1         = nullptr;
    IDXGISwapChain4*           pSwapChain4       = nullptr;

    ID3D12Device9*             pDevice9          = nullptr;
    ID3D12CommandQueue*        pCommandQueue     = nullptr;
    ID3D12DescriptorHeap*      pRtvHeap          = nullptr;
    ID3D12Resource*            pRenderTargets[MAX_FRAMES_IN_FLIGHT] = { nullptr };
    ID3D12CommandAllocator*    pCommandAllocators[MAX_FRAMES_IN_FLIGHT] = { nullptr };
    ID3D12GraphicsCommandList* pCommandList      = nullptr;
    ID3D12Fence*               pFence            = nullptr;
    ID3D12Heap*                pHeaps[2]         = { nullptr }; // 0 is default, 1 is upload

    ID3D12RootSignature*       pRootSignature    = nullptr;
    ID3D12PipelineState*       pPipelineState    = nullptr;

    HINSTANCE                  hInstance         = NULL;
    HWND                       hMainWindow       = NULL;

    UINT                       frameIndex        = 0;
    HANDLE                     fenceHandle       = NULL;
    UINT64                     fenceValues[MAX_FRAMES_IN_FLIGHT] = { 0 };
    UINT64                     maxChunkSizes[2]     = { 0 };

    UINT                       rtvDescriptorSize = 0;
    D3D_FEATURE_LEVEL          featureLevel      = D3D_FEATURE_LEVEL_12_0;
    D3D12_VIEWPORT             viewport;
    D3D12_RECT                 scissorRect;

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOpts;

#ifdef _DEBUG
    static inline const bool enableDebugLayers = true;
#else
    static inline const bool enableDebugLayers = false;
#endif
};

#pragma endregion

#pragma region Public Interface

bool Harmony::Init(HINSTANCE inst) {
    hInstance = inst;

    try {
        OpenWindow(hInstance);

        CreateAdapter();

        CreateDevice();

        CreateCommandQueue();

        CreateSwapChain();

        CreateHeaps();

        CreateResourcesAndViews();

        CreatePipelines();

        CreateCommandLists();

        CreateSyncObjects();

        //BeginOnetime


        //end onetime

        //etc

    }
    catch (std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return false;
    }

    return true;
}

void Harmony::Run() {
    for (;;) {
        MSG msg;

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        Render();
    }
}

void Harmony::Shutdown() {
    WaitForGpu();

    delQ.Finalize();
}

void Harmony::Resize() {
    //TODO:
}

#pragma endregion

#pragma region Init Calls

void Harmony::OpenWindow(HINSTANCE instance) {
    WNDCLASSEX wcex{
       sizeof(WNDCLASSEX),
       CS_HREDRAW | CS_VREDRAW,
       WndProc,
       0,
       0,
       instance,
       LoadIcon(instance, IDI_APPLICATION),
       LoadCursor(instance, IDC_ARROW),
       (HBRUSH)GetStockObject(BLACK_BRUSH),
       NULL,
       APPLICATION_NAME,
       LoadIcon(instance, IDI_APPLICATION)
    };

    if (!RegisterClassEx(&wcex)) {
        throw std::runtime_error("Could not register class!");
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowX = screenWidth / 2 - WINDOW_WIDTH / 2;
    int windowY = screenHeight / 2 - WINDOW_HEIGHT / 2;

    hMainWindow = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW, APPLICATION_NAME, APPLICATION_NAME,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        windowX, windowY, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        instance,
        NULL);

    if (!hMainWindow) {
        throw std::runtime_error("Could not create main window!");
    }

    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);
    SetForegroundWindow(hMainWindow);
    SetFocus(hMainWindow);

    SetWindowLongPtr(hMainWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    delQ.Append(
        [ cHwnd = hMainWindow] {
            DestroyWindow(cHwnd);
        }
    );
}

void Harmony::CreateAdapter() {
    UINT dxgiFactoryFlags = 0;

    if constexpr (enableDebugLayers) {
        ComPtr<ID3D12Debug> debugController;

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }

    ComPtr<IDXGIFactory> factory;
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
        throw std::runtime_error("Could not create DXGI factory!");
    }

    ComPtr<IDXGIAdapter1> adapter1;
    ComPtr<IDXGIFactory7> factory7;
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory7)))) {
        UINT adapterIndex = 0;

        for (; SUCCEEDED(factory7->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter1))); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter1->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG::DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), featureLevel, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    if (!adapter1.Get()) {
        throw std::runtime_error("Could not find GPU HW adapter!");
    }

    pFactory7 = factory7.Detach();
    pAdapter1 = adapter1.Detach();

    delQ.Append([cFactory = pFactory7, cAdapter = pAdapter1] {
        cAdapter->Release();
        cFactory->Release();
    });

    DXGI_ADAPTER_DESC1 adDesc = {};
    pAdapter1->GetDesc1(&adDesc);

    std::wcout << "Running on: " << adDesc.Description << std::endl;
    std::wcout << "     Video  Mem: " << GetSizeInMB(adDesc.DedicatedVideoMemory)  << "MB" << std::endl;
    std::wcout << "     System Mem: " << GetSizeInMB(adDesc.DedicatedSystemMemory) << "MB" << std::endl;
    std::wcout << "     Shared Mem: " << GetSizeInMB(adDesc.SharedSystemMemory)    << "MB" << std::endl;

    maxChunkSizes[0] = (adDesc.DedicatedVideoMemory ? adDesc.DedicatedVideoMemory : adDesc.DedicatedSystemMemory) / 2;
    maxChunkSizes[1] = (adDesc.SharedSystemMemory) / 2;
}

void Harmony::CreateDevice() {
    ComPtr<ID3D12Device9> device;

    if (FAILED(D3D12CreateDevice(pAdapter1, featureLevel, IID_PPV_ARGS(&device)))) {
        throw std::runtime_error("Could not create ID3D12Device9 device!");
    }

    pDevice9 = device.Detach();

    delQ.Append([cDevice = pDevice9] {
        cDevice->Release();
    });

    // Query feature 
    pDevice9->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS, &featureDataOpts, sizeof featureDataOpts);
    if (featureDataOpts.ResourceHeapTier != D3D12_RESOURCE_HEAP_TIER_2) {
        throw std::runtime_error("Only support resource heap Tier2 device!");
    }
}

void Harmony::CreateCommandQueue() {
    ComPtr<ID3D12CommandQueue> cmdQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    if (FAILED(pDevice9->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue)))) {
        throw std::runtime_error("Could not create command queue");
    }

    pCommandQueue = cmdQueue.Detach();

    delQ.Append([cCommandQueue = pCommandQueue] {
        cCommandQueue->Release();
    });
}

void Harmony::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

    swapChainDesc.BufferCount = MAX_FRAMES_IN_FLIGHT;
    swapChainDesc.Width       = WINDOW_WIDTH;
    swapChainDesc.Height      = WINDOW_HEIGHT;
    swapChainDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    swapChainDesc.SampleDesc.Count   = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    ComPtr<IDXGISwapChain1> swapChain;
    
    if (FAILED(pFactory7->CreateSwapChainForHwnd(pCommandQueue, hMainWindow, &swapChainDesc, nullptr, nullptr, &swapChain))) {
        throw std::runtime_error("Could not create swap chain!");
    }

    if (FAILED(pFactory7->MakeWindowAssociation(hMainWindow, DXGI_MWA_NO_ALT_ENTER))) {
        throw std::runtime_error("Could not associate swap chain with hWnd!");
    }

    swapChain->QueryInterface(&pSwapChain4);
    if (!pSwapChain4) {
        throw std::runtime_error("Could not get SwapChain4 interface!");
    }

    frameIndex = pSwapChain4->GetCurrentBackBufferIndex();

    delQ.Append([cSwapChain = pSwapChain4 ] {
        cSwapChain->Release();
    });

    viewport    = D3D12_VIEWPORT{ .TopLeftX = 0.0f, .TopLeftY = 0.0f, .Width = WINDOW_WIDTH, .Height = WINDOW_HEIGHT, .MinDepth = D3D12_MIN_DEPTH, .MaxDepth = D3D12_MAX_DEPTH };
    scissorRect = D3D12_RECT{ .left = 0, .top = 0, .right = WINDOW_WIDTH, .bottom = WINDOW_HEIGHT };
}

void Harmony::CreateHeaps() {
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = MAX_FRAMES_IN_FLIGHT,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    if (FAILED(pDevice9->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&descriptorHeap)))) {
        throw std::runtime_error("Could not create RTV heap!");
    }

    rtvDescriptorSize = pDevice9->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    pRtvHeap = descriptorHeap.Detach();

    delQ.Append([cHeap = pRtvHeap] {
        cHeap->Release();
    });

    auto AllocateHeap = [cDevice = pDevice9](UINT64 chunkSize, D3D12_HEAP_TYPE type) -> ID3D12Heap* {
        D3D12_HEAP_PROPERTIES hProps = cDevice->GetCustomHeapProperties(0, type);

        D3D12_HEAP_DESC heapDesc {
            .SizeInBytes = chunkSize,
            .Properties = {.Type = hProps.Type, .CPUPageProperty = hProps.CPUPageProperty, .MemoryPoolPreference = hProps.MemoryPoolPreference, .CreationNodeMask = hProps.CreationNodeMask , .VisibleNodeMask = hProps.VisibleNodeMask },
            .Alignment  = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Flags      = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES
        };

        ComPtr<ID3D12Heap> memHeap;
        if (FAILED(cDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&memHeap)))) {
            throw std::runtime_error("Could not create default memory heap!");
        }

        return memHeap.Detach();
    };

    pHeaps[0] = AllocateHeap(GetSizeInMB(maxChunkSizes[0]), D3D12_HEAP_TYPE_DEFAULT);
    pHeaps[1] = AllocateHeap(GetSizeInMB(maxChunkSizes[1]), D3D12_HEAP_TYPE_UPLOAD);

    delQ.Append([cHeaps = pHeaps] {
        cHeaps[1]->Release();
        cHeaps[0]->Release();
    });
}

void Harmony::CreateResourcesAndViews() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (FAILED(pSwapChain4->GetBuffer(i, IID_PPV_ARGS(&pRenderTargets[i])))) {
            throw std::runtime_error("Could not Get swap chain buffer!");
        }

        pDevice9->CreateRenderTargetView(pRenderTargets[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }

    // TODO: create constant buffers

    // TODO: Create depth buffer 

    // TODO: Create texture       (upload data)  

    // TODO: Create vertex buffer (upload date)

    // TODO: Create index buffer  (upload data)
}

void Harmony::CreatePipelines() {
    // root signature (empty except for flags)
    {
        D3D12_ROOT_SIGNATURE_DESC rDesc{
            .NumParameters = 0,
            .pParameters = nullptr,
            .NumStaticSamplers = 0,
            .pStaticSamplers = nullptr,
            .Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        };

        ComPtr<ID3DBlob> signature;
        if (FAILED(D3D12SerializeRootSignature(&rDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, nullptr))) {
            throw std::runtime_error("Could not serialize root desc!");
        }

        ComPtr<ID3D12RootSignature> rs;
        if (FAILED(pDevice9->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rs)))) {
            throw std::runtime_error("Could not create root signature!");
        }

        pRootSignature = rs.Detach();
        delQ.Append([cRootSignature = pRootSignature] {
            cRootSignature->Release();
            });
    }

    // pipeline
    {
        ComPtr<ID3DBlob>            vs, ps;
        ComPtr<ID3DBlob>            errBlob;
        ComPtr<ID3D12PipelineState> pso;

        UINT compileFlags = 0;

        if constexpr (enableDebugLayers) {
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        }

        if (FAILED(D3DCompileFromFile(L"shaders/shaders.hlsl", nullptr, nullptr, "VsMain", "vs_5_1", compileFlags, 0, &vs, &errBlob))) {
            throw std::runtime_error(reinterpret_cast<const char *>(errBlob->GetBufferPointer()));
        }

        if (FAILED(D3DCompileFromFile(L"shaders/shaders.hlsl", nullptr, nullptr, "PsMain", "ps_5_1", compileFlags, 0, &ps, &errBlob))) {
            throw std::runtime_error(reinterpret_cast<const char *>(errBlob->GetBufferPointer()));
        }

        D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_BLEND_DESC blendDesc;
        blendDesc.AlphaToCoverageEnable  = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;

        for (UINT i = 0; i < 8; ++i) {
            blendDesc.RenderTarget[i] = {
                FALSE,FALSE,
                D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
                D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
                D3D12_LOGIC_OP_NOOP,
                D3D12_COLOR_WRITE_ENABLE_ALL,
            };
        }

        D3D12_RASTERIZER_DESC rastDesc;
        rastDesc.FillMode              = D3D12_FILL_MODE_SOLID;
        rastDesc.CullMode              = D3D12_CULL_MODE_BACK;
        rastDesc.FrontCounterClockwise = FALSE;
        rastDesc.DepthBias             = 0;
        rastDesc.DepthBiasClamp        = 0.0f;
        rastDesc.SlopeScaledDepthBias  = 0.0f;
        rastDesc.DepthClipEnable       = FALSE;
        rastDesc.MultisampleEnable     = FALSE;
        rastDesc.AntialiasedLineEnable = FALSE;
        rastDesc.ForcedSampleCount     = 0;

        D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
            D3D12_STENCIL_OP_KEEP,
            D3D12_STENCIL_OP_KEEP,
            D3D12_STENCIL_OP_KEEP,
            D3D12_COMPARISON_FUNC_ALWAYS
        };

        D3D12_DEPTH_STENCIL_DESC dsDesc;
        dsDesc.DepthEnable      = TRUE;
        dsDesc.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        dsDesc.StencilEnable    = FALSE;
        dsDesc.StencilReadMask  = 0;
        dsDesc.StencilWriteMask = 0;
        dsDesc.FrontFace        = defaultStencilOp;
        dsDesc.BackFace         = defaultStencilOp;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{ 0 };
        psoDesc.pRootSignature        = pRootSignature;
        psoDesc.VS                    = D3D12_SHADER_BYTECODE{ .pShaderBytecode = vs->GetBufferPointer(), .BytecodeLength = vs->GetBufferSize() };
        psoDesc.PS                    = D3D12_SHADER_BYTECODE{ .pShaderBytecode = ps->GetBufferPointer(), .BytecodeLength = ps->GetBufferSize() };
        psoDesc.BlendState            = blendDesc;
        psoDesc.SampleMask            = D3D12_DEFAULT_SAMPLE_MASK;
        psoDesc.RasterizerState       = rastDesc;
        psoDesc.DepthStencilState     = dsDesc;
        psoDesc.InputLayout           = { .pInputElementDescs = inputElementDesc, .NumElements = 2 };
        psoDesc.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets      = 1;
        psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc            = { .Count = 1, .Quality = 0 };
        psoDesc.NodeMask              = 0;
        psoDesc.CachedPSO             = { .pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0 };
        psoDesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

        HRESULT hr = pDevice9->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

        if(FAILED(hr)) {
            throw std::runtime_error("Could not create pipeline state object!");
        }

        pPipelineState = pso.Detach();

        delQ.Append([cPipelineState = pPipelineState] {
            cPipelineState->Release();
            });
    }
}

void Harmony::CreateCommandLists() {
    for (UINT i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (FAILED(pDevice9->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocators[i])))) {
            throw std::runtime_error("Could not allocate command allocator!");
        }
    }

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    if (FAILED(pDevice9->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocators[0], nullptr, IID_PPV_ARGS(&cmdList)))) {
        throw std::runtime_error("Could not create command list!");
    }

    pCommandList = cmdList.Detach();
    pCommandList->Close();

    delQ.Append([cCommandList = pCommandList, cCommandAllocators = pCommandAllocators] {
        cCommandList->Release();

        for (UINT i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            cCommandAllocators[i]->Release();
        }
    });
}

void Harmony::CreateSyncObjects() {
    ComPtr<ID3D12Fence> fence;

    if (FAILED(pDevice9->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        throw std::runtime_error("Could not create fence!");
    }

    fenceHandle = CreateEventA(nullptr, FALSE, FALSE, "gpuFence");
    if (!fenceHandle) {
        throw std::runtime_error("Could not create fence event!");
    }

    pFence = fence.Detach();

    delQ.Append([cEvent = fenceHandle, cFence = pFence] {
        cFence->Release();
        CloseHandle(cEvent);
    });
}

#pragma endregion

#pragma region Misc

LRESULT Harmony::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
    case WM_DESTROY:
    {
        PostQuitMessage(0);         // close the application entirely
        return 0;
    };

    case WM_SIZE:
    {
        Harmony* pApp = reinterpret_cast<Harmony*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (pApp) {
            pApp->Resize();
            return 0;
        }
    };
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#pragma endregion

#pragma region Rendering

void Harmony::Render() {
    PopulateCommandList();

    ID3D12CommandList* ppCmdLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(1, ppCmdLists);

    pSwapChain4->Present(1, 0);

    MoveToNextFrame();
}

void Harmony::PopulateCommandList() {
    pCommandAllocators[frameIndex]->Reset();
    pCommandList->Reset(pCommandAllocators[frameIndex], nullptr);

    D3D12_RESOURCE_BARRIER rtBarrier {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource   = pRenderTargets[frameIndex],
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET,
        }
    };

    pCommandList->ResourceBarrier(1, &rtBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + frameIndex * rtvDescriptorSize);
    pCommandList->OMSetRenderTargets(1, &rtHandle, FALSE, nullptr);

    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

    const float clearColor[] = { 0.5f, 0.2f, 0.0f, 1.0f };
    pCommandList->ClearRenderTargetView(rtHandle, clearColor, 0, nullptr);

    


    std::swap(rtBarrier.Transition.StateBefore, rtBarrier.Transition.StateAfter);

    pCommandList->ResourceBarrier(1, &rtBarrier);
    pCommandList->Close();
}

void Harmony::MoveToNextFrame() {
    auto curFenceVal = fenceValues[frameIndex];

    // signal in cmd queue
    pCommandQueue->Signal(pFence, curFenceVal);

    // move frame index
    frameIndex = pSwapChain4->GetCurrentBackBufferIndex();

    if (pFence->GetCompletedValue() < fenceValues[frameIndex]) {
        pFence->SetEventOnCompletion(fenceValues[frameIndex], fenceHandle);

        WaitForSingleObjectEx(fenceHandle, INFINITE, FALSE);
    }

    fenceValues[frameIndex] = curFenceVal + 1;
}

void Harmony::WaitForGpu() {
    pCommandQueue->Signal(pFence, fenceValues[frameIndex]);

    pFence->SetEventOnCompletion(fenceValues[frameIndex], fenceHandle);
    WaitForSingleObjectEx(fenceHandle, INFINITE, FALSE);

    fenceValues[frameIndex] += 1;
}

#pragma endregion

static void MakeConsole() {
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    SetConsoleTitle(TEXT(APPLICATION_NAME));

    FILE* fDummy = nullptr;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
}

int main(int argc, char* argv[]) {
    HINSTANCE instance = NULL;

    MakeConsole();

    Harmony app;

    if (!app.Init(instance)) {
        std::cerr << "App::Init failed!" << std::endl;
        return -1;
    }

    app.Run();
    app.Shutdown();

	return 0;
}

