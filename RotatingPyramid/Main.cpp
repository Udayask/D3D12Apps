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
using namespace DirectX;

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#define APPLICATION_NAME        "Rotating Pyramid"
#define WINDOW_WIDTH            1920
#define WINDOW_HEIGHT           1080

#define TEXTURE_WIDTH           1024
#define TEXTURE_HEIGHT          1024

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
    DirectX::XMFLOAT3 color;
    DirectX::XMFLOAT2 uv;
};

static Vertex vertices[12] = {
    {{ 0.5f,  0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
    {{ 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

    {{ 0.5f,  0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}},
    {{ 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f,  0.0f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

    {{-0.5f,  0.0f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
    {{ 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},             
    {{-0.5f,  0.0f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    {{-0.5f,  0.0f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.5f, 0.5f}},
    {{ 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},             
    {{ 0.5f,  0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
};

static uint32_t indices[12] = {
    0,  1,  2,
    3,  4,  5,
    6,  7,  8,
    9, 10, 11
};

struct UniformBuffer
{
    XMMATRIX mvp;
    char     pad[192];
};

static_assert(sizeof(UniformBuffer) == 256);

inline void WaitForFence (ID3D12Fence* fence, UINT64 completionValue, HANDLE waitEvent) {
    if (fence->GetCompletedValue() < completionValue) {
        fence->SetEventOnCompletion (completionValue, waitEvent);
        WaitForSingleObject (waitEvent, INFINITE);
    }
}

inline UINT Align(UINT val, UINT alignment) {
    UINT misalign = val % alignment;

    if (misalign != 0) {
        val += (alignment - misalign);
    }

    return val;
}

#pragma region ClassDecl

class alignas(64) Harmony
{
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    static constexpr uint32_t DEFAULT_CHUNK_SIZE   = 128 * 1024 * 1024;

    bool Init(HINSTANCE inst);
    void Run();
    void Shutdown();
    void Resize();

private:
    inline uint32_t GetSizeInMB(UINT64 sizeInBytes) {
        return (sizeInBytes >> 20) & 0xFFFFFFFF;
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
    void DownloadData();

    void UpdateUbo();
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
    ID3D12CommandQueue*        pCopyQueue        = nullptr;
    ID3D12DescriptorHeap*      pRtvHeap          = nullptr;
    ID3D12DescriptorHeap*      pDsvHeap          = nullptr;
    ID3D12DescriptorHeap*      pSrvHeap          = nullptr;
    ID3D12DescriptorHeap*      pSmpHeap          = nullptr;
    ID3D12Resource*            pRenderTargets[MAX_FRAMES_IN_FLIGHT] = { nullptr };
    ID3D12CommandAllocator*    pCommandAllocators[MAX_FRAMES_IN_FLIGHT] = { nullptr };
    ID3D12GraphicsCommandList* pCommandList      = nullptr;
    ID3D12Fence*               pFence            = nullptr;
    ID3D12Heap*                pHeap             = nullptr;

    ID3D12RootSignature*       pRootSignature    = nullptr;
    ID3D12PipelineState*       pPipelineState    = nullptr;

    ID3D12Resource*            pConstantBuffer   = nullptr;
    ID3D12Resource*            pDepthBuffer      = nullptr;
    ID3D12Resource*            pTexture          = nullptr;
    ID3D12Resource*            pVertexBuffer     = nullptr;
    ID3D12Resource*            pIndexBuffer      = nullptr;
    ID3D12Resource*            pUploadBuffer     = nullptr;

    HINSTANCE                  hInstance         = NULL;
    HWND                       hMainWindow       = NULL;

    UINT                       frameIndex        = 0;
    HANDLE                     fenceHandle       = NULL;
    UINT64                     fenceValues[MAX_FRAMES_IN_FLIGHT] = { 0 };

    UINT                       rtvDescriptorSize = 0;
    UINT                       dsvDescriptorSize = 0;
    UINT                       srvDescriptorSize = 0;
    UINT                       smpDescriptorSize = 0;

    D3D_FEATURE_LEVEL          featureLevel      = D3D_FEATURE_LEVEL_12_2;
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

        DownloadData();
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

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
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
        std::cout << "Resource heap Tier is not Tier 2!" << std::endl;
    }

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };

    if (FAILED(pDevice9->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5)) {
        throw std::runtime_error("Shader Model 6.5 is not supported!");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};

    if (FAILED(pDevice9->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
        || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)) {
        throw std::exception("Mesh Shaders aren't supported!");
    }
}

void Harmony::CreateCommandQueue() {
    ComPtr<ID3D12CommandQueue> cmdQueue;

    {
        D3D12_COMMAND_QUEUE_DESC desc = {
            .Type     = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = 0,
            .Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0
        };

        if (FAILED(pDevice9->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue)))) {
            throw std::runtime_error("Could not create graphics command queue");
        }

        pCommandQueue = cmdQueue.Detach();

        delQ.Append([cCommandQueue = pCommandQueue] {
            cCommandQueue->Release();
        });
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {
            .Type     = D3D12_COMMAND_LIST_TYPE_COPY,
            .Priority = 0,
            .Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0
        };

        if (FAILED(pDevice9->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue)))) {
            throw std::runtime_error("Could not create copy command queue");
        }

        pCopyQueue = cmdQueue.Detach();

        delQ.Append([cCommandQueue = pCommandQueue] {
            cCommandQueue->Release();
        });
    }
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

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = MAX_FRAMES_IN_FLIGHT,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };

        if (FAILED(pDevice9->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&descriptorHeap)))) {
            throw std::runtime_error("Could not create RTV heap!");
        }

        rtvDescriptorSize = pDevice9->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        pRtvHeap = descriptorHeap.Detach();

        delQ.Append([cHeap = pRtvHeap] {
            cHeap->Release();
        });
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = 1,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
        };

        if (FAILED(pDevice9->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&descriptorHeap)))) {
            throw std::runtime_error("Could not create DSV heap!");
        }

        dsvDescriptorSize = pDevice9->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        pDsvHeap = descriptorHeap.Detach();

        delQ.Append([cHeap = pDsvHeap] {
            cHeap->Release();
        });
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 8,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        };

        if (FAILED(pDevice9->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&descriptorHeap)))) {
            throw std::runtime_error("Could not create CBV_SRV_UAV heap!");
        }

        srvDescriptorSize = pDevice9->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        pSrvHeap = descriptorHeap.Detach();

        delQ.Append([cHeap = pSrvHeap] {
            cHeap->Release();
        });
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC smpHeapDesc {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            .NumDescriptors = 8,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        };

        if (FAILED(pDevice9->CreateDescriptorHeap(&smpHeapDesc, IID_PPV_ARGS(&descriptorHeap)))) {
            throw std::runtime_error("Could not create CBV_SRV_UAV heap!");
        }

        smpDescriptorSize = pDevice9->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        pSmpHeap = descriptorHeap.Detach();

        delQ.Append([cHeap = pSmpHeap] {
            cHeap->Release();
        });
    }

    auto AllocateHeap = [cDevice = pDevice9](UINT64 chunkSize, D3D12_HEAP_TYPE type) -> ID3D12Heap* {
        D3D12_HEAP_PROPERTIES hProps = cDevice->GetCustomHeapProperties(0, type);

        D3D12_HEAP_DESC heapDesc {
            .SizeInBytes = chunkSize,
            .Properties = {.Type = type, .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN, .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN, .CreationNodeMask = hProps.CreationNodeMask , .VisibleNodeMask = hProps.VisibleNodeMask },
            .Alignment  = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Flags      = D3D12_HEAP_FLAG_NONE
        };

        ComPtr<ID3D12Heap> memHeap;
        if (FAILED(cDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&memHeap)))) {
            throw std::runtime_error("Could not create default memory heap!");
        }

        return memHeap.Detach();
    };

    pHeap = AllocateHeap(DEFAULT_CHUNK_SIZE, D3D12_HEAP_TYPE_DEFAULT);
    
    delQ.Append([cHeap = pHeap] {
        cHeap->Release();
    });
}

