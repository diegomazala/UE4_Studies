// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Interfaces/IHttpRequest.h"
#include "URequestHttpAsync.generated.h"


// Event that will be the 'Completed' exec wire in the blueprint node along with all parameters as output pins.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHttpRequestCompleted, const FString&, MOTD, bool, bSuccess);

UCLASS()
class UE4_STUDIES_API UURequestHttpAsync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

protected:

	void HandleRequestCompleted(FString ResponseString, bool bSuccess);
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

public:

	/** Execute the actual load */
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "HTTP", WorldContext = "WorldContextObject"))
	static UURequestHttpAsync* AsyncRequestHTTP(UObject* WorldContextObject, FString URL);

	UPROPERTY(BlueprintAssignable)
	FOnHttpRequestCompleted Completed;

	// URL to send GET request to 
	FString URL;
	
};
