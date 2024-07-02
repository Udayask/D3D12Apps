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

#include "directx/d3d12.h"
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;

#include "directx/d3dx12.h"
#include "shaders/ShaderAndAppCommon.h"
#include "shaders/TestShaders.h"

#include "ImageManager.h"

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#define APPLICATION_NAME        "Mesh HLK"
#define WINDOW_WIDTH            1920
#define WINDOW_HEIGHT           1080

#define VERIFY_SUCCEEDED(x)   \
    if(FAILED((x))) {   throw std::runtime_error("Failed!"); }    \

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

#pragma region ClassDecl

class alignas(64) Harmony
{
public:
    static constexpr uint32_t DEFAULT_CHUNK_SIZE   = 128 * 1024 * 1024;
    static constexpr uint32_t UPLOAD_CHUNK_SIZE    = 64 * 1024 * 1024;

    static constexpr UINT numRTs = 2;

    bool Init(HINSTANCE inst);
    void Run(const std::string& testName);
    void Shutdown();

private:
    inline uint32_t GetSizeInMB(UINT64 sizeInBytes) {
        return (sizeInBytes >> 20) & 0xFFFFFFFF;
    }

    enum MeshType {
        Small,
        Medium,
        Large,
        MediumLine,
        SmallUseViewInstancing,
        MediumUseViewInstancing,
        PSSystemValues,
        PrimitiveID,
        UseVariableRateShading,
        RayQuery,
        CullAllPrimitives
    };

    void InitVariables(void);

    void CreateRTs(UINT width, UINT height, UINT16 arraySize = 1);
    void CreateShaders(MeshType meshType,bool bWithAmpShader = false);
    void CreateShadersStateObject(bool bWithAmpShader = false);
    void FlushAndFinish(bool bNeedToClose = true);
    void SetupRenderCommon(bool bBindReferenceRTs, bool bBundle = false);
    void Draw();
    void DispatchMesh(UINT numGroups, UINT amplificationFactor);

    void VerifyImages();

    void CreateAdapter();
    void CreateDevice();
    void CreateCommandQueue();
    void CreateHeaps();
    void CreateCommandLists();
    void CreateSyncObjects();
    void CreateRootSignature();

    void WaitForGpu();
    void Render();

    DeletionQueue              delQ;

    IDXGIFactory7*              pFactory7         = nullptr;
    IDXGIAdapter1*              pAdapter1         = nullptr;

    ID3D12Device9*              pDevice9          = nullptr;
    ID3D12CommandQueue*         pCommandQueue     = nullptr;
    ID3D12CommandQueue*         pCopyQueue        = nullptr;
    ID3D12DescriptorHeap*       pRtvHeap          = nullptr;
    ID3D12DescriptorHeap*       pDsvHeap          = nullptr;
    ID3D12Resource*             pRenderTarget     = nullptr;
    ID3D12CommandAllocator*     pCommandAllocator = nullptr;
    ID3D12GraphicsCommandList7* pCommandList      = nullptr;
    ID3D12Fence*                pFence            = nullptr;

    ID3D12Heap*                 pHeaps[2]         = { nullptr }; // 0 is default, 1 is upload

    ID3D12RootSignature*        pRootSignature    = nullptr;

    ID3D12PipelineState*        pVSPS             = nullptr;
    ID3D12PipelineState*        pMeshPSO          = nullptr;
    ID3D12StateObject*          pVSPSSO           = nullptr;
    ID3D12StateObject*          pMeshSO           = nullptr;

    HINSTANCE                   hInstance         = NULL;
    HWND                        hMainWindow       = NULL;
 
    UINT                        frameIndex        = 0;
    HANDLE                      fenceHandle       = NULL;
    UINT64                      fenceValue        = 0;
    UINT64                      maxChunkSizes[2]  = { DEFAULT_CHUNK_SIZE, UPLOAD_CHUNK_SIZE };

    UINT                        rtvDescriptorSize = 0;
    UINT                        dsvDescriptorSize = 0;
    D3D_FEATURE_LEVEL           featureLevel      = D3D_FEATURE_LEVEL_12_2;

    UINT64                      defaultHeapOffset = 0;
    UINT64                      uploadHeapOffset  = 0;

    D3D12_VIEWPORT              viewport;
    D3D12_RECT                  scissorRect;