void Harmony::CreateResourcesAndViews() {
    UINT64 defaultHeapOffset = 0;

    {
        D3D12_RESOURCE_DESC cbDesc {
            .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width            = sizeof(UniformBuffer) * MAX_FRAMES_IN_FLIGHT,
            .Height           = 1,
            .DepthOrArraySize = 1,
            .MipLevels        = 1,
            .Format           = DXGI_FORMAT_UNKNOWN,
            .SampleDesc       = { .Count = 1, .Quality = 0 },
            .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags            = D3D12_RESOURCE_FLAG_NONE,
        };

        D3D12_HEAP_PROPERTIES uploadHeapProps {
            .Type                   = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask       = 0,
            .VisibleNodeMask        = 0
        };

        D3D12_RESOURCE_ALLOCATION_INFO resInfo = pDevice9->GetResourceAllocationInfo(0, 1, &cbDesc);
        if (resInfo.Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT) {
            cbDesc.Alignment = resInfo.Alignment;
        }

        if (FAILED(pDevice9->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pConstantBuffer)))) {
            throw std::runtime_error("Could not create constant buffer!");
        }

        delQ.Append([cbuff = pConstantBuffer] {
            cbuff->Release();
        });
    }
    
    {
        D3D12_CLEAR_VALUE dsVal {
            .Format = DXGI_FORMAT_D32_FLOAT,
            .Color = { 1.0f, 1.0f, 1.0f, 1.0f }
        };

        D3D12_RESOURCE_DESC depthDesc {
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width     = WINDOW_WIDTH,
            .Height    = WINDOW_HEIGHT,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_D32_FLOAT,
            .SampleDesc = {.Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        };

        D3D12_RESOURCE_ALLOCATION_INFO resInfo = pDevice9->GetResourceAllocationInfo(0, 1, &depthDesc);
        
        if (FAILED(pDevice9->CreatePlacedResource(pHeap, defaultHeapOffset, &depthDesc, D3D12_RESOURCE_STATE_COMMON, &dsVal, IID_PPV_ARGS(&pDepthBuffer)))) {
            throw std::runtime_error("Could not create depth buffer!");
        }

        defaultHeapOffset += resInfo.SizeInBytes;

        delQ.Append([cbuff = pDepthBuffer] {
            cbuff->Release();
        });
    }

    {
        D3D12_CLEAR_VALUE texVal{
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Color = { 0.0f, 0.0f, 0.0f, 0.0f }
        };

        D3D12_RESOURCE_DESC texDesc{
            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width = TEXTURE_WIDTH,
            .Height = TEXTURE_HEIGHT,
            .DepthOrArraySize = 1,
            .MipLevels = static_cast<UINT16>(log2(TEXTURE_HEIGHT) + 1),
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {.Count = 1, .Quality = 0 },
            .Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE,
            .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        };

        D3D12_RESOURCE_ALLOCATION_INFO resInfo = pDevice9->GetResourceAllocationInfo(0, 1, &texDesc);

        if (FAILED(pDevice9->CreatePlacedResource(pHeap, defaultHeapOffset, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, &texVal, IID_PPV_ARGS(&pTexture)))) {
            throw std::runtime_error("Could not create texture!");
        }

        defaultHeapOffset += resInfo.SizeInBytes;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = texDesc.MipLevels,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.0f
            }
        };

        pDevice9->CreateShaderResourceView(pTexture, &srvDesc, pSrvHeap->GetCPUDescriptorHandleForHeapStart());

        delQ.Append([ctex = pTexture] {
            ctex->Release();
        });
    }

    {
        D3D12_SAMPLER_DESC smpDesc {
            .Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressV       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressW       = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .MipLODBias     = 0.0f,
            .MaxAnisotropy  = 0,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor    = { 0.0f, 0.0f, 0.0f, 0.0f },
            .MinLOD         = 0.0f,
            .MaxLOD         = 0.0f 
        };

        pDevice9->CreateSampler(&smpDesc, pSmpHeap->GetCPUDescriptorHandleForHeapStart());
    }

    {
        D3D12_RESOURCE_DESC vbDesc {
            .Dimension  = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment  = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width      = sizeof(vertices),
            .Height     = 1,
            .DepthOrArraySize = 1,
            .MipLevels  = 1,
            .Format     = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {.Count = 1, .Quality = 0 },
            .Layout     = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags      = D3D12_RESOURCE_FLAG_NONE
        };

        D3D12_RESOURCE_ALLOCATION_INFO resInfo = pDevice9->GetResourceAllocationInfo(0, 1, &vbDesc);

        if (FAILED(pDevice9->CreatePlacedResource(pHeap, defaultHeapOffset, &vbDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pVertexBuffer)))) {
            throw std::runtime_error("Could not create vertex buffer!");
        }

        defaultHeapOffset += resInfo.SizeInBytes;

        delQ.Append([cbuff = pVertexBuffer] {
            cbuff->Release();
        });
    }

    {
        D3D12_RESOURCE_DESC ibDesc {
            .Dimension  = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment  = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Width      = sizeof(indices),
            .Height     = 1,
            .DepthOrArraySize = 1,
            .MipLevels  = 1,
            .Format     = DXGI_FORMAT_UNKNOWN,
            .SampleDesc = {.Count = 1, .Quality = 0 },
            .Layout     = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags      = D3D12_RESOURCE_FLAG_NONE
        };

        D3D12_RESOURCE_ALLOCATION_INFO resInfo = pDevice9->GetResourceAllocationInfo(0, 1, &ibDesc);

        if (FAILED(pDevice9->CreatePlacedResource(pHeap, defaultHeapOffset, &ibDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pIndexBuffer)))) {
            throw std::runtime_error("Could not create index buffer!");
        }

        defaultHeapOffset += resInfo.SizeInBytes;

        delQ.Append([cbuff = pIndexBuffer] {
            cbuff->Release();
        });
    }

    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (UINT i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (FAILED(pSwapChain4->GetBuffer(i, IID_PPV_ARGS(&pRenderTargets[i])))) {
                throw std::runtime_error("Could not Get swap chain buffer!");
            }

            pDevice9->CreateRenderTargetView(pRenderTargets[i], nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsViewDesc {
            .Format        = DXGI_FORMAT_D32_FLOAT,
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Flags         = D3D12_DSV_FLAG_NONE,
            .Texture2D     = { 0 }
        };

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = pDsvHeap->GetCPUDescriptorHandleForHeapStart();

        pDevice9->CreateDepthStencilView(pDepthBuffer, &dsViewDesc, dsvHandle);
    }
}

