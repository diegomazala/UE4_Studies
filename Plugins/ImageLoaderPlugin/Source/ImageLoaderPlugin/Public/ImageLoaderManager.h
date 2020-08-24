// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ImageLoaderManager.generated.h"

class UTexture2D;
class UTextureBuffer;


UCLASS(Blueprintable, BlueprintType)
class UImageLoaderManager : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Image Loader")
	static UImageLoaderManager* GetImageLoaderManager();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static void Initialize();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static void Uninitialize();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static void ReleaseAllBuffers();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
    static UTextureBuffer* LoadImageSequence(UObject* Outer, const FString& Path, bool PingPong = true, float FrameIntervalInSec = 0.033f, int32 MaxImagesCount = 0, int32 TemporalResolution = 1);
	
	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static void UnloadImageSequence(const FString& Path);

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static UTexture2D* GetTexture(const FName& Path);

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	void OnImageSequenceLoadComplete(int32 ImageCount, FName SequenceName);

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static bool LoadTextureBufferImages(UTextureBuffer* TexBuffer);

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	static bool StartImageLoading();
	static bool LoadImageFromQueue();

	UFUNCTION(BlueprintCallable, Category = "Image Loader")
	bool IsLoading() const;

	UPROPERTY(BlueprintReadWrite)
	int32	TestCount = 0;
	

	UPROPERTY(Category = MapsAndSets, BlueprintReadWrite)
	TMap<FName, UTextureBuffer*>		ImgTextureBufferMap;

	UFUNCTION()
	void OnImageLoadCompleted(UTexture2D* Texture, int32 Idx);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImageSequenceLoadCompleted, int32, ImageCount, FName, SequenceName);
	FOnImageSequenceLoadCompleted& OnImageSequenceLoadCompleted()
	{
		return ImageSequenceLoadCompleted;
	}

private:
	static UImageLoaderManager*					LoaderMngr;

	TQueue<TPair<UTextureBuffer*, int32>>		ImageLoadingQueue;
	int32										ImageLoadingQueueSize = 0;
	int32										MaxNumberOfImagesLoadingParallel = 16;
	
	bool										IsInitialized = false;

	UPROPERTY(BlueprintAssignable, Category = ImageLoader, meta = (AllowPrivateAccess = true))
	FOnImageSequenceLoadCompleted				ImageSequenceLoadCompleted;
};
