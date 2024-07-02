#pragma once

#include "directx/d3d12.h"
#include "directx/d3dx12.h"

#include <vector>

#include <wrl.h>
using Microsoft::WRL::ComPtr;

struct Bitmap
{
    UINT Width;
    UINT Height;
    std::vector<DWORD> Pixels;

    Bitmap (
        void
        ):
        Width(0),
        Height(0)
    {

    }

    Bitmap (
        UINT TargetWidth,
        UINT TargetHeight) :
        Width(TargetWidth),
        Height(TargetHeight)
    {
        Pixels.resize(Width * Height);
    }

    Bitmap(
        UINT TargetWidth,
        UINT TargetHeight,
        UINT IntialValue) :
        Width(TargetWidth),
        Height(TargetHeight)
    {
        Pixels.resize(Width * Height);
        std::fill(Pixels.begin(), Pixels.end(), IntialValue);
    }
};

// This GUID maximizes the validation of heap serialization inside ImageManager.
// When passing textures to AddImage, store the D3D12_CLEAR_VALUE used during the texture's creation in private data for this GUID.
EXTERN_C const GUID DECLSPEC_SELECTANY PDID_CLEARVALUE =
    { 0xdd4b5b96, 0xc3f5, 0x4064, { 0xae, 0x0a, 0x28, 0x8b, 0x76, 0x20, 0x2d, 0x99 }
    };

class ImageManager {
protected:
    std::vector<Bitmap> m_Images;
    size_t m_ReferenceImageIndex;
    bool m_WriteImagesOnAdd;
    bool m_ValidateHeapSerializationDefault;

public:
    ImageManager(void);
    explicit ImageManager(bool validateHeapSerializationDefault); // See PDID_CLEARVALUE when supporting heap serialization valiation
    size_t AddImage(Bitmap &Image);
    size_t AddImage(ID3D12CommandQueue *CommandQueue, ID3D12Device *Device, ID3D12Resource *Buffer, D3D12_RESOURCE_STATES BufferState = D3D12_RESOURCE_STATE_COPY_SOURCE, UINT Subresource = 0);
    size_t AddImage(wchar_t *ImageFilename);
    void SetReferenceImage(size_t Index = -1);
    void Resize(size_t Size);
    void SetImage(size_t Index, ID3D12CommandQueue *CommandQueue, ID3D12Device *Device, ID3D12Resource *Buffer, D3D12_RESOURCE_STATES BufferState, UINT Subresource);
    void SetImage (size_t Index, Bitmap &NewBitmap);

    void PurgeAllImages(void);
    virtual HRESULT VerifyImage(size_t Index = -1, bool bLogError = true);

    HRESULT ExportImage(const char *ImageFilename, size_t Index = -1);
    HRESULT ExportAllImages(const char *ImageDirectoryFilename, const char *ImagePrefix);

    virtual HRESULT ImageVerificationFailureCallback(size_t ReferenceIndex, size_t Index);
    virtual bool PixelsEqual(DWORD Pixel1, DWORD Pixel2);

    bool Empty() const;
private:
    virtual HRESULT CreateDifferenceBitmap(Bitmap &DiffImage, Bitmap &Bitmap0, Bitmap &Bitmap1);
    HRESULT CopyBitmap(Bitmap &Destination, Bitmap &Source, UINT WidthOffset);
    HRESULT WriteBitmap(const char* FileName, Bitmap &Image);
};

// Wrapper around ImageManager
class RenderTargetCompareTest
{
public:
    RenderTargetCompareTest(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue);
    virtual ~RenderTargetCompareTest();

    void Execute();

protected:
    void SetCommandListState(ID3D12GraphicsCommandList* pCL);

    virtual void Initialize() = 0;
    virtual void ExecuteTest(UINT Index, ID3D12Resource* pResource) = 0;

    ComPtr<ID3D12Device> m_pDevice;
    ComPtr<ID3D12CommandQueue> m_pCommandQueue;

private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
    ComPtr<ID3D12Resource> m_pResource;
};