void Harmony::CreatePipelines() {
    ComPtr<ID3DBlob>            errBlob;

    // root signature has 3 params for the shader
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE rootFeatures = {};

        rootFeatures.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(pDevice9->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootFeatures, sizeof rootFeatures))) {
            rootFeatures.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1;
        }

        D3D12_ROOT_PARAMETER    rootParams[3];

        D3D12_DESCRIPTOR_RANGE  descRange[3] = {
            {.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     .NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = 0 },
            {.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     .NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = 0 },
            {.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, .NumDescriptors = 1, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = 0 },
        };

        rootParams[0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister           = 0;
        rootParams[0].Descriptor.RegisterSpace            = 0;
        rootParams[0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_VERTEX;

        rootParams[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges   = &descRange[1];
        rootParams[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        rootParams[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[2].DescriptorTable.pDescriptorRanges   = &descRange[2];
        rootParams[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rDesc {
            .NumParameters     = 3,
            .pParameters       = rootParams,
            .NumStaticSamplers = 0,
            .pStaticSamplers   = nullptr,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        };

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> errBlob;

        // FIXME: Root signature version 1_1 not working
        if (FAILED(D3D12SerializeRootSignature(&rDesc, /*rootFeatures.HighestVersion*/ D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errBlob))) {
            throw std::runtime_error(reinterpret_cast<const char*>(errBlob->GetBufferPointer()));
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
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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
        rastDesc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

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
        psoDesc.InputLayout           = { .pInputElementDescs = inputElementDesc, .NumElements = 3 };
        psoDesc.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets      = 1;
        psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc            = { .Count = 1, .Quality = 0 };
        psoDesc.NodeMask              = 0;
        psoDesc.CachedPSO             = { .pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0 };
        psoDesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

        if(FAILED(pDevice9->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)))) {
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

void Harmony::DownloadData() {
    ComPtr<ID3D12GraphicsCommandList>   uploadCommandList;
    ComPtr<ID3D12CommandAllocator>      uploadCommandAllocator;
    ComPtr<ID3D12Fence>                 uploadFence;

    HRESULT hr;
    void*   pData = nullptr;

    hr = pDevice9->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (&uploadFence));
    if (FAILED(hr)) {
        throw std::runtime_error("Could not create fence!");
    }
    
    hr = pDevice9->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS (&uploadCommandAllocator));
    if (FAILED(hr)) {
        throw std::runtime_error("Could not create command allocator!");
    }

    hr = pDevice9->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadCommandAllocator.Get (), nullptr, IID_PPV_ARGS (&uploadCommandList));
    if (FAILED(hr)) {
        throw std::runtime_error("Could not create command list!");
    }

    HANDLE uploadWaitEvent = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (uploadWaitEvent == NULL) {
        throw std::runtime_error("Could not create event!");
    }

    UINT   rowPitch     = Align(TEXTURE_WIDTH * sizeof(uint32_t), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    UINT64 textureBytes = rowPitch * TEXTURE_HEIGHT;

    UINT   meshSize     = sizeof vertices + sizeof indices;
    UINT64 textureBase  = Align(meshSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    UINT64 totalBytes   = textureBase + textureBytes;

    D3D12_SUBRESOURCE_FOOTPRINT subresFP {
        .Format   = DXGI_FORMAT_R8G8B8A8_UNORM,
        .Width    = TEXTURE_WIDTH,
        .Height   = TEXTURE_HEIGHT,
        .Depth    = 1,
        .RowPitch = rowPitch
    };

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedSFP{
        .Offset    = textureBase,
        .Footprint = subresFP
    };

    D3D12_RESOURCE_DESC bufferDesc {
        .Dimension  = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment  = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width      = totalBytes,
        .Height     = 1,
        .DepthOrArraySize = 1,
        .MipLevels  = 1,
        .Format     = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = { 1, 0 },
        .Layout     = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags      = D3D12_RESOURCE_FLAG_NONE
    };

    D3D12_HEAP_PROPERTIES uploadHeapProps {
        .Type                   = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty        = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference   = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask       = 0,
        .VisibleNodeMask        = 0
    };

    hr = pDevice9->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadBuffer));
    if (FAILED(hr)) {
        throw std::runtime_error("Could not create upload buffer!");
    }

    if (FAILED(pUploadBuffer->Map(0, 0, &pData)) || pData == nullptr) {
        throw std::runtime_error("Could not map upload buffer!");
    }

    char* px = reinterpret_cast<char*>(pData);

    memcpy_s(px, sizeof vertices, vertices, sizeof vertices);
    px += sizeof vertices;

    memcpy_s(px, sizeof indices, indices, sizeof indices);
    
    {
        px = reinterpret_cast<char*>(pData) + textureBase;

        for (UINT i = 0; i < TEXTURE_HEIGHT; ++i) {
            UINT* pRow = reinterpret_cast<UINT*>(px) + (i * TEXTURE_WIDTH);

            for (UINT j = 0; j < TEXTURE_HEIGHT; ++j) {
                if (((j / 128) % 2) == 0)
                {
                    pRow[j] = (((i / 128) % 2) == 0) ? 0xFFFFFFFF : 0x0;
                }
                else
                {
                    pRow[j] = (((i / 128) % 2) == 0) ? 0x0 : 0xFFFFFFFF;
                }
            }
        }
    }

    pUploadBuffer->Unmap(0, nullptr);
    
    // copy all
    uploadCommandList->CopyBufferRegion(pVertexBuffer, 0, pUploadBuffer, 0, sizeof vertices);
    uploadCommandList->CopyBufferRegion(pIndexBuffer, 0, pUploadBuffer, sizeof vertices, sizeof indices);

    {
        D3D12_TEXTURE_COPY_LOCATION dstLoc{
            .pResource        = pTexture,
            .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = 0,
        };

        D3D12_TEXTURE_COPY_LOCATION srcLoc{
            .pResource       = pUploadBuffer,
            .Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
            .PlacedFootprint = placedSFP
        };

        uploadCommandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    }
    
    D3D12_RESOURCE_BARRIER barrierIB {
        .Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource   = pIndexBuffer,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter  = D3D12_RESOURCE_STATE_INDEX_BUFFER,
        }
    };

    D3D12_RESOURCE_BARRIER barrierVB {
        .Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource   = pVertexBuffer,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        }
    };

    D3D12_RESOURCE_BARRIER barrierTex {
        .Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource   = pTexture,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        }
    };
    
    D3D12_RESOURCE_BARRIER barriers[] = { barrierIB, barrierVB, barrierTex };
    uploadCommandList->ResourceBarrier(3, barriers);

    uploadCommandList->Close();

    ID3D12CommandList* commandLists[] = { uploadCommandList.Get() };
    pCommandQueue->ExecuteCommandLists(1, commandLists);
    pCommandQueue->Signal(uploadFence.Get(), 1);

    WaitForFence(uploadFence.Get(), 1, uploadWaitEvent);
    uploadCommandAllocator->Reset();
    CloseHandle(uploadWaitEvent);
}

