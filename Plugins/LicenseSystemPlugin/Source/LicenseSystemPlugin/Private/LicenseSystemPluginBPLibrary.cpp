// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "LicenseSystemPluginBPLibrary.h"
#include "LicenseSystemPlugin.h"
#include "EncryptionContextOpenSSL.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Containers/UnrealString.h"

ULicenseSystemPluginBPLibrary::ULicenseSystemPluginBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float ULicenseSystemPluginBPLibrary::LicenseSystemPluginSampleFunction(float Param)
{
	return -1;
}



bool ULicenseSystemPluginBPLibrary::LoadLicense(FString LicenseFilename, FString KeyFilename)
{
	//FString filePath = *FPaths::GamePluginsDir() + folder + "/" + name;
	//FFileHelper::SaveStringToFile(SaveTextB, *(FPaths::GameDir() + FileNameB));

	LicenseFilename = "c:/tmp/crypto/AcuteArt.aa";
	KeyFilename = "c:/tmp/crypto/AcuteArt.key";
	FString EncryptedFilename = "c:/tmp/crypto/UE4_AcuteArt.aa";
	FString DecryptedFilename = "c:/tmp/crypto/UE4_AcuteArt.json";

	 TArray<uint8> License, Key;

	if (FPaths::FileExists(LicenseFilename))
	{
		if (!FFileHelper::LoadFileToArray(License, *LicenseFilename))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load license file: %s"), *LicenseFilename);
		}
	}

	if (FPaths::FileExists(KeyFilename))
	{
		if (!FFileHelper::LoadFileToArray(Key, *KeyFilename))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not load key file: %s"), *KeyFilename);
		}
	}

	FString InputFilename = "c:/tmp/crypto/AcuteArt.json";
	TArray<uint8> InputData;
	if (!FFileHelper::LoadFileToArray(InputData, *InputFilename))
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load key file: %s"), *InputFilename);
	}

	
	TArray<uint8> Iv = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6 };
	//Iv.Append(Key.GetData(), 16);

	EPlatformCryptoResult OpenSslResult;
	FEncryptionContextOpenSSL* OpenSslCtx = new FEncryptionContextOpenSSL();



	TArray<uint8> Encrypted = OpenSslCtx->Encrypt_AES_256_CBC(InputData, Key, Iv, OpenSslResult);

	if (OpenSslResult == EPlatformCryptoResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Encryption successfully"), *InputFilename);
		FFileHelper::SaveArrayToFile(Encrypted, *EncryptedFilename);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not decrypt file: %s"), *InputFilename);
	}


	TArray<uint8> Decrypted = OpenSslCtx->Decrypt_AES_256_CBC(Encrypted, Key, Iv, OpenSslResult);
	
	if (OpenSslResult == EPlatformCryptoResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Decryption successfully"), *LicenseFilename);
		FFileHelper::SaveArrayToFile(Decrypted, *DecryptedFilename);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not decrypt file: %s"), *LicenseFilename);
	}

	return false;
}

