#include "DynamicTexture.h"

#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"

// See https://wiki.unrealengine.com/Procedural_Materials


struct UpdateTextureRegionsParams
{
	UTexture2D* Texture;
	int32 MipIndex;
	uint32 NumRegions;
	const FUpdateTextureRegion2D* Regions;
	uint32 SrcPitch;
	uint32 SrcBpp;
	const uint8* SrcData;
	bool FreeData;
};

// Send the render command to update the texture
void UpdateTextureRegions(UpdateTextureRegionsParams& params, FThreadSafeBool& updated)
{
	if (params.Texture && params.Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			uint32 NumRegions;
			const FUpdateTextureRegion2D* Regions;
			uint32 SrcPitch;
			uint32 SrcBpp;
			const uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*) params.Texture->Resource;
		RegionData->MipIndex = params.MipIndex;
		RegionData->NumRegions = params.NumRegions;
		RegionData->Regions = params.Regions;
		RegionData->SrcPitch = params.SrcPitch;
		RegionData->SrcBpp = params.SrcBpp;
		RegionData->SrcData = params.SrcData;

        bool FreeData = params.FreeData;
        ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)([RegionData, FreeData, &updated](FRHICommandListImmediate& RHICmdList)
            {
				for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
				{
					int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
					if (RegionData->MipIndex >= CurrentFirstMip)
					{
						RHIUpdateTexture2D(RegionData->Texture2DResource->GetTexture2DRHI(), RegionData->MipIndex - CurrentFirstMip,
							RegionData->Regions[RegionIndex], RegionData->SrcPitch,
							RegionData->SrcData + RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch +
								RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp);
					}
				}
				//if (FreeData)
				//{
				//	FMemory::Free(RegionData->Regions);
				//	FMemory::Free(RegionData->SrcData);
				//}
				delete RegionData;
                updated = true;
			});
	}
}

UDynamicTexture::UDynamicTexture(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UDynamicTexture::~UDynamicTexture()
{
	//if (UpdateTextureRegion)
	//{
	//	delete UpdateTextureRegion;
	//	UpdateTextureRegion = nullptr;
	//}

    //Pixels.Empty();

	//DestroyTexture2D();
}

void UDynamicTexture::DestroyTexture2D()
{
    if (Texture2D)
    {
        if (Texture2D->IsValidLowLevel())
        {
            Texture2D->ReleaseResource();
            Texture2D->ConditionalBeginDestroy();	// instantly clears UObject out of memory
        }
        Texture2D = nullptr;
    }
    
    Width = 0;
    Height = 0;

    Uploaded = false;
    Created = false;
}

void UDynamicTexture::ReleaseCpuMemory()
{
	//if (UpdateTextureRegion)
	//{
	//	delete UpdateTextureRegion;
	//	UpdateTextureRegion = nullptr;
	//}

    //Pixels.Empty();
}

int32 UDynamicTexture::SizeKb() const
{
	if (Pixels)
		return (Pixels->Num() * sizeof(uint8)) >> 10;
	else
		return 0;
}

bool UDynamicTexture::CreateResource(int32 width, int32 height, EPixelFormat pixelFormat, TextureAddress TexTilingAddres)
{
    //DestroyTexture2D();

    if (width < 1 || height < 1)
    {
        UE_LOG(LogTemp, Error, TEXT("-- DynamicTexture::Initialize : Invalid image for DynamicTexture initialization"));
        return false;
    }

    Width = width;
    Height = height;

    // Texture2D setup
    if (Texture2D == nullptr)
    {
        Texture2D = UTexture2D::CreateTransient(Width, Height, pixelFormat);
        Texture2D->CompressionSettings = TextureCompressionSettings::TC_Default;
        Texture2D->NeverStream = 0;
        Texture2D->AddressX = TexTilingAddres; // TextureAddress::TA_Clamp;
        Texture2D->AddressY = TexTilingAddres; // TextureAddress::TA_Clamp;
        //Texture2D->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
        // Turn off Gamma correction
        Texture2D->SRGB = 1;
        // Make sure it never gets garbage collected
        //Texture2D->AddToRoot();
        // Update the texture with these new settings
        Texture2D->UpdateResource();
    }

    // TextureRegion setup
    UpdateTextureRegion = FUpdateTextureRegion2D(0, 0, 0, 0, Width, Height);

    Created = true;
    return Created;
}

bool UDynamicTexture::UpdateGpu()
{
    if (Created)
    {
        UpdateTextureRegionsParams params = {
            /*Texture = */ Texture2D,
            /*MipIndex = */ 0,
            /*NumRegions = */ 1,
            /*Regions = */ &UpdateTextureRegion,
            /*SrcPitch = */ static_cast<uint32>(Width * sizeof(uint8) * 4),
            /*SrcBpp = */ sizeof(uint8) * 4,
            /*SrcData = */ reinterpret_cast<const uint8*>(Pixels->GetData()),
            /*FreeData = */ false,
        };
        UpdateTextureRegions(params, Uploaded);
        return true;
    }

    return false;
}


bool UDynamicTexture::UpdateRandomGpu()
{
    static float gTime = 0.f;
	if (Created)
	{
		gTime += 0.1f;
		uint8* px = new uint8[Width * Height * 4];
		UDynamicTexture::RandomTexture(gTime, Width, Height, px);

		UpdateTextureRegionsParams params = {
			/*Texture = */ Texture2D,
			/*MipIndex = */ 0,
			/*NumRegions = */ 1,
			/*Regions = */ &UpdateTextureRegion,
			/*SrcPitch = */ static_cast<uint32>(Width * sizeof(uint8) * 4),
			/*SrcBpp = */ sizeof(uint8) * 4,
			///*SrcData = */ reinterpret_cast<const uint8*>(Pixels->GetData()),
			/*SrcData = */ px,
			/*FreeData = */ false,
		};
		UpdateTextureRegions(params, Uploaded);

		delete[] px;

		return true;
	}

	return false;
}


void UDynamicTexture::UpdatePixels(const TArray<uint8>& PixelData)
{
    Pixels = &PixelData;
}



void UDynamicTexture::RandomTexture(float Time, int Width, int Height, uint8* Pixels)
{
    //Time += 0.1f;
    
    const float t = Time;

    int textureRowPitch = Width * 4;	// 4 bpp

    unsigned char* dst = (unsigned char*)Pixels;

    for (int y = 0; y < Height; ++y)
    {
        unsigned char* ptr = dst;
        for (int x = 0; x < Width; ++x)
        {
            // Simple "plasma effect": several combined sine waves
            int vv = int((127.0f + (127.0f * sinf(x / 7.0f + t))) + (127.0f + (127.0f * sinf(y / 5.0f - t))) +
                (127.0f + (127.0f * sinf((x + y) / 6.0f - t))) +
                (127.0f + (127.0f * sinf(sqrtf(float(x * x + y * y)) / 4.0f - t)))) /
                4;

            // Write the texture pixel
            ptr[0] = vv;
            ptr[1] = vv;
            ptr[2] = vv;
            ptr[3] = vv;

            // To next pixel (our pixels are 4 bpp)
            ptr += 4;
        }

        // To next image row
        dst += textureRowPitch;
    }
}