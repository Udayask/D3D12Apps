#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "ImageManager.h"
#include "TemporaryResourceTransition.h"

#include <iostream>
#include <fstream>
#include <cassert>
using namespace std;

#define VERIFY_SUCCEEDED(x)   \
    if(FAILED((x))) {   throw std::runtime_error("Failed!"); }    \


template <typename T>
inline void AlignValue(T& val, size_t alignment) {
    T r = val % alignment;
    val = r ? val + (alignment - r) : val;
}

ImageManager::ImageManager (
    void
    )
{
    m_WriteImagesOnAdd = false;
    m_ReferenceImageIndex = 0;
    m_ValidateHeapSerializationDefault = false;
}

ImageManager::ImageManager (
    bool validateHeapSerializationDefault
    ) : ImageManager()
{
    m_ValidateHeapSerializationDefault = validateHeapSerializationDefault;
}

void
ImageManager::Resize (
    size_t Size
    )
{
    m_Images.resize(Size);

    if (m_ReferenceImageIndex >= Size) {
        m_ReferenceImageIndex = 0;
    }
}

void
ImageManager::SetImage (
    size_t Index,
    Bitmap &NewBitmap
    )
{
    m_Images[Index] = NewBitmap;
}

void
ImageManager::SetImage (
    size_t Index,
    ID3D12CommandQueue *CommandQueue,
    ID3D12Device *Device,
    ID3D12Resource *Buffer,
    D3D12_RESOURCE_STATES BufferState,
    UINT Subresource
    )
{
    D3D12_RESOURCE_DESC TextureDescriptor = Buffer->GetDesc();
    assert(TextureDescriptor.Width <= MAXDWORD32);

#if ENABLE_WINDOW_OUTPUT
    const UINT64 BaseOffset = D3D12_TEXTURE_OFFSET_ALIGNMENT;
#else
    const UINT64 BaseOffset = 0;
#endif

    UINT NumRows;
    UINT64 RowSizeInBytes, RowMajorSize;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedTexture2D;
    Device->GetCopyableFootprints(
        &TextureDescriptor,
        Subresource,
        1, /* NumSubresources */
        BaseOffset,
        &PlacedTexture2D,
        &NumRows,
        &RowSizeInBytes,
        &RowMajorSize
        );

    AlignValue(RowSizeInBytes, sizeof(DWORD));
    assert (RowSizeInBytes <= PlacedTexture2D.Footprint.RowPitch);

    AlignValue(RowMajorSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    D3D12_RESOURCE_ALLOCATION_INFO HeapResAllocInf = { RowMajorSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT };

    D3D12_RESOURCE_HEAP_TIER ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1;
    D3D12_HEAP_SERIALIZATION_TIER HeapSerializationTier = D3D12_HEAP_SERIALIZATION_TIER_0;
    bool bHeapSerializationValidation = m_ValidateHeapSerializationDefault;

    if ( /*FAILED(TestData::TryGetValue(L"HeapSerializationValidation", bHeapSerializationValidation) */ FALSE || bHeapSerializationValidation )
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS FDOptions = {};
        if (SUCCEEDED( Device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS, &FDOptions, sizeof( FDOptions ) ) ))
        {
            ResourceHeapTier = FDOptions.ResourceHeapTier;
        }
        D3D12_FEATURE_DATA_SERIALIZATION FDSerial = {};
        if (SUCCEEDED( Device->CheckFeatureSupport( D3D12_FEATURE_SERIALIZATION, &FDSerial, sizeof( FDSerial ) ) ))
        {
            HeapSerializationTier = FDSerial.HeapSerializationTier;
        }

        bool bForce = false;
        if ( /*SUCCEEDED( TestData::TryGetValue(L"ForceHeapSerializationValidation", bForce)*/ FALSE && bForce)
        {
            HeapSerializationTier = D3D12_HEAP_SERIALIZATION_TIER_10;
        }
    }

    // Validate the heap serialization requirement that makes texture data agnostic of physical address;
    // and do a little sanity testing of how the heap serialization will be used.
    // Validating texture data will take a longer route.
    // Aliasing and data-inheritance will be leveraged, and texture data will be copied through overlapping buffers.
    ComPtr<ID3D12Resource> StagingResource;
    D3D12_RESOURCE_ALLOCATION_INFO TexResAllocInf = { 0, 0 };

    if (HeapSerializationTier >= D3D12_HEAP_SERIALIZATION_TIER_10
        && ResourceHeapTier >= D3D12_RESOURCE_HEAP_TIER_2
        && TextureDescriptor.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER
        )
    {
        // The CPU-accessible heap is grown to include room for two textures, copies of 'Buffer'.
        // Two buffers will fit on this heap, dividing it into two regions.
        // The first region is sized large enough to hold either the row-major footprint or a texture copy.
        // The second region is sized large enough for a texture copy.
        TexResAllocInf = Device->GetResourceAllocationInfo( 1, 1, &TextureDescriptor );
        AlignValue(HeapResAllocInf.SizeInBytes, UINT(TexResAllocInf.Alignment));
        HeapResAllocInf.SizeInBytes = max( 2 * TexResAllocInf.SizeInBytes, TexResAllocInf.SizeInBytes + HeapResAllocInf.SizeInBytes );
        HeapResAllocInf.Alignment = max( D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, TexResAllocInf.Alignment );

        // Heap Serialization needs no state barriers:
        D3D12_MESSAGE_ID Messages[] = { D3D12_MESSAGE_ID_INVALID_SUBRESOURCE_STATE, D3D12_MESSAGE_ID_RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH, };
        //D3D12Helper::PushDebugLayerIgnoreSettings(Device, Messages, _countof(Messages));
    }
    else
    {
        CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(RowMajorSize);

        VERIFY_SUCCEEDED(Device->CreateCommittedResource(
            &HeapProps,
            D3D12_HEAP_FLAG_NONE,
            &BufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&StagingResource)
            ));
    }
    const bool bValidatingHeapSerialization = (TexResAllocInf.SizeInBytes != 0);

    /*
    auto PopDL = wil::scope_exit( [bValidatingHeapSerialization, Device]()
        {
            if (bValidatingHeapSerialization)
            {
                D3D12Helper::PopDebugLayerIgnoreSettings(Device);
            }
        } );
    */

    D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = CommandQueue->GetDesc();
    ComPtr<ID3D12CommandAllocator> spCommandAllocator;

    VERIFY_SUCCEEDED(Device->CreateCommandAllocator(
        CommandQueueDesc.Type,
        IID_PPV_ARGS(&spCommandAllocator)));

    ComPtr<ID3D12GraphicsCommandList> spCommandList;

    VERIFY_SUCCEEDED(Device->CreateCommandList(
        CommandQueueDesc.NodeMask,
        CommandQueueDesc.Type,
        spCommandAllocator.Get(),
        NULL,
        IID_PPV_ARGS(&spCommandList)));

    ComPtr<ID3D12Resource> CopySrc = Buffer, HSTex0, HSTex1, Buf1;
    D3D12_RESOURCE_STATES CopySrcState = BufferState;

    if (bValidatingHeapSerialization)
    {
        // Custom heaps must be used to bypass safe-guards, preventing undefined texture layouts in CPU-accessible heaps.
        // Resource heap tier 2 allows buffers & textures to mix without declaring any heap flags.
        const D3D12_HEAP_PROPERTIES HProp = Device->GetCustomHeapProperties( 0, D3D12_HEAP_TYPE_READBACK );
        ComPtr<ID3D12Heap> StagingHeap;

        CD3DX12_HEAP_DESC  heapDesc(HeapResAllocInf, HProp);
        VERIFY_SUCCEEDED( Device->CreateHeap( &heapDesc, IID_PPV_ARGS(&StagingHeap) ));

        auto buff = CD3DX12_RESOURCE_DESC::Buffer(HeapResAllocInf.SizeInBytes - TexResAllocInf.SizeInBytes);
        VERIFY_SUCCEEDED(Device->CreatePlacedResource(
            StagingHeap.Get(),
            0,
            &buff,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&StagingResource)
            ));

        // Maximize the odds that compression stays intact, by re-using the optimized clear color passed to texture creation.
        D3D12_CLEAR_VALUE ClearValue, *pClearValue = nullptr;
        UINT PDSize = sizeof( ClearValue );
        if (SUCCEEDED( Buffer->GetPrivateData( PDID_CLEARVALUE, &PDSize, &ClearValue ) ) && PDSize == sizeof( ClearValue ))
        {
            pClearValue = &ClearValue;
        }

        VERIFY_SUCCEEDED(Device->CreatePlacedResource(
            StagingHeap.Get(),
            0,
            &TextureDescriptor,
            D3D12_RESOURCE_STATE_COPY_DEST,
            pClearValue,
            IID_PPV_ARGS(&HSTex0)
            ));

        VERIFY_SUCCEEDED(Device->CreatePlacedResource(
            StagingHeap.Get(),
            HeapResAllocInf.SizeInBytes - TexResAllocInf.SizeInBytes,
            &TextureDescriptor,
            D3D12_RESOURCE_STATE_COPY_DEST,
            pClearValue,
            IID_PPV_ARGS(&HSTex1)
            ));

        auto buff2 = CD3DX12_RESOURCE_DESC::Buffer(TexResAllocInf.SizeInBytes);
        VERIFY_SUCCEEDED(Device->CreatePlacedResource(
            StagingHeap.Get(),
            HeapResAllocInf.SizeInBytes - TexResAllocInf.SizeInBytes,
            &buff2,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&Buf1)
            ));

        TemporaryResourceTransition SrcResourceTransition(spCommandList.Get(), Buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, BufferState);

        // Heap Serialization needs no state barriers:
        spCommandList->CopyResource( HSTex0.Get(), Buffer);

        //keep the debug layer happy with the placed resource
        TemporaryResourceTransition DestResourceTransition(spCommandList.Get(), StagingResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        // The full resource barrier includes an aliasing barrier, in case such strictness ever becomes required:
        // StagingResource inherits from Tex0.
        // D3D12Helper::FullResourceBarrier( spCommandList );

        spCommandList->CopyBufferRegion( Buf1.Get(), 0, StagingResource.Get(), 0, TexResAllocInf.SizeInBytes );

        // The full resource barrier includes an aliasing barrier, in case such strictness ever becomes required:
        // Tex1 inherits from Buf1.
        // D3D12Helper::FullResourceBarrier( spCommandList );

        CopySrc = HSTex1;
        CopySrcState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    {
        // Transition the source resource to the COPY_SOURCE state
        TemporaryResourceTransition ResourceTransition(spCommandList.Get(), CopySrc.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, CopySrcState, Subresource);

        if (TextureDescriptor.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            spCommandList->CopyBufferRegion(
                StagingResource.Get(),            //ID3D12Resource *pDstBuffer
                0,                          //UINT64 DstOffset
                CopySrc.Get(),                    //ID3D12Resource *pSrcBuffer
                0,                          //INT64 SrcOffset
                TextureDescriptor.Width     //UINT64 NumBytes
                );
        }
        else
        {
            //assert(TextureDescriptor.Format != DXGI_FORMAT_UNKNOWN);

            CD3DX12_TEXTURE_COPY_LOCATION Dst(StagingResource.Get(), PlacedTexture2D);
            CD3DX12_TEXTURE_COPY_LOCATION Src(CopySrc.Get(), Subresource);

            spCommandList->CopyTextureRegion(&Dst,
                0, 0, 0,
                &Src,
                NULL
                );
        }
    }

    VERIFY_SUCCEEDED(spCommandList->Close());

    ID3D12CommandList* ppCmdList[] = { spCommandList.Get() };
    CommandQueue->ExecuteCommandLists(1, ppCmdList);

    HANDLE Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(Event != NULL);

    ComPtr<ID3D12Fence> Fence;
    VERIFY_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    VERIFY_SUCCEEDED(CommandQueue->Signal(Fence.Get(), 1));
    VERIFY_SUCCEEDED(Fence->SetEventOnCompletion(1, Event));

    WaitForSingleObject(Event, INFINITE);
    CloseHandle(Event);

    D3D12_RANGE MapRange;
    PVOID Data;
    MapRange.Begin = static_cast<SIZE_T>(PlacedTexture2D.Offset);
    MapRange.End = static_cast<SIZE_T>(PlacedTexture2D.Offset + RowMajorSize);
    VERIFY_SUCCEEDED(StagingResource->Map(0, &MapRange, &Data));

    /*
    MEMORY_BASIC_INFORMATION MBInfo;
    (void)VirtualQuery(Data, &MBInfo, sizeof(MBInfo));

    const UINT MaskedProtection = MBInfo.Protect & g_PageProtectionMask;

    assert(PAGE_READWRITE == MaskedProtection);
    assert(PlacedTexture2D.Offset < MAXDWORD32);
    */

    Bitmap NewBitmap;
    NewBitmap.Width = static_cast<DWORD>(RowSizeInBytes / sizeof(DWORD));
    NewBitmap.Height = NumRows;

    // To support surfaces that are not 128 byte aligned the data needs to be re-packed.
    NewBitmap.Pixels.resize(static_cast<SIZE_T>(NewBitmap.Width * NewBitmap.Height));
    for (UINT y = 0; y < NewBitmap.Height; y++) {
        DWORD *pScan =
            reinterpret_cast<DWORD *>(
            static_cast<BYTE *>(Data) + PlacedTexture2D.Offset + (y * PlacedTexture2D.Footprint.RowPitch)
            );

        PVOID Destination = &(NewBitmap.Pixels[static_cast<SIZE_T>(y * NewBitmap.Width)]);
        memcpy(Destination,
               pScan,
               static_cast<SIZE_T>(sizeof(DWORD) * NewBitmap.Width));
    }

    m_Images[Index] = NewBitmap;

    if (m_WriteImagesOnAdd) {
        ExportImage("Output.bmp");
    }

    StagingResource->Unmap(0, nullptr);
}

