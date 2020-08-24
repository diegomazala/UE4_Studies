#pragma once

#include "CoreMinimal.h"
#include "TextureBuffer.generated.h"


UENUM(BlueprintType)
enum class ETextureBufferStatus : uint8
{
	E_Unloaded 	UMETA(DisplayName = "Unloaded"),
	E_Enqueued  UMETA(DisplayName = "Enqueued"),
	E_Loading 	UMETA(DisplayName = "Loading"),
	E_Loaded	UMETA(DisplayName = "Loaded")
};

class UTexture2D;

UCLASS(Blueprintable, BlueprintType)
class IMAGELOADERPLUGIN_API UTextureBuffer : public UObject
{
	GENERATED_BODY()

public:
		
	UTextureBuffer(const FObjectInitializer& ObjectInitializer);
	virtual ~UTextureBuffer();

	UFUNCTION(BlueprintCallable, Category = "Image Loader") 
	bool IsLoading() const;
	
	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	bool IsFinished() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Image Loader")
	float GetTimeSinceLastUpdate();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	void Update(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	UTexture2D* GetTexture();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	UTexture2D* GetPrevTexture();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	UTexture2D* GetNextTexture();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	UTexture2D* GetFallbackTexture();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	int32 MoveNext();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	bool IsEmpty() const;

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	bool IsPaused() const;
	
	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	void Pause();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	void Resume();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	void Play();


	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	bool LoadImageSequence();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	void ReleaseBuffer();

	UPROPERTY(BlueprintReadWrite)
	float FrameIntervalInSec = 0.0333f;

	UPROPERTY(BlueprintReadWrite)
	bool PingPong = false;

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> FileList;

	UPROPERTY(BlueprintReadWrite)
	int32 LoadingCount = 0;


	UFUNCTION()
	void OnImageLoadCompleted(UTexture2D* Texture, int32 Id);

	UPROPERTY(BlueprintReadWrite)
	UTexture2D* FallbackTexture = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<UTexture2D*> TexBuffer;

	UPROPERTY(BlueprintReadWrite)
	ETextureBufferStatus Status = ETextureBufferStatus::E_Unloaded;

    
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImageSequenceLoadCompleted, int32, ImageCount, FName, SequenceName);
	FOnImageSequenceLoadCompleted& OnImageSequenceLoadCompleted()
	{
		return ImageSequenceLoadCompleted;
	}

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImageSequenceLoadInProgress, int32, ImageCount, FName, SequenceName);
	FOnImageSequenceLoadInProgress& OnImageSequenceLoadInProgress()
	{
		return ImageSequenceLoadInProgress;
	}
    
	UPROPERTY(BlueprintReadWrite)
	FName SequenceName;

private:

	int32 UpdateIndex = 0;

	double LastUpdateTime = 0.0;
	bool Reverse = false;
	bool IsPlaying = false;

    UPROPERTY(BlueprintAssignable, Category = ImageLoader, meta = (AllowPrivateAccess = true))
    FOnImageSequenceLoadCompleted ImageSequenceLoadCompleted;

	UPROPERTY(BlueprintAssignable, Category = ImageLoader, meta = (AllowPrivateAccess = true))
	FOnImageSequenceLoadInProgress ImageSequenceLoadInProgress;

	
};
