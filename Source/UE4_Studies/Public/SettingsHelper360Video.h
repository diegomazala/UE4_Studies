// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SettingsHelper360Video.generated.h"

/**
 * 
 */
UCLASS()
class UE4_STUDIES_API USettingsHelper360Video : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category=Settings)
	static void SetFixedTimeStep(float desiredDeltaTime);

	UFUNCTION(BlueprintCallable, Category=Settings)
	static void DisableFixedTimeStep();
};
