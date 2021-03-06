#include "ImageLoader.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

#include "Runtime/RHI/Public/RHICommandList.h"

#include <exception>
#include "nv_dds.h"


// Module loading is not allowed outside of the main thread, so we load the ImageWrapper module ahead of time.
static IImageWrapperModule* ImageWrapperModule = nullptr;

UImageLoader* UImageLoader::LoadImageFromDiskAsync(UObject* Outer, const FString& ImagePath, int32 Id)
{
	// This simply creates a new ImageLoader object and starts an asynchronous load.
	UImageLoader* Loader = NewObject<UImageLoader>();
	Loader->LoadImageAsync(Outer, ImagePath, Id);
	return Loader;
}

void UImageLoader::LoadImageAsync(UObject* Outer, const FString& ImagePath, int32 Id)
{
	// The asynchronous loading operation is represented by a Future, which will contain the result value once the operation is done.
	// We store the Future in this object, so we can retrieve the result value in the completion callback below.
	Future = LoadImageFromDiskAsync(Outer, ImagePath, [this, Id]()
	{
		// This is the same Future object that we assigned above, but later in time.
		// At this point, loading is done and the Future contains a value.
		if (Future.IsValid())
		{
			// Notify listeners about the loaded texture on the game thread.
			AsyncTask(ENamedThreads::GameThread, [this, Id]() { LoadCompleted.Broadcast(Future.Get(), Id); });
		}
	});
}

TFuture<UTexture2D*> UImageLoader::LoadImageFromDiskAsync(UObject* Outer, const FString& ImagePath, TFunction<void()> CompletionCallback)
{
	// Run the image loading function asynchronously through a lambda expression, capturing the ImagePath string by value.
	// Run it on the thread pool, so we can load multiple images simultaneously without interrupting other tasks.

	if (FPaths::GetExtension(ImagePath).Compare("dds", ESearchCase::IgnoreCase) == 0)
	{
		return Async(EAsyncExecution::ThreadPool, [=]() { return LoadDDSFromDisk(Outer, ImagePath); }, CompletionCallback);
	}
	else
	{
		return Async(EAsyncExecution::ThreadPool, [=]() { return LoadImageFromDisk(Outer, ImagePath); }, CompletionCallback);
	}
}

UTexture2D* UImageLoader::LoadImageFromDisk(UObject* Outer, const FString& ImagePath)
{
	// Check if the file exists first
	if (!FPaths::FileExists(ImagePath))
	{
		UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *ImagePath);
		return nullptr;
	}

	// Load the compressed byte data from the file
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *ImagePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *ImagePath);
		return nullptr;
	}

	if (!ImageWrapperModule)
	{
		ImageWrapperModule = FModuleManager::LoadModulePtr<IImageWrapperModule>(TEXT("ImageWrapper"));
	}

	// Detect the image type using the ImageWrapper module
	EImageFormat ImageFormat = ImageWrapperModule->DetectImageFormat(FileData.GetData(), FileData.Num());
	if (ImageFormat == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Error, TEXT("Unrecognized image file format: %s"), *ImagePath);
		return nullptr;
	}

	// Create an image wrapper for the detected image format
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule->CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper for file: %s"), *ImagePath);
		return nullptr;
	}

	// Decompress the image data
	const TArray<uint8>* RawData = nullptr;
	ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num());
	ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);
	if (RawData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to decompress image file: %s"), *ImagePath);
		return nullptr;
	}
	// Create the texture and upload the uncompressed image data
	FString TextureBaseName = TEXT("Texture_") + FPaths::GetBaseFilename(ImagePath);
	return CreateTexture(Outer, *RawData, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8, FName(*TextureBaseName));
}




UTexture2D* UImageLoader::CreateTexture(UObject* Outer, const TArray<uint8>& PixelData, int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, FName BaseName)
{
	// Shamelessly copied from UTexture2D::CreateTransient with a few modifications
	if (InSizeX <= 0 || InSizeY <= 0 ||
		(InSizeX % GPixelFormats[InFormat].BlockSizeX) != 0 ||
		(InSizeY % GPixelFormats[InFormat].BlockSizeY) != 0)
	{
		//UIL_LOG(Warning, TEXT("Invalid parameters specified for UImageLoader::CreateTexture()"));
		return nullptr;
	}

	// Most important difference with UTexture2D::CreateTransient: we provide the new texture with a name and an owner
	FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), BaseName);
	UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);

	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = InSizeX;
	NewTexture->PlatformData->SizeY = InSizeY;
	NewTexture->PlatformData->PixelFormat = InFormat;

	// Allocate first mipmap and upload the pixel data
	int32 NumBlocksX = InSizeX / GPixelFormats[InFormat].BlockSizeX;
	int32 NumBlocksY = InSizeY / GPixelFormats[InFormat].BlockSizeY;