#pragma endregion

#pragma region Misc

LRESULT Harmony::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Harmony* pApp = reinterpret_cast<Harmony*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);         // close the application entirely
        return 0;

    case WM_SIZE:
        if (pApp) {
            pApp->Resize();
        }
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

#pragma endregion

#pragma region Rendering

void Harmony::Render() {
    UpdateUbo();
    PopulateCommandList();

    ID3D12CommandList* ppCmdLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(1, ppCmdLists);

    pSwapChain4->Present(1, 0);

    MoveToNextFrame();
}

void Harmony::UpdateUbo() {
    static auto epoch = std::chrono::high_resolution_clock::now();

    auto current = std::chrono::high_resolution_clock::now();
    float time   = std::chrono::duration<float, std::chrono::seconds::period>( current - epoch ).count();

    const XMVECTOR Eye = XMVectorSet( 0.0f,  0.75f, -1.5f, 0.0f );
    const XMVECTOR At  = XMVectorSet( 0.0f,  0.0f,  0.0f, 0.0f );
    const XMVECTOR Up  = XMVectorSet( 0.0f,  1.0f,  0.0f, 0.0f );

    float yDisplacement = (sin(time * 5) * 0.25f) - 0.25f;

    XMMATRIX  world      = XMMatrixRotationY(time * XMConvertToRadians(90.0f));
    world *= XMMatrixTranslation(0.0f, yDisplacement, 0.0f);

    XMMATRIX  view       = XMMatrixLookAtLH( Eye, At, Up );
    XMMATRIX  projection = XMMatrixPerspectiveFovLH(70, WINDOW_WIDTH / float(WINDOW_HEIGHT), 0.1f, 20.0f);
    
    void* pData = nullptr;
    if (FAILED(pConstantBuffer->Map(0, nullptr, &pData)) || pData == nullptr) {
        throw std::runtime_error("Could not map CB!");
    }

    UniformBuffer ubo;
    ubo.mvp = world * view * projection;

    uint8_t* pBytePtr = ((uint8_t*)pData) + frameIndex * sizeof(UniformBuffer);

    memcpy_s(pBytePtr, sizeof(XMMATRIX), &ubo.mvp, sizeof(XMMATRIX));
    
    pConstantBuffer->Unmap(0, nullptr);
}

