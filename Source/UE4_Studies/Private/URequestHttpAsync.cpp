// Fill out your copyright notice in the Description page of Project Settings.


#include "URequestHttpAsync.h"

#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"


void UURequestHttpAsync::Activate()
{
	// Create HTTP Request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	//HttpRequest->SetHeader("Content-Type", "application/json");
	HttpRequest->SetURL(URL);

	// Setup Async response
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UURequestHttpAsync::OnResponseReceived);
	//HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	//	{
	//		FString ResponseString = "";
	//		if (bSuccess)
	//		{
	//			ResponseString = Response->GetContentAsString();
	//		}
	//		this->HandleRequestCompleted(ResponseString, bSuccess);
	//	});


	// Handle actual request
	HttpRequest->ProcessRequest();
	
}




/*Assigned function on successfull http call*/
void UURequestHttpAsync::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccessful)
{
	//UE_LOG(LogTemp, Warning, TEXT("------------------- OnResponseReceived: %d"), bSuccessful);
	//UE_LOG(LogTemp, Warning, TEXT("%s"), *Response->GetContentAsString());
	Completed.Broadcast(Response->GetContentAsString(), bSuccessful);
}


void UURequestHttpAsync::HandleRequestCompleted(FString ResponseString, bool bSuccess)
{
	FString OutString;
	if (bSuccess)
	{
		// Deserialize object 
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<>::Create(ResponseString);
		FJsonSerializer::Deserialize(JsonReader, JsonObject);

		// The simplest example parsing of the plain JSON.
		// Here you can expand to fetch your specific layout of values and objects and return
		// it via a UStruct or separate params in the Completed.Broadcast()
		if (!JsonObject->TryGetStringField("MOTD", OutString))
		{
			// While response may be successful, we failed to retrieve the string field
			bSuccess = false;
		}
	}

	Completed.Broadcast(OutString, bSuccess);
}


UURequestHttpAsync* UURequestHttpAsync::AsyncRequestHTTP(UObject* WorldContextObject, FString URL)
{
	// Create Action Instance for Blueprint System
	UURequestHttpAsync* Action = NewObject<UURequestHttpAsync>();
	Action->URL = URL;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}