#if 0
	FTexture2DMipMap* Mip = new(NewTexture->PlatformData->Mips) FTexture2DMipMap();
#else
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
#endif
	Mip->SizeX = InSizeX;
	Mip->SizeY = InSizeY;
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	void* TextureData = Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[InFormat].BlockBytes);
	FMemory::Memcpy(TextureData, PixelData.GetData(), PixelData.Num());
	Mip->BulkData.Unlock();

	NewTexture->UpdateResource();
	return NewTexture;

}




UTexture2D* UImageLoader::LoadDDSFromDisk(UObject* Outer, const FString& ImagePath)
{
	nv_dds::CDDSImage image;
	bool flip_image = false;

	try
	{
		image.load(TCHAR_TO_UTF8(*ImagePath), flip_image);

		if (image.get_format_dxt() != nv_dds::DXT1 && image.get_format_dxt() != nv_dds::DXT5)
		{
			UE_LOG(LogTemp, Error, TEXT("%Unsupported DXT format: %s"), *ImagePath);
			return nullptr;
		}

	}
	catch (const std::exception& ex)
	{
		UE_LOG(LogTemp, Error, TEXT("%s Failed to load nv_dds image file: %s"), ex.what(), *ImagePath);
		return nullptr;
	}


	try
	{
		int32 SizeX = image.get_width();
		int32 SizeY = image.get_height();
		int32 BlockSizeX = 4;
		int32 BlockSizeY = 4;
		int32 BlockBytes = 16;

		FString TextureBaseName = TEXT("Texture_") + FPaths::GetBaseFilename(ImagePath);
		FName TextureName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), FName(*TextureBaseName));
		UTexture2D* NewTexture = NewObject<UTexture2D>(Outer, TextureName, RF_Transient);

		NewTexture->PlatformData = new FTexturePlatformData();
		NewTexture->PlatformData->SizeX = SizeX;
		NewTexture->PlatformData->SizeY = SizeY;
		NewTexture->PlatformData->PixelFormat = (image.get_format_dxt() == nv_dds::DXT5) ? EPixelFormat::PF_DXT5 : EPixelFormat::PF_DXT1;

		// Allocate first mipmap and upload the pixel data
		int32 NumBlocksX = SizeX / BlockSizeX;
		int32 NumBlocksY = SizeY / BlockSizeY;
#if 0
		FTexture2DMipMap* Mip = new(NewTexture->PlatformData->Mips) FTexture2DMipMap();
#else
		FTexture2DMipMap* Mip = new FTexture2DMipMap();
		NewTexture->PlatformData->Mips.Add(Mip);
#endif
		Mip->SizeX = SizeX;
		Mip->SizeY = SizeY;
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* TextureData = Mip->BulkData.Realloc(image.get_size());
		FMemory::Memcpy(TextureData, image, image.get_size());
		Mip->BulkData.Unlock();
		NewTexture->UpdateResource();
		return NewTexture;
	}
	catch (const std::exception& ex)
	{
		UE_LOG(LogTemp, Error, TEXT("%s Failed to create texture from dxt file: %s"), ex.what(), *ImagePath);
		return nullptr;
	}
}





bool UImageLoader::CopyTexture(UTexture2D* SourceTexture2D, UTexture2D* DestTexture2D)
{
	if (!SourceTexture2D || !DestTexture2D)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed: (!SourceTexture2D || !DestTexture2D)"));
		return false;
	}

	FTextureResource* SrcTextureResource = SourceTexture2D->Resource;
	FTextureResource* DstTextureResource = DestTexture2D->Resource;

	if (!SrcTextureResource || !DstTextureResource)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed: (!SrcTextureResource || !DstTextureResource)"));
		return false;
	}

	ENQUEUE_RENDER_COMMAND(ResolvePixelData)
	(
		[SrcTextureResource, DstTextureResource](FRHICommandListImmediate& RHICmdList)
		{
			if (!SrcTextureResource->TextureRHI || !DstTextureResource->TextureRHI)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed: (!SrcTextureResource->TextureRHI || !DstTextureResource->TextureRHI)"));
				return false;
			}
			
			FRHICopyTextureInfo CopyInfo;

			GRHICommandList.GetImmediateCommandList().CopyTexture(
								SrcTextureResource->TextureRHI, 
								DstTextureResource->TextureRHI,
								CopyInfo);

			return true;
		}
	);

	return true;
}