    const DXGI_FORMAT RTFormat[numRTs] = {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM};

    struct Res {
        Res():p(nullptr), state(D3D12_RESOURCE_STATE_COPY_DEST)
        {}

        ID3D12Resource* p;
        D3D12_RESOURCE_STATES state;
    };

    typedef struct RT_SET
    {
        Res RT[numRTs];
        D3D12_CPU_DESCRIPTOR_HANDLE hRTV[numRTs];
    } RT_SET;

    RT_SET m_Reference  = {};
    RT_SET m_MeshShaded = {};

    UINT m_RTWidth = 0;
    UINT m_RTHeight = 0;
    UINT m_RTArraySize = 0;

    bool m_meshShaderSupportsFullRangeRenderTargetArrayIndex = false;

    D3D12_RT_FORMAT_ARRAY m_RTFormatArray;

    D3D_PRIMITIVE_TOPOLOGY m_Topology;

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOpts;

    LPCWSTR m_VSPSProgramName = L"VsPsProgram";
    LPCWSTR m_MeshProgramName = L"MeshProgram";

    ImageManager m_ImageManager;
};

#pragma endregion

#pragma region Public Interface

bool Harmony::Init(HINSTANCE inst) {
    hInstance = inst;

    try {
        CreateAdapter();

        CreateDevice();

        CreateCommandQueue();

        CreateHeaps();

        CreateCommandLists();

        CreateSyncObjects();

        CreateRootSignature();

        InitVariables();
    }
    catch (std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        return false;
    }

    return true;
}

void Harmony::InitVariables() {

    m_RTFormatArray.RTFormats[0] = RTFormat[0];
    m_RTFormatArray.RTFormats[1] = RTFormat[1];

    for (UINT i = 2; i < 8; i++)
    {
        m_RTFormatArray.RTFormats[i] = DXGI_FORMAT_UNKNOWN;
    }

    m_RTFormatArray.NumRenderTargets = 2;
}

void Harmony::Run(const std::string &testName) {
    CreateRTs(SMALL_TEST_NUM_QUADS_PER_GROUP,SMALL_TEST_NUM_GROUPS);
    CreateShaders(Small,false);
    Draw();
    DispatchMesh(SMALL_TEST_NUM_GROUPS,1);
    VerifyImages();
}

void Harmony::Shutdown() {
    WaitForGpu();

    delQ.Finalize();
}

#pragma endregion

#pragma region Init Calls

void Harmony::CreateAdapter() {
    UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();

        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

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
}

void Harmony::CreateHeaps() {
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = 4,
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
    }
}
    