size_t
ImageManager::AddImage (
    Bitmap &Image
    )
{
    m_Images.push_back(Image);
    return m_Images.size() - 1;
}

size_t
ImageManager::AddImage (
    ID3D12CommandQueue *CommandQueue,
    ID3D12Device *Device,
    ID3D12Resource *Buffer,
    D3D12_RESOURCE_STATES BufferState,
    UINT Subresource
    )
{
    Bitmap Empty;

    m_Images.push_back(Empty);

    const size_t Index = m_Images.size() - 1;

    SetImage(Index, CommandQueue, Device, Buffer, BufferState, Subresource);
    return Index;
}

size_t
ImageManager::AddImage (
    wchar_t *ImageFilename
    )
{
    // TODO: Write me.

    UNREFERENCED_PARAMETER(ImageFilename);
    return m_Images.size() - 1;
}

void
ImageManager::SetReferenceImage (
    size_t Index
    )
{
    if (Index == -1) {
        Index = m_Images.size() - 1;
    }

    m_ReferenceImageIndex = Index;
}

void
ImageManager::PurgeAllImages (
    void
    )
{
    m_ReferenceImageIndex = 0;
    m_Images.clear();
}

bool
ImageManager::PixelsEqual(
    DWORD Pixel1,
    DWORD Pixel2
    ) 
{
    return Pixel1 == Pixel2;
}

