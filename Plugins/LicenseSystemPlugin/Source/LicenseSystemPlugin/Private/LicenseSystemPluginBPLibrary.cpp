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
	LicenseFilename = "c:/tmp/crypto/aes.enc";
	KeyFilename = "c:/tmp/crypto/aes.key";
	FString EncryptedFilename = "c:/tmp/crypto/UE4_AcuteArt.aa";
	FString DecryptedFilename = "c:/tmp/crypto/UE4_aes.json";

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

	
	TArray<uint8> Iv(Key.GetData(), 128 / 8);;
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