void Harmony::CreateCommandLists() {
    if (FAILED(pDevice9->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)))) {
        throw std::runtime_error("Could not allocate command allocator!");
    }
    
    ComPtr<ID3D12GraphicsCommandList7> cmdList;
    if (FAILED(pDevice9->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&cmdList)))) {
        throw std::runtime_error("Could not create command list!");
    }

    pCommandList = cmdList.Detach();

    delQ.Append([cCommandList = pCommandList, cCommandAllocator = pCommandAllocator] {
        cCommandList->Release();
        cCommandAllocator->Release();
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

void Harmony::CreateRootSignature() {
    ComPtr<ID3D12RootSignature> rs;

    if (FAILED(pDevice9->CreateRootSignature(0x1, g_VS, sizeof(g_VS), IID_PPV_ARGS(&rs)))) {
        throw std::runtime_error("Could not create root signature!");
    }

    pRootSignature = rs.Detach();

    delQ.Append([cRs= pRootSignature] {
        cRs->Release();
    });
}

#pragma endregion

#pragma region Rendering

void Harmony::CreateRTs(UINT width, UINT height, UINT16 arraySize)
{
    m_RTWidth = width;
    m_RTHeight = height;
    m_RTArraySize = arraySize;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    for(UINT s = 0; s < 2; s++)
    {
        RT_SET& set = (s==0 ? m_Reference : m_MeshShaded);

        for(UINT i = 0; i < numRTs; i++)
        {
            set.RT[i].state = D3D12_RESOURCE_STATE_RENDER_TARGET;

            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(RTFormat[i], width, height, arraySize, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            
            VERIFY_SUCCEEDED(pDevice9->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                set.RT[i].state,
                nullptr,
                IID_PPV_ARGS(&set.RT[i].p)));

            set.hRTV[i].ptr = pRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (s*2 + i) * rtvDescriptorSize;

            pDevice9->CreateRenderTargetView(set.RT[i].p, NULL, set.hRTV[i]);

            float col[] = {0.0,0.0,0.0,1.0};
            pCommandList->ClearRenderTargetView(set.hRTV[i],col,0,nullptr);

            FlushAndFinish();
        }
    }
}

void Harmony::CreateShaders(MeshType meshType, bool bWithAmpShader)
{
    if(meshType == SmallUseViewInstancing || meshType == MediumUseViewInstancing)
    {
        D3D12_VIEW_INSTANCE_LOCATION ViewLocs[VIEW_INSTANCING_TEST_NUM_VIEWS] = {};
        CD3DX12_VIEW_INSTANCING_DESC ViewInstancingDesc(VIEW_INSTANCING_TEST_NUM_VIEWS,ViewLocs,D3D12_VIEW_INSTANCING_FLAG_NONE);
        // Make VSPS PSO
        {
            struct PSO_STREAM
            {
                CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
                CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
                CD3DX12_PIPELINE_STATE_STREAM_VS VS;
                CD3DX12_PIPELINE_STATE_STREAM_PS PS;
                CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormat;
                CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING ViewInstancing;
            } Stream;

            Stream.pRootSignature = pRootSignature;
            Stream.VS = CD3DX12_SHADER_BYTECODE(g_VSViewInstancing,sizeof(g_VSViewInstancing));
            m_Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSViewInstancing,sizeof(g_PSViewInstancing));
            Stream.RTFormat = m_RTFormatArray;
            Stream.ViewInstancing = ViewInstancingDesc;
            D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
            StreamDesc.pPipelineStateSubobjectStream = &Stream;
            StreamDesc.SizeInBytes = sizeof(Stream);
            VERIFY_SUCCEEDED(pDevice9->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&pVSPS)));
        }

        if(bWithAmpShader)
        {
            struct PSO_STREAM
            {
                CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
                CD3DX12_PIPELINE_STATE_STREAM_AS AS;
                CD3DX12_PIPELINE_STATE_STREAM_MS MS;
                CD3DX12_PIPELINE_STATE_STREAM_PS PS;
                CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormat;
                CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING ViewInstancing;
            } Stream;

            Stream.pRootSignature = pRootSignature;

            if (meshType == SmallUseViewInstancing)
            {
                Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASSmallPayload,sizeof(g_ASSmallPayload));
                Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSSmallViewInstancingFromAS,sizeof(g_MSSmallViewInstancingFromAS));
            }
            else
            {
                Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASLargePayload,sizeof(g_ASLargePayload));
                Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSLargeViewInstancingFromAS,sizeof(g_MSLargeViewInstancingFromAS));
            }

            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSViewInstancing,sizeof(g_PSViewInstancing));
            Stream.RTFormat = m_RTFormatArray;
            Stream.ViewInstancing = ViewInstancingDesc;
            D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
            StreamDesc.pPipelineStateSubobjectStream = &Stream;
            StreamDesc.SizeInBytes = sizeof(Stream);
            VERIFY_SUCCEEDED(pDevice9->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&pMeshPSO)));       
        }
        else
        {
            struct PSO_STREAM
            {
                CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
                CD3DX12_PIPELINE_STATE_STREAM_MS MS;
                CD3DX12_PIPELINE_STATE_STREAM_PS PS;
                CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormat;
                CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING ViewInstancing;
            } Stream;

            Stream.pRootSignature = pRootSignature;
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSViewInstancing,sizeof(g_MSViewInstancing));
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSViewInstancing,sizeof(g_PSViewInstancing));
            Stream.RTFormat = m_RTFormatArray;
            Stream.ViewInstancing = ViewInstancingDesc;
            D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
            StreamDesc.pPipelineStateSubobjectStream = &Stream;
            StreamDesc.SizeInBytes = sizeof(Stream);
            VERIFY_SUCCEEDED(pDevice9->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&pMeshPSO)));       
        }
        return;      
    }

    // Make VSPS PSO
    if( meshType != PrimitiveID )
    {
        struct PSO_STREAM
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormat;
        } Stream;

        Stream.pRootSignature = pRootSignature;
        switch(meshType)
        {
        case MediumLine:
            Stream.VS = CD3DX12_SHADER_BYTECODE(g_VSLine,sizeof(g_VSLine));
            m_Topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;
        case PSSystemValues:
            Stream.VS = CD3DX12_SHADER_BYTECODE(g_VSSysVal,sizeof(g_VSSysVal));
            m_Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        case UseVariableRateShading:
            Stream.VS = CD3DX12_SHADER_BYTECODE(g_VSVRS, sizeof(g_VSVRS));
            m_Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        default:
            Stream.VS = CD3DX12_SHADER_BYTECODE(g_VS,sizeof(g_VS));
            m_Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        }        
        switch(meshType)
        {
        case PSSystemValues:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSSysVal,sizeof(g_PSSysVal));
            break;
        case RayQuery:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSRayQuery,sizeof(g_PSRayQuery));
            break;
        case UseVariableRateShading:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSVRS, sizeof(g_PSVRS));
            break;
        default:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PS,sizeof(g_PS));
            break;
        }        
        Stream.RTFormat = m_RTFormatArray;

        D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
        StreamDesc.pPipelineStateSubobjectStream = &Stream;
        StreamDesc.SizeInBytes = sizeof(Stream);
        VERIFY_SUCCEEDED(pDevice9->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&pVSPS)));              
    }
    if(bWithAmpShader)
    {
        struct PSO_STREAM
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_AS AS;
            CD3DX12_PIPELINE_STATE_STREAM_MS MS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormat;
        } Stream;

        Stream.pRootSignature = pRootSignature;
        switch(meshType)
        {
        case UseVariableRateShading:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSVariableRateShading, sizeof(g_MSVariableRateShading));
            break;
        case Small:
            Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASSmallPayload,sizeof(g_ASSmallPayload));
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSSmallFromAS,sizeof(g_MSSmallFromAS));
            break;
        case Medium:
            Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASLargePayload,sizeof(g_ASLargePayload));
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSMediumFromAS,sizeof(g_MSMediumFromAS));
            break;
        case Large:
            Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASLargePayload,sizeof(g_ASLargePayload));
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSLargeFromAS,sizeof(g_MSLargeFromAS));
            break;
        case MediumLine:
            Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASLargePayload,sizeof(g_ASLargePayload));
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSMediumLineFromAS,sizeof(g_MSMediumLineFromAS));
            break;
        case PrimitiveID:
            Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASPrimitiveID,sizeof(g_ASPrimitiveID));
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSPrimitiveIDFromAS,sizeof(g_MSPrimitiveIDFromAS));
            break;
        case RayQuery:
            Stream.AS = CD3DX12_SHADER_BYTECODE(g_ASRayQuery,sizeof(g_ASRayQuery));
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSRayQueryFromAS,sizeof(g_MSRayQueryFromAS));
            break;
        default:
            assert(false);
        }
        switch(meshType)
        {
        case PrimitiveID:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSPrimitiveID,sizeof(g_PSPrimitiveID));
            break;
        case RayQuery:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSRayQuery,sizeof(g_PSRayQuery));
            break;
        default:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PS,sizeof(g_PS));
            break;
        }        
        Stream.RTFormat = m_RTFormatArray;
        D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
        StreamDesc.pPipelineStateSubobjectStream = &Stream;
        StreamDesc.SizeInBytes = sizeof(Stream);

        VERIFY_SUCCEEDED(pDevice9->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&pMeshPSO)));       
    }
    else
    {
        struct PSO_STREAM
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_MS MS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTFormat;
        } Stream;

        Stream.pRootSignature = pRootSignature;
        switch(meshType)
        {
        case UseVariableRateShading:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSVariableRateShading, sizeof(g_MSVariableRateShading));
            break;
        case CullAllPrimitives:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSCullAllPrimitives, sizeof(g_MSCullAllPrimitives));
            break;
        case Small:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSSmall,sizeof(g_MSSmall));
            break;
        case Medium:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSMedium,sizeof(g_MSMedium));
            break;
        case Large:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSLarge,sizeof(g_MSLarge));
            break;
        case MediumLine:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSMediumLine,sizeof(g_MSMediumLine));
            break;
        case PSSystemValues:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSSysVal,sizeof(g_MSSysVal));
            break;
        case PrimitiveID:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSPrimitiveID,sizeof(g_MSPrimitiveID));
            break;
        case RayQuery:
            Stream.MS = CD3DX12_SHADER_BYTECODE(g_MSRayQuery,sizeof(g_MSRayQuery));
            break;
        default:
            assert(false);
        }
        switch(meshType)
        {
        case PSSystemValues:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSSysVal,sizeof(g_PSSysVal));
            break;
        case PrimitiveID:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSPrimitiveID,sizeof(g_PSPrimitiveID));
            break;
        case RayQuery:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PSRayQuery,sizeof(g_PSRayQuery));
            break;
        default:
            Stream.PS = CD3DX12_SHADER_BYTECODE(g_PS,sizeof(g_PS));
            break;
        }

        Stream.RTFormat = m_RTFormatArray;
        D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc;
        StreamDesc.pPipelineStateSubobjectStream = &Stream;
        StreamDesc.SizeInBytes = sizeof(Stream);
        VERIFY_SUCCEEDED(pDevice9->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&pMeshPSO)));       
    }
}