void Harmony::PopulateCommandList() {
    pCommandAllocators[frameIndex]->Reset();
    pCommandList->Reset(pCommandAllocators[frameIndex], pPipelineState);

    pCommandList->SetGraphicsRootSignature(pRootSignature);

    D3D12_RESOURCE_BARRIER rtBarrier {
        .Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource   = pRenderTargets[frameIndex],
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET,
        }
    };

    pCommandList->ResourceBarrier(1, &rtBarrier);

    D3D12_RESOURCE_BARRIER dsBarrier {
        .Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource   = pDepthBuffer,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COMMON,
            .StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        }
    };

    pCommandList->ResourceBarrier(1, &dsBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtHandle = static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + frameIndex * rtvDescriptorSize);
    D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = pDsvHeap->GetCPUDescriptorHandleForHeapStart();

    pCommandList->OMSetRenderTargets(1, &rtHandle, FALSE, &dsHandle);

    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    pCommandList->ClearRenderTargetView(rtHandle, clearColor, 0, nullptr);
    pCommandList->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    
    pCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_INDEX_BUFFER_VIEW ibv {
        pIndexBuffer->GetGPUVirtualAddress(),
        sizeof indices,
        DXGI_FORMAT_R32_UINT,
    };

    D3D12_VERTEX_BUFFER_VIEW vbv{
        pVertexBuffer->GetGPUVirtualAddress(),
        sizeof vertices,
        sizeof Vertex
    };

    pCommandList->IASetIndexBuffer(&ibv);
    pCommandList->IASetVertexBuffers(0, 1, &vbv);

    ID3D12DescriptorHeap* pDescHeaps[2] = { pSrvHeap, pSmpHeap };

    pCommandList->SetDescriptorHeaps(2, pDescHeaps);

    pCommandList->SetGraphicsRootConstantBufferView(0, pConstantBuffer->GetGPUVirtualAddress() + frameIndex * sizeof(UniformBuffer) );
    pCommandList->SetGraphicsRootDescriptorTable(1, pSrvHeap->GetGPUDescriptorHandleForHeapStart());
    pCommandList->SetGraphicsRootDescriptorTable(2, pSmpHeap->GetGPUDescriptorHandleForHeapStart());

    pCommandList->DrawIndexedInstanced(12, 1, 0, 0, 0);

    std::swap(dsBarrier.Transition.StateBefore, dsBarrier.Transition.StateAfter);
    pCommandList->ResourceBarrier(1, &dsBarrier);

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

