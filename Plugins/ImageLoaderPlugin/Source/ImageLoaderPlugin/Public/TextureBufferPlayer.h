// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TextureBufferPlayer.generated.h"

class UTextureBuffer;
class UTexture2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (ImageLoader), meta = (BlueprintSpawnableComponent))
class IMAGELOADERPLUGIN_API UTextureBufferPlayer : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UTextureBufferPlayer();

protected:
	
	virtual void BeginPlay() override;

public:	
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = TextureBufferPlayer)
	void LoadImageSequenceFromDisk();

	UFUNCTION()
	void OnImageSequenceLoadInProgress(int32 Count, FName SequenceName);

	UFUNCTION()
	void OnImageSequenceLoadCompleted(int32 Count, FName SequenceName);

	UFUNCTION(BlueprintCallable, Category = TextureBufferPlayer)
	void Unload();

	UFUNCTION(BlueprintCallable, Category = TextureBufferPlayer)
	void Play();

	UFUNCTION(BlueprintCallable, Category = TextureBufferPlayer)
	void Pause();

	UFUNCTION(BlueprintCallable, Category = TextureBufferPlayer)
	void Resume();


	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	UMaterialInterface* TemplateMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	FString FileListPath;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	bool LoadOnStart = true;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	bool PingPong = true;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	float FrameIntervalInSeconds = 0.03f;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	int32 TemporalResolution = 1;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	int32 MaxImages = 0;

	UPROPERTY(EditAnywhere, Category = TextureBufferPlayer)
	bool TryToApplyMaterialToMesh = true;

private:

	bool SetupMaterial();
	bool UpdateMaterial();
	bool UpdateTextures();


	UTextureBuffer*				TextureBuffer = nullptr;
	UMaterialInstanceDynamic*	MainMaterial = nullptr;
	UTexture2D*					MainTexture = nullptr;
	UTexture2D*					PrevTexture = nullptr;
	bool						IsPlaying = false;

};