void Harmony::CreateShadersStateObject(bool bWithAmpShader)
{
    // PS VS state object
    {
        // Create the state object.
        CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE::D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

        // Add root signature
        auto rootSignatureSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        rootSignatureSubobject->SetRootSignature(pRootSignature);

        // Add render target formats
        auto renderTargetFormatSubobject = stateObjectDesc.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
        renderTargetFormatSubobject->SetNumRenderTargets(m_RTFormatArray.NumRenderTargets);
        for (UINT i = 0; i < m_RTFormatArray.NumRenderTargets; i++)
        {
            renderTargetFormatSubobject->SetRenderTargetFormat(i, m_RTFormatArray.RTFormats[i]);
        }

        // Add primitive topology
        auto primitiveTopologySubobject = stateObjectDesc.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
        m_Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        primitiveTopologySubobject->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

        // Add vertex shader
        auto vsSubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

        CD3DX12_SHADER_BYTECODE vs(g_VS, sizeof(g_VS));
        vsSubobject->SetDXILLibrary(&vs);
        vsSubobject->DefineExport(L"VS", L"*");

        // Add pixel shader
        auto psSubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

        CD3DX12_SHADER_BYTECODE psRayQuery(g_PSRayQuery, sizeof(g_PSRayQuery));
        psSubobject->SetDXILLibrary(&psRayQuery);
        psSubobject->DefineExport(L"PSRayQuery", L"*");

        // Create program
        auto programSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
        programSubobject->SetProgramName(m_VSPSProgramName);
        programSubobject->AddExport(L"VS");
        programSubobject->AddExport(L"PSRayQuery");
        programSubobject->AddSubobject(*renderTargetFormatSubobject);
        programSubobject->AddSubobject(*primitiveTopologySubobject);

        VERIFY_SUCCEEDED(pDevice9->CreateStateObject(&(*stateObjectDesc), IID_PPV_ARGS(&pVSPSSO)));
    }

    // MS AS state object
    {
        // Create the state object.
        CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

        // Add root signature
        auto rootSignatureSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        rootSignatureSubobject->SetRootSignature(pRootSignature);

        // Add render target formats
        auto renderTargetFormatSubobject = stateObjectDesc.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
        renderTargetFormatSubobject->SetNumRenderTargets(m_RTFormatArray.NumRenderTargets);
        for (UINT i = 0; i < m_RTFormatArray.NumRenderTargets; i++)
        {
            renderTargetFormatSubobject->SetRenderTargetFormat(i, m_RTFormatArray.RTFormats[i]);
        }

        // Create program
        auto programSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();
        programSubobject->SetProgramName(m_MeshProgramName);

        // Add shaders
        if (bWithAmpShader)
        {
            // Add amplification shader
            auto asSubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

            CD3DX12_SHADER_BYTECODE asRayQuery(g_ASRayQuery, sizeof(g_ASRayQuery));
            asSubobject->SetDXILLibrary(&asRayQuery);
            asSubobject->DefineExport(L"ASRayQuery", L"*");

            // Add mesh shader
            auto msSubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

            CD3DX12_SHADER_BYTECODE msRayQueryFromAS(g_MSRayQueryFromAS, sizeof(g_MSRayQueryFromAS));
            msSubobject->SetDXILLibrary(&msRayQueryFromAS);
            msSubobject->DefineExport(L"MSRayQueryFromAS", L"*");

            // Add exports to the program
            programSubobject->AddExport(L"ASRayQuery");
            programSubobject->AddExport(L"MSRayQueryFromAS");
        }
        else
        {
            // Add mesh shader
            auto msSubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

            CD3DX12_SHADER_BYTECODE msRayQuery(g_MSRayQuery, sizeof(g_MSRayQuery));
            msSubobject->SetDXILLibrary(&msRayQuery);
            msSubobject->DefineExport(L"MSRayQuery", L"*");

            // Add export to the program
            programSubobject->AddExport(L"MSRayQuery");
        }

        // Add pixel shader
        auto psSubobject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

        CD3DX12_SHADER_BYTECODE psRayQuery(g_PSRayQuery, sizeof(g_PSRayQuery));
        psSubobject->SetDXILLibrary(&psRayQuery);
        psSubobject->DefineExport(L"PSRayQuery", L"*");

        // Add export to the program
        programSubobject->AddExport(L"PSRayQuery");

        VERIFY_SUCCEEDED(pDevice9->CreateStateObject(&(*stateObjectDesc), IID_PPV_ARGS(&pMeshSO)));
    }
}