HRESULT
ImageManager::VerifyImage (
    size_t Index,
    bool bLogError
    )
{
    if (Index == -1) {
        Index = m_Images.size() - 1;
    }

    Bitmap &Bitmap0 = m_Images[m_ReferenceImageIndex];
    Bitmap &Bitmap1 = m_Images[Index];

    assert(Bitmap0.Width == Bitmap1.Width);
    assert(Bitmap0.Height == Bitmap1.Height);

    UINT NumPixels = Bitmap0.Width * Bitmap0.Height;

    assert(Bitmap0.Pixels.size() == NumPixels);
    assert(Bitmap1.Pixels.size() == NumPixels);

    for (UINT i = 0; i < NumPixels; i++) {
        DWORD Pixel0 = Bitmap0.Pixels[i];
        DWORD Pixel1 = Bitmap1.Pixels[i];

        if (!PixelsEqual(Pixel0, Pixel1)) {
            if(bLogError)
            {
                ImageVerificationFailureCallback(m_ReferenceImageIndex, Index);
                std::cerr << "Images mismatch!";
            }

            return S_FALSE;
        }
    }

    return S_OK;
}

HRESULT
ImageManager::ExportImage (
    const char *ImageFilename,
    size_t Index
    )
{
    if (Index == -1) {
        Index = m_Images.size() - 1;
    }

    Bitmap &Bitmap = m_Images[Index];
    return WriteBitmap(ImageFilename, Bitmap);
}

