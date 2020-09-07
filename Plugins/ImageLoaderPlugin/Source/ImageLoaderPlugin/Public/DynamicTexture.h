#pragma once

// See https://wiki.unrealengine.com/Procedural_Materials

//#include <memory>
#include "CoreMinimal.h"
#include "DynamicTexture.generated.h"

class UTexture2D;
struct FUpdateTextureRegion2D;


UCLASS(Blueprintable, BlueprintType)
class IMAGELOADERPLUGIN_API UDynamicTexture : public UObject
{
    GENERATED_BODY()

public:
    
    UDynamicTexture(const FObjectInitializer& ObjectInitializer);
    virtual ~UDynamicTexture();
        	
    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    void ReleaseCpuMemory();

    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    void DestroyTexture2D();

    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    bool IsCreated() const { return Created; }

    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    bool IsUploaded() const { return Uploaded; }

    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    void SetUploadPendant() { Uploaded = false; }

    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    bool CreateResource(int32 w, int32 h, EPixelFormat pixelFormat = EPixelFormat::PF_B8G8R8A8, TextureAddress TexTilingAddres = TextureAddress::TA_Wrap);

    UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    bool UpdateGpu();

	UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
	bool UpdateRandomGpu();

    //UFUNCTION(BlueprintCallable, Category = "Dynamic Texture")
    void UpdatePixels(const TArray<uint8>& PixelData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dynamic Texture")
	int32 SizeKb() const;

    static void RandomTexture(float Time, int Width, int Height, uint8* Pixels);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Texture")
    UTexture2D* Texture2D = nullptr;

private:
	
	bool            Created = false;
    FThreadSafeBool Uploaded = false;
    

	const TArray<uint8>* Pixels = nullptr;
    int32 Width;
    int32 Height;
    

	FUpdateTextureRegion2D UpdateTextureRegion;

	
};