void Harmony::FlushAndFinish(bool bNeedToClose)
{
    if(bNeedToClose)
    {
        VERIFY_SUCCEEDED(pCommandList->Close());
    }

    ID3D12CommandList* ppCmdList[] = { reinterpret_cast<ID3D12CommandList *>(pCommandList) };

    pCommandQueue->ExecuteCommandLists(1, ppCmdList);

    VERIFY_SUCCEEDED(pCommandQueue->Signal(pFence, ++fenceValue));
    VERIFY_SUCCEEDED(pFence->SetEventOnCompletion(fenceValue, fenceHandle));

    DWORD waitResult = WaitForSingleObject(fenceHandle, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        throw std::runtime_error("Flush and finish wait failed!");
    }

    VERIFY_SUCCEEDED(pCommandAllocator->Reset());
    VERIFY_SUCCEEDED(pCommandList->Reset(pCommandAllocator, nullptr));

    /*
    if(m_bUseBundle && !bNeedToClose)
    {
    VERIFY_SUCCEEDED(m_pBundleCA->Reset());
    VERIFY_SUCCEEDED(m_pBundleCL->Reset(m_pBundleCA, nullptr));
    }
    */
}

void Harmony::SetupRenderCommon(bool bBindReferenceRTs, bool bBundle)
{
    pCommandList->SetGraphicsRootSignature(pRootSignature);

    if(m_RTWidth)
    {
        D3D12_VIEWPORT vp {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width    = (float)m_RTWidth,
            .Height   = (float)m_RTHeight,
            .MinDepth = 0,
            .MaxDepth = 1,
        };

        pCommandList->RSSetViewports(1, &vp);

        D3D12_RECT scis {
            .left   = 0,
            .top    = 0,
            .right  = (LONG)m_RTWidth,
            .bottom = (LONG)m_RTHeight,
        };

        pCommandList->RSSetScissorRects(1, &scis);
        pCommandList->OMSetRenderTargets(2, bBindReferenceRTs ? m_Reference.hRTV : m_MeshShaded.hRTV, FALSE, nullptr);
    }

    /*
    if(bBundle)
    {
    m_pBundleCL->SetGraphicsRootSignature(pRootSignature);
    }
    */
}