HRESULT
ImageManager::ExportAllImages (
    const char *ImageDirectoryFilename,
    const char *ImagePrefix
    )
{
    char DefaultImagePrefix[] = "Image";

    if (ImagePrefix == NULL) {
        ImagePrefix = DefaultImagePrefix;
    }

    for (size_t Image = 0; Image < m_Images.size(); Image += 1) {
        basic_string<char> FilePath;
        FilePath = ImageDirectoryFilename;
        FilePath += ImagePrefix;
        //FilePath += Image;
        ExportImage(FilePath.c_str(), Image);
    }

    return S_OK;
}

HRESULT
ImageManager::CreateDifferenceBitmap (
    Bitmap &DiffImage,
    Bitmap &Bitmap0,
    Bitmap &Bitmap1
    )
{
    UINT NumPixels = Bitmap0.Width * Bitmap0.Height;

    assert(Bitmap0.Pixels.size() == NumPixels);
    assert(Bitmap1.Pixels.size() == NumPixels);

    DiffImage.Pixels.resize(NumPixels);
    DiffImage.Height = Bitmap0.Height;
    DiffImage.Width = Bitmap0.Width;

    // Set a white color for any difference in a pixel.
    for (UINT i = 0; i < NumPixels; i++) {
        DWORD Pixel0 = Bitmap0.Pixels[i];
        DWORD Pixel1 = Bitmap1.Pixels[i];

        if (!PixelsEqual(Pixel0, Pixel1)) {
            DiffImage.Pixels[i] = 0xFFFFFFFF;

        } else {
            DiffImage.Pixels[i] = 0;
        }
    }

    return S_OK;
}

