// Fill out your copyright notice in the Description page of Project Settings.


#include "TextureBufferPlayer.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "TextureBuffer.h"
#include "ImageLoaderManager.h"


UTextureBufferPlayer::UTextureBufferPlayer()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UTextureBufferPlayer::BeginPlay()
{
	Super::BeginPlay();

	IsPlaying = false;

	if (!SetupMaterial())
		return;
	
	if (LoadOnBeginPlay)
		LoadImageSequenceFromDisk();
}

void UTextureBufferPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!UnloadOnEndPlay)
		Unload();
}


void UTextureBufferPlayer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!TextureBuffer)
		return;

	if (UpdateTextures())
		UpdateMaterial();

	if (IsPlaying)
		TextureBuffer->Update(DeltaTime);
}


bool UTextureBufferPlayer::LoadImageSequenceFromDisk()
{
	TextureBuffer = UImageLoaderManager::GetImageLoaderManager()->LoadImageSequence(this, FileListPath, PingPong, FrameIntervalInSeconds, MaxImages, TemporalResolution);
	if (TextureBuffer)
	{
		TextureBuffer->OnImageSequenceLoadInProgress().AddDynamic(this, &UTextureBufferPlayer::OnImageSequenceLoadInProgress);
		TextureBuffer->OnImageSequenceLoadCompleted().AddDynamic(this, &UTextureBufferPlayer::OnImageSequenceLoadCompleted);
		return true;
	}
	return false;
}

void UTextureBufferPlayer::OnImageSequenceLoadInProgress(int32 Count, FName SequenceName)
{
	//UE_LOG(LogTemp, Warning, TEXT("UTextureBufferPlayer::OnImageSequenceLoadInProgress: %d %s"), Count, *SequenceName.ToString());
	
	if (UpdateTextures())
		UpdateMaterial();
}


void UTextureBufferPlayer::OnImageSequenceLoadCompleted(int32 Count, FName SequenceName)
{
	//UE_LOG(LogTemp, Warning, TEXT("UTextureBufferPlayer::OnImageSequenceLoadCompleted: %d %s"), Count, *SequenceName.ToString());
	IsPlaying = true;
}



bool UTextureBufferPlayer::SetupMaterial()
{
	if (!TemplateMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("UTextureBufferPlayer::SetupMaterial: Missing reference for TemplateMaterial %s"), *GetOwner()->GetName());
		return false;
	}

	MainMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, TemplateMaterial);

	if (TryToApplyMaterialToMesh)
	{
		UMeshComponent* MeshComp = GetOwner()->FindComponentByClass<UMeshComponent>();
		if (MeshComp)
			MeshComp->SetMaterial(0, MainMaterial);
	}

	return true;
}


bool UTextureBufferPlayer::UpdateMaterial()
{
	if (!MainMaterial || !TextureBuffer)
		return false;

	float lerpAlpha = FMath::Clamp(TextureBuffer->GetTimeSinceLastUpdate() / this->FrameIntervalInSeconds, 0.0f, 1.0f);

	MainMaterial->SetTextureParameterValue("MainTexture", MainTexture);
	MainMaterial->SetTextureParameterValue("PrevTexture", PrevTexture);
	MainMaterial->SetScalarParameterValue("LerpAlpha", lerpAlpha);
	return true;
}


bool UTextureBufferPlayer::UpdateTextures()
{
	if (!TextureBuffer)
		return false;

	if (TextureBuffer->IsFinished())
	{
		MainTexture = TextureBuffer->GetTexture();
		PrevTexture = TextureBuffer->GetPrevTexture();
	}
	else
	{
		MainTexture = TextureBuffer->GetFallbackTexture();
		PrevTexture = TextureBuffer->GetFallbackTexture();
	}
	return true;
}


void UTextureBufferPlayer::Play()
{
	if (TextureBuffer)
		TextureBuffer->GoToBegin();
}

void UTextureBufferPlayer::Pause()
{
	IsPlaying = false;
}

void UTextureBufferPlayer::Resume()
{
	IsPlaying = true;
}

void UTextureBufferPlayer::PauseResume()
{
	IsPlaying = !IsPlaying;
}


void UTextureBufferPlayer::Unload()
{
	if (UImageLoaderManager::GetImageLoaderManager()->UnloadImageSequence(FileListPath))
		TextureBuffer = nullptr;
}