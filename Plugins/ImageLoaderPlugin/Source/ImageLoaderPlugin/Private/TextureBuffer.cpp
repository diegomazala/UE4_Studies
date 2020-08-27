#include "TextureBuffer.h"
#include "ImageLoaderManager.h"
#include "ImageLoader.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"



UTextureBuffer::UTextureBuffer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UTextureBuffer::~UTextureBuffer()
{
	//UE_LOG(LogTemp, Warning, TEXT("UTextureBuffer::~UTextureBuffer: %d %s"), FileList.Num(), *SequenceName.ToString());
	ReleaseBuffer();
	FallbackTexture = nullptr;
}

bool UTextureBuffer::IsLoading() const
{
	return (Status == ETextureBufferStatus::E_Loading);
}

bool UTextureBuffer::IsFinished() const
{
	return (Status == ETextureBufferStatus::E_Loaded);
}

float UTextureBuffer::GetTimeSinceLastUpdate()
{
	return static_cast<float>(FPlatformTime::Seconds() - LastUpdateTime);
}

void UTextureBuffer::Update(float DeltaTime)
{
	//
	// Check if it's time interval minimum for update
	//
    {
        double TimeDiff = FPlatformTime::Seconds() - LastUpdateTime;
        if (TimeDiff < FrameIntervalInSec)
        {
            return;
        }
        LastUpdateTime = FPlatformTime::Seconds();
    }

	MoveNext();
	
	//UE_LOG(LogTemp, Warning, TEXT("UTextureBuffer::Update: %d %s %s %s"), UpdateIndex, *GetNextTexture()->GetName(), *GetTexture()->GetName(), *GetPrevTexture()->GetName());
	
}

void UTextureBuffer::GoToBegin()
{
	UpdateIndex = (!Reverse) ? 0 : TexBuffer.Num() - 1;
}

int32 UTextureBuffer::GetIndex() const
{
	return UpdateIndex;
}
void UTextureBuffer::SetIndex(int32 Idx)
{
	UpdateIndex = FMath::Clamp(Idx, 0, TexBuffer.Num() - 1);
}


int32 UTextureBuffer::MoveNext()
{
	if (!PingPong)
	{
		++UpdateIndex;
		if (UpdateIndex >= TexBuffer.Num())
			UpdateIndex = 0;
	}
	else
	{
		if (!Reverse)
		{
			++UpdateIndex;
			if (UpdateIndex >= TexBuffer.Num())
			{
				UpdateIndex = TexBuffer.Num() - 1;
				Reverse = true;
			}
		}
		else
		{
			--UpdateIndex;
			if (UpdateIndex < 0)
			{
				UpdateIndex = 0;
				Reverse = false;
			}
		}
	}

	return UpdateIndex;
}

UTexture2D* UTextureBuffer::GetTexture()
{
	if (TexBuffer.Num() > 0 && UpdateIndex < TexBuffer.Num() && UpdateIndex > -1)
		return TexBuffer[UpdateIndex];

    return GetFallbackTexture();
}

UTexture2D* UTextureBuffer::GetPrevTexture()
{
	if (TexBuffer.Num() < 1)
		return GetFallbackTexture();

	if (!Reverse)
	{
		int32 PrevIndex = FMath::Clamp(UpdateIndex - 1, 0, TexBuffer.Num() - 1);
		return (TexBuffer[PrevIndex] ? TexBuffer[PrevIndex] : GetFallbackTexture());
	}
	else
	{
		int32 PrevIndex = FMath::Clamp(UpdateIndex + 1, 0, TexBuffer.Num() - 1);
		return (TexBuffer[PrevIndex] ? TexBuffer[PrevIndex] : GetFallbackTexture());
	}
}

UTexture2D* UTextureBuffer::GetNextTexture()
{
	if (TexBuffer.Num() < 1)
		return GetFallbackTexture();

	if (Reverse)
	{
		int32 NextIndex = FMath::Clamp(UpdateIndex - 1, 0, TexBuffer.Num() - 1);
		return (TexBuffer[NextIndex] ? TexBuffer[NextIndex] : GetFallbackTexture());
	}
	else
	{
		int32 NextIndex = FMath::Clamp(UpdateIndex + 1, 0, TexBuffer.Num() - 1);
		return (TexBuffer[NextIndex] ? TexBuffer[NextIndex] : GetFallbackTexture());
	}
}

UTexture2D* UTextureBuffer::GetFallbackTexture()
{
	return FallbackTexture;
}

bool UTextureBuffer::IsEmpty() const
{
	ImageSequenceLoadCompleted.Broadcast(TexBuffer.Num(), FName(*this->GetName()));
	return (TexBuffer.Num() == 0);
}


bool UTextureBuffer::LoadImageSequence()
{
	if (Status == ETextureBufferStatus::E_Loading)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTextureBuffer::LoadImageSequence: Loading in course. %s"), *GetName());
		return false;
	}

	if (Status == ETextureBufferStatus::E_Loaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTextureBuffer::LoadImageSequence: Already loaded. %s"), *GetName());
		return false;
	}

	LoadingCount = 0;
	
	//TexBuffer.Empty();

	if (FileList.Num() < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Error UTextureBuffer::OnImageLoadCompleted TexBuffer.Num() < 1 %d %d"), TexBuffer.Num(), FileList.Num());
	}

	UImageLoaderManager::GetImageLoaderManager()->LoadTextureBufferImages(this);

	return true;
}

void UTextureBuffer::OnImageLoadCompleted(UTexture2D* Texture, int32 Id)
{
	++LoadingCount;
	//UE_LOG(LogTemp, Warning, TEXT("%d UTextureBuffer::OnImageLoadCompleted %s"), Id, *FileList[Id]);

	// Check if at least one image has been loaded
	if (LoadingCount == 1)
	{
		Status = ETextureBufferStatus::E_Loading;
		ImageSequenceLoadInProgress.Broadcast(TexBuffer.Num(), FName(*this->GetName()));

		TexBuffer.Empty();
		TexBuffer.AddDefaulted(FileList.Num());
	}

	if (TexBuffer.Num() < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("%d Error UTextureBuffer::OnImageLoadCompleted TexBuffer.Num() < 1 %d %d"), Id, TexBuffer.Num(), FileList.Num());
		return;
	}

	// Check if the texture was loaded correctly
	if (Texture)
	{
		TexBuffer[Id] = Texture;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Error UTextureBuffer::OnImageLoadCompleted Texture==nullptr <Failed> %d %s"), Id, *FileList[Id]);
	}


	if (FallbackTexture == nullptr && Texture != nullptr)
		FallbackTexture = Texture;

	if (LoadingCount == TexBuffer.Num())
	{
		UpdateIndex = 0;
		
        UE_LOG(LogTemp, Warning, TEXT(">> %d %d UTextureBuffer::LoadImageSequence: <Completed> : %s"), FileList.Num(), LoadingCount, *SequenceName.ToString());
		UImageLoaderManager::GetImageLoaderManager()->OnImageSequenceLoadComplete(FileList.Num(), SequenceName);

		Status = ETextureBufferStatus::E_Loaded;
		ImageSequenceLoadCompleted.Broadcast(TexBuffer.Num(), FName(*this->GetName()));
	}
}




void UTextureBuffer::ReleaseBuffer()
{
	//UE_LOG(LogTemp, Warning, TEXT("UTextureBuffer::ReleaseBuffer: %d %d %s"), FileList.Num(), LoadingCount, *GetName());
    FallbackTexture = GetTexture();
	TexBuffer.Empty();
	Status = ETextureBufferStatus::E_Unloaded;
}