HRESULT
ImageManager::CopyBitmap (
    Bitmap &Destination,
    Bitmap &Source,
    UINT WidthOffset
    )
{
    for (UINT y = 0; y < Source.Height; y += 1) {
        memcpy(&Destination.Pixels[WidthOffset + Destination.Width * y],
               &Source.Pixels[Source.Width * y],
               Source.Width * sizeof(Source.Pixels[0]));
    }

    return S_OK;
}

HRESULT
ImageManager::WriteBitmap (
    const char* FileName,
    Bitmap &Image
    )
{
    // Create the bitmap header.
    BITMAPFILEHEADER BitmapFileHeader;
    BITMAPINFOHEADER BitmapInfoHeader;

    ZeroMemory(&BitmapFileHeader, sizeof(BitmapFileHeader));
    ZeroMemory(&BitmapInfoHeader, sizeof(BitmapInfoHeader));

    BitmapFileHeader.bfType = 'MB';
    BitmapFileHeader.bfOffBits = (DWORD)sizeof(BitmapFileHeader) +
                                        sizeof(BitmapInfoHeader);
                                        
    BitmapFileHeader.bfSize = BitmapFileHeader.bfOffBits +
                              (DWORD)( sizeof(Image.Pixels[0]) *
                                       Image.Pixels.size() );

    BitmapInfoHeader.biSize = sizeof(BitmapInfoHeader);
    BitmapInfoHeader.biBitCount = sizeof(Image.Pixels[0]) * 8;
    BitmapInfoHeader.biHeight = Image.Height;
    BitmapInfoHeader.biWidth = Image.Width;
    BitmapInfoHeader.biPlanes = 1;

    // Bitmaps are flipped horizontally, flip the data.
    Bitmap FlippedImage;
    FlippedImage.Pixels.resize(Image.Width * Image.Height);
    for (UINT y = 0; y < Image.Height; y += 1) {
        memcpy(&FlippedImage.Pixels[y * Image.Width],
               &Image.Pixels[(Image.Height - y - 1) * Image.Width],
               Image.Width * sizeof(Image.Pixels[0]));
    }

    // Work around. WEX does not support appending data to a file.
    std::vector<char> Data;
    UINT Offset;

    Offset = 0;
    Data.resize(BitmapFileHeader.bfSize);
    memcpy(&Data[Offset], &BitmapFileHeader, sizeof(BitmapFileHeader));
    Offset += sizeof(BitmapFileHeader);
    memcpy(&Data[Offset], &BitmapInfoHeader, sizeof(BitmapInfoHeader));
    Offset += sizeof(BitmapInfoHeader);
    memcpy(&Data[Offset],
           &(FlippedImage.Pixels[0]),
           sizeof(Image.Pixels[0]) * Image.Pixels.size());

    std::fstream file;
    file.open(FileName, std::ios::binary | std::ios::out);

    if (file) {
        file.write(Data.data(), Data.size());
        file.close();
    }
    
    return S_OK;
}

