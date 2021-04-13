// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "LicenseSystemPluginBPLibrary.h"
#include "LicenseSystemPlugin.h"
#include "EncryptionContextOpenSSL.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Containers/UnrealString.h"

//#include "Serialization/JsonTypes.h"
//#include "Serialization/JsonReader.h"
//#include "Policies/PrettyJsonPrintPolicy.h"
//#include "Serialization/JsonSerializer.h"
//#include "JsonObjectConverter.h"

ULicenseSystemPluginBPLibrary::ULicenseSystemPluginBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float ULicenseSystemPluginBPLibrary::LicenseSystemPluginSampleFunction(float Param)
{
	return -1;
}

FString ULicenseSystemPluginBPLibrary::ConvertBytesToString(const TArray<uint8>& In)
{
	FString Broken = BytesToString(In.GetData(), In.Num());
	FString Fixed;
	for (int i = 0; i < Broken.Len(); i++)
	{
		const TCHAR c = Broken[i] - 1;
		Fixed.AppendChar(c);
	}
	return Fixed;
}

bool ULicenseSystemPluginBPLibrary::IsLicenseValid(FString LicenseFilename, FString KeyFilename)
{
	TArray<uint8> License, Key;

	if (FPaths::FileExists(LicenseFilename))
	{
		if (!FFileHelper::LoadFileToArray(License, *LicenseFilename))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load license file: %s"), *LicenseFilename);
			return false;
		}
	}

	if (FPaths::FileExists(KeyFilename))
	{
		if (!FFileHelper::LoadFileToArray(Key, *KeyFilename))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load key file: %s"), *KeyFilename);
			return false;
		}
	}

	
	TArray<uint8> Iv(Key.GetData(), 128 / 8);
	//TArray<uint8> Iv;
	//Iv.SetNumZeroed(16);
		

	EPlatformCryptoResult OpenSslResult;
	FEncryptionContextOpenSSL* OpenSslCtx = new FEncryptionContextOpenSSL();



	//TArray<uint8> Encrypted = OpenSslCtx->Encrypt_AES_256_CBC(InputData, Key, Iv, OpenSslResult);

	//if (OpenSslResult == EPlatformCryptoResult::Success)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Encryption successfully"), *InputFilename);
	//	FFileHelper::SaveArrayToFile(Encrypted, *EncryptedFilename);
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Could not decrypt file: %s"), *InputFilename);
	//}


	TArray<uint8> Decrypted = OpenSslCtx->Decrypt_AES_256_CBC(License, Key, Iv, OpenSslResult);
	
	//if (OpenSslResult == EPlatformCryptoResult::Success)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Decryption successfully"), *LicenseFilename);
	//	FFileHelper::SaveArrayToFile(Decrypted, *DecryptedFilename);
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Could not decrypt file: %s"), *LicenseFilename);
	//}

	FString JsonStr = ConvertBytesToString(Decrypted);

	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonStr);
	TSharedPtr<FJsonObject> JsonObject;

	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Could not parse json from license file"));
		return false;
	}
	
	TArray<FString> BeginDateStr;
	JsonObject->GetStringField("BeginDate").ParseIntoArray(BeginDateStr, TEXT("/"), true);

	TArray<FString> EndDateStr;
	JsonObject->GetStringField("EndDate").ParseIntoArray(EndDateStr, TEXT("/"), true);

	if (BeginDateStr.Num() != 3 || EndDateStr.Num() != 3)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not parse date from license file"));
		return false;
	}

	const FDateTime BeginDate(FCString::Atoi(*BeginDateStr[2]), FCString::Atoi(*BeginDateStr[1]), FCString::Atoi(*BeginDateStr[0]));
	const FDateTime EndDate(FCString::Atoi(*EndDateStr[2]), FCString::Atoi(*EndDateStr[1]), FCString::Atoi(*EndDateStr[0]));;
	const FDateTime Now = FDateTime::UtcNow();

	return (Now >= BeginDate && Now <= EndDate);
}