void Harmony::Draw()
{
    SetupRenderCommon(true);

    UINT Data[] = {m_RTWidth,m_RTHeight,m_RTArraySize};

    pCommandList->SetGraphicsRoot32BitConstants(0,_countof(Data),Data,0);
    pCommandList->SetPipelineState(pVSPS);
    pCommandList->IASetPrimitiveTopology(m_Topology);

    switch(m_Topology)
    {
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        pCommandList->DrawInstanced(m_RTWidth*6,m_RTHeight*m_RTArraySize,0,0);
        break;
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        pCommandList->DrawInstanced(m_RTWidth*2,m_RTHeight*m_RTArraySize,0,0);
        break;
    }

    FlushAndFinish();
}

void Harmony::DispatchMesh(UINT numGroups, UINT amplificationFactor)
{
    SetupRenderCommon(false, false);

    struct CommandArgs
    {
        UINT                             Constants[3];
        D3D12_DISPATCH_MESH_ARGUMENTS    DispatchMeshDesc;
    };

    CommandArgs args;
    args.Constants[0] = m_RTWidth;
    args.Constants[1] = m_RTHeight;
    args.Constants[2] = m_RTArraySize;

    if(amplificationFactor == 1)
    {
        args.DispatchMeshDesc.ThreadGroupCountX = 1;
        args.DispatchMeshDesc.ThreadGroupCountY = numGroups;
        args.DispatchMeshDesc.ThreadGroupCountZ = 1;
    }
    else // amplification
    {
        args.DispatchMeshDesc.ThreadGroupCountX = 1;
        args.DispatchMeshDesc.ThreadGroupCountY = 1;
        args.DispatchMeshDesc.ThreadGroupCountZ = amplificationFactor;
    }    

    pCommandList->SetPipelineState(pMeshPSO);

    /*
    if(m_hLogVA)
    {
    pCommandList->SetGraphicsRootUnorderedAccessView(LoggingUAVSlot, m_hLogVA);
    }
    if (m_bDoQuery)
    {
    m_Query.Initialize(pDevice9, 1, D3D12_QUERY_TYPE_PIPELINE_STATISTICS1);
    m_Query.BeginQuery(pCommandList);
    }

    if(m_bUseExecuteIndirect)
    {
    if(m_bUseBundle)
    {
    pCommandList->SetGraphicsRoot32BitConstants(0,_countof(args.Constants),args.Constants,0); 
    }
    pCLToUse->ExecuteIndirect(m_pCommandSignature,1,m_pIndirectArgumentBuffer,0,nullptr,0);
    }
    else
    */
    {
        pCommandList->SetGraphicsRoot32BitConstants(0,_countof(args.Constants),args.Constants,0); 
        pCommandList->DispatchMesh(
            args.DispatchMeshDesc.ThreadGroupCountX,
            args.DispatchMeshDesc.ThreadGroupCountY,
            args.DispatchMeshDesc.ThreadGroupCountZ);
    }

    /*if (m_bUseBundle)
    {
    VERIFY_SUCCEEDED(m_pBundleCL->Close());
    pCommandList->ExecuteBundle(m_pBundleCL);
    }
    if (m_bDoQuery)
    {
    m_Query.EndQuery(pCommandList);
    m_Query.ResolveQuery(pCommandList);
    }
    */

    VERIFY_SUCCEEDED(pCommandList->Close());

    /*
    if(m_bUseExecuteIndirect)
    {
    if(m_bUseBundle)
    {
    m_BufferHelper.CopyToResource(m_pIndirectArgumentBuffer,&args.DispatchMeshDesc,sizeof(D3D12_DISPATCH_MESH_ARGUMENTS),D3D12_RESOURCE_STATE_GENERIC_READ);
    }
    else
    {
    m_BufferHelper.CopyToResource(m_pIndirectArgumentBuffer,&args,sizeof(CommandArgs),D3D12_RESOURCE_STATE_GENERIC_READ);
    }
    }
    */

    FlushAndFinish(false);
}

void Harmony::VerifyImages()
{
    m_ImageManager.SetReferenceImage(0);

    for(UINT i = 0; i < numRTs; i++)
    {
        for(UINT a = 0; a < m_RTArraySize; a++)
        {
            m_ImageManager.PurgeAllImages();

            m_ImageManager.AddImage(pCommandQueue, pDevice9, m_Reference.RT[i].p, m_Reference.RT[i].state, a);

            if (true)
            {
                m_ImageManager.ExportImage("Reference.bmp");
            }

            m_ImageManager.AddImage(pCommandQueue, pDevice9, m_MeshShaded.RT[i].p, m_MeshShaded.RT[i].state, a);

            if (true)
            {
                m_ImageManager.ExportImage("MeshShaded.bmp");
            }

            /* VERIFY_ARE_EQUAL(S_OK,*/m_ImageManager.VerifyImage(1);
        }
    }
}

void Harmony::WaitForGpu() {
    pCommandQueue->Signal(pFence, fenceValue);

    pFence->SetEventOnCompletion(fenceValue, fenceHandle);
    WaitForSingleObjectEx(fenceHandle, INFINITE, FALSE);

    fenceValue += 1;
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

    app.Run("SmallMesh");
    app.Shutdown();

	return 0;
}