HRESULT
ImageManager::ImageVerificationFailureCallback (
    size_t ReferenceIndex,
    size_t Index
    )
{
    Bitmap &Bitmap0 = m_Images[ReferenceIndex];
    Bitmap &Bitmap1 = m_Images[Index];

    // If reference and test bitmaps do not match in size log the fact and
    // return.
    if (Bitmap0.Height != Bitmap1.Height || Bitmap0.Width != Bitmap1.Width) {
        std::cerr <<  "The reference bitmap and verification bitmap do not match in size." << std::endl;
        return S_FALSE;
    }

    // Allocate a new bitmap 3x the size of the original bitmap and two
    // extra colums of black pixels.
    Bitmap LoggedImage;
    LoggedImage.Height = Bitmap0.Height;
    LoggedImage.Width = Bitmap0.Width * 3 + 2;
    LoggedImage.Pixels.resize(LoggedImage.Width * LoggedImage.Height);

    // Generate the diff bitmap.
    Bitmap DiffImage;
    CreateDifferenceBitmap(DiffImage, Bitmap0, Bitmap1);

    // Copy the bitmaps into the output bitmap.
    CopyBitmap(LoggedImage, Bitmap0, 0);
    CopyBitmap(LoggedImage, Bitmap1, Bitmap1.Width + 1);
    CopyBitmap(LoggedImage, DiffImage, Bitmap1.Width * 2 + 2);
    WriteBitmap("Failed.bmp", LoggedImage);
    return S_OK;
}

bool ImageManager::Empty() const {
    return m_Images.empty();
}

/*
RenderTargetCompareTest::RenderTargetCompareTest(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue)
    : m_pDevice(pDevice)
    , m_pCommandQueue(pCommandQueue)
{
    m_RTVHandle.ptr = 0;
}

RenderTargetCompareTest::~RenderTargetCompareTest()
{
}

void RenderTargetCompareTest::Execute()
{
    const UINT Count = 2;
    const UINT Width = 512;
    const UINT Height = 512;
    const D3D12_RESOURCE_STATES ResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // Call derived class to do initialization used for both test cases
    Initialize();

    // Create render target texture2Ds
    ComPtr<ID3D12Resource> pResource[Count];

    for (UINT i = 0; i < Count; i++)
    {
        VERIFY_SUCCEEDED(m_pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R8G8B8A8_UNORM,
                Width,
                Height,
                1,
                1,
                1,
                0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                ),
            ResourceState,
            nullptr,
            IID_PPV_ARGS( &(pResource[i]) )
            ));

        // Clear the resource to all 0s
        {
            std::vector<UINT> Pixels(Width * Height, 0);

            ResourceCopy RC(m_pCommandQueue);

            RC.UploadData(
                pResource[i],
                0,
                ResourceState,
                Pixels.data(),
                sizeof(Pixels[0]) * Width,
                sizeof(Pixels[0]) * Width * Height
                );
        }
    }

    ImageManager ImageMan;

    // For each test case
    for (UINT i = 0; i < Count; i++)
    {
        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            1,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            0
        };

        // Create a RTV
        CComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
        VERIFY_SUCCEEDED(m_pDevice->CreateDescriptorHeap(
            &HeapDesc,
            IID_PPV_ARGS( &pDescriptorHeap )
            ));

        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        m_pDevice->CreateRenderTargetView(
            pResource[i],
            nullptr,
            RTVHandle
            );

        // Save parameters needed by SetCommandListState
        m_RTVHandle = RTVHandle;
        m_pResource = pResource[i];

        // Call derived class to render into the resource
        ExecuteTest(i, pResource[i]);

        // Save the image in the image manager
        ImageMan.AddImage(m_pCommandQueue, m_pDevice, pResource[i], ResourceState);
    }

    // Compare the 2 images
    ImageMan.VerifyImage();
}

void RenderTargetCompareTest::SetCommandListState(ID3D12GraphicsCommandList* pCL)
{
    pCL->OMSetRenderTargets(
        1,
        &m_RTVHandle,
        TRUE,
        nullptr
        );

    D3D12_RESOURCE_DESC ResourceDesc = m_pResource->GetDesc();

    D3D12_VIEWPORT Viewport = 
    {
        0.0f,
        0.0f,
        static_cast<float>(ResourceDesc.Width),
        static_cast<float>(ResourceDesc.Height),
        0.0f,
        1.0f
    };

    pCL->RSSetViewports(1, &Viewport);

    D3D12_RECT ScissorRect = {0, 0, static_cast<int>(ResourceDesc.Width), static_cast<int>(ResourceDesc.Height)};

    pCL->RSSetScissorRects(
        1,
        &ScissorRect
        );
}
*/