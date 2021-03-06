#include "ImageLoaderManager.h"
#include "TextureBuffer.h"
#include "ImageLoader.h"
#include "Engine.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"
#include "Runtime/Core/Public/HAL/FileManagerGeneric.h"
#include "Runtime/Core/Public/Misc/Paths.h"

UImageLoaderManager* UImageLoaderManager::LoaderMngr = nullptr;



static TArray<FString> GetAllFilesInDirectory(const FString directory, const bool fullPath = true, const FString onlyFilesStartingWith = TEXT(""), const FString onlyFilesEndingWith = TEXT(""));



UImageLoaderManager::UImageLoaderManager(const FObjectInitializer& ObjectInitializer)// : Super(ObjectInitializer)
{
	LoaderMngr = this;

#if WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("ImageLoaderManager::UImageLoaderManager ------------ WITH_EDITOR -------------"));
#endif
}

UImageLoaderManager* UImageLoaderManager::GetImageLoaderManager()
{
	return LoaderMngr;
}




void UImageLoaderManager::Release()
{
	for (auto& Elem : LoaderMngr->ImgTextureBufferMap)
	{
		UTextureBuffer* ImgSeq = (UTextureBuffer*) Elem.Value;
		if (ImgSeq)
		{
			ImgSeq->ReleaseBuffer();
			ImgSeq = nullptr;
		}
	}
	LoaderMngr->ImgTextureBufferMap.Empty();
	LoaderMngr->ImageLoadingQueueSize = 0;
	LoaderMngr->ImagePreLoadingQueueSize = 0;
	LoaderMngr->ImageLoadingPriorityQueue.Empty();
	LoaderMngr->ImageLoadingQueue.Empty();
}




UTextureBuffer* UImageLoaderManager::LoadImageSequence(UObject* Outer, const FString& Path, bool PingPong, float FrameIntervalInSec, int32 MaxImagesCount, int32 TemporalResolution)
{
	if (Path.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ImageLoaderManager: File list path is null"));
		return nullptr;
	}

	if (!FPaths::FileExists(Path) && !FPaths::DirectoryExists(Path))
	{
		UE_LOG(LogTemp, Error, TEXT("ImageLoaderManager: File list or directory does not exist: %s"), *Path);
		return nullptr;
	}
	
	FName SequenceName = FName(*FPaths::GetPathLeaf(Path));

	if (LoaderMngr->ImgTextureBufferMap.Contains(SequenceName))
	{
		return &(*LoaderMngr->ImgTextureBufferMap[SequenceName]);
	}
	else
	{
		TArray<FString> FileList;
		if (FPaths::DirectoryExists(Path))
		{
			FileList = GetAllFilesInDirectory(Path);
		}
		else if (!FFileHelper::LoadFileToStringArray(FileList, *Path))
		{
			UE_LOG(LogTemp, Error, TEXT("ImageLoaderManager: Could not load path %s"), *Path);
			return nullptr;
		}

		if (FileList.Num() < 1)
		{
			UE_LOG(LogTemp, Error, TEXT("ImageLoaderManager: The path is invalid or there is no file in it %s"), *Path);
			return nullptr;
		}

		if (PingPong)
		{
			FileList.SetNum(FileList.Num() / 2 + 1, true);
		}

		if (MaxImagesCount > 0 && MaxImagesCount < FileList.Num())
		{
			FileList.SetNum(MaxImagesCount, true);
		}

		if (TemporalResolution < 1)
			TemporalResolution = 1;

		TArray<FString> FileListNewRes;
		for (int32 Idx = 0; Idx < FileList.Num(); Idx += TemporalResolution)
		{
			FileListNewRes.Add(FileList[Idx]);
		}
		FileList = FileListNewRes;
		
		
        FName TexBufferName = MakeUniqueObjectName(Outer, UTexture2D::StaticClass(), SequenceName);
		UTextureBuffer* TexBuffer = NewObject<UTextureBuffer>(Outer, UTextureBuffer::StaticClass(), TexBufferName);
		TexBuffer->SequenceName = SequenceName;
		LoaderMngr->ImgTextureBufferMap.Add(SequenceName, TexBuffer);
        
        TexBuffer->PingPong = PingPong;
        TexBuffer->FrameIntervalInSec = FrameIntervalInSec;
		TexBuffer->FileList = FileList;
		TexBuffer->LoadImageSequence();

		return TexBuffer;
	}
}

bool UImageLoaderManager::UnloadImageSequence(const FString& Path)
{
	FName SequenceName = FName(*FPaths::GetPathLeaf(Path));

	if (LoaderMngr->ImgTextureBufferMap.Contains(SequenceName))
	{
		UTextureBuffer*& TexBuffer = LoaderMngr->ImgTextureBufferMap[SequenceName];

		if (!TexBuffer->IsLoading())
		{
			LoaderMngr->ImgTextureBufferMap.Remove(SequenceName);
			TexBuffer->ReleaseBuffer();
			TexBuffer = nullptr;
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UImageLoaderManager::UnloadImageSequence Cannot unload because it is still loading images %s"), *TexBuffer->SequenceName.ToString());
			return false;
		}
	}
	return false;
}

bool UImageLoaderManager::IsLoading() const
{
	return !LoaderMngr->ImageLoadingQueue.IsEmpty();
}

bool UImageLoaderManager::LoadTextureBufferImages(UTextureBuffer* TexBuffer)
{
	TexBuffer->Status = ETextureBufferStatus::E_Enqueued;


	LoaderMngr->ImageLoadingPriorityQueue.Enqueue(TPair<UTextureBuffer*, int32>(TexBuffer, 0));
	LoaderMngr->ImagePreLoadingQueueSize++;

	for (int32 Idx = 1; Idx < TexBuffer->FileList.Num(); ++Idx)
	{
		LoaderMngr->ImageLoadingQueue.Enqueue(TPair<UTextureBuffer*, int32>(TexBuffer, Idx));
		LoaderMngr->ImagePreLoadingQueueSize++;
	}

	StartImageLoading();

	return true;
}


bool UImageLoaderManager::StartImageLoading()
{
	bool success = true;
	for (auto i = LoaderMngr->ImageLoadingQueueSize; i < LoaderMngr->MaxNumberOfImagesLoadingParallel; ++i)
		success = success && LoadImageFromQueue();

	return success;
}


bool UImageLoaderManager::LoadImageFromQueue()
{
	TPair<UTextureBuffer*, int32> pair;

	if (LoaderMngr->ImageLoadingPriorityQueue.Dequeue(pair))
	{
		LoaderMngr->ImagePreLoadingQueueSize--;

		UTextureBuffer* TexBuffer = pair.Key;
		int32 Idx = pair.Value;

		if (TexBuffer)
		{
			UImageLoader* ImageLoader = UImageLoader::LoadImageFromDiskAsync(TexBuffer, TexBuffer->FileList[Idx], Idx);
			ImageLoader->OnLoadCompleted().AddDynamic(LoaderMngr, &UImageLoaderManager::OnImageLoadCompleted);
			ImageLoader->OnLoadCompleted().AddDynamic(TexBuffer, &UTextureBuffer::OnImageLoadCompleted);
			LoaderMngr->ImageLoadingQueueSize++;
			
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UImageLoaderManager::LoadImageFromQueue: Invalid buffer"));
		}
	}

	if (LoaderMngr->ImageLoadingQueue.Dequeue(pair))
	{
		LoaderMngr->ImagePreLoadingQueueSize--; 

		UTextureBuffer* TexBuffer = pair.Key;
		int32 Idx = pair.Value;

		if (TexBuffer)
		{
			UImageLoader* ImageLoader = UImageLoader::LoadImageFromDiskAsync(TexBuffer, TexBuffer->FileList[Idx], Idx);
			ImageLoader->OnLoadCompleted().AddDynamic(LoaderMngr, &UImageLoaderManager::OnImageLoadCompleted);
			ImageLoader->OnLoadCompleted().AddDynamic(TexBuffer, &UTextureBuffer::OnImageLoadCompleted);
			LoaderMngr->ImageLoadingQueueSize++;
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UImageLoaderManager::LoadImageFromQueue: Invalid buffer"));
		}
	}

	return false;
}


void UImageLoaderManager::OnImageLoadCompleted(UTexture2D* Texture, int32 Idx)
{
	LoaderMngr->ImageLoadingQueueSize--;
	LoadImageFromQueue();
}


void UImageLoaderManager::OnImageSequenceLoadComplete(int32 ImageCount, FName SequenceName)
{
}


UTexture2D* UImageLoaderManager::GetTexture(const FName& Path)
{
    UTextureBuffer** ImgTexElem = LoaderMngr->ImgTextureBufferMap.Find(Path);

	if (!ImgTexElem)
		return nullptr;

	UTextureBuffer* TexBuffer = *ImgTexElem;

	return TexBuffer->GetTexture();
}




static TArray<FString> GetAllFilesInDirectory(const FString directory, const bool fullPath, const FString onlyFilesStartingWith, const FString onlyFilesEndingWith)
{
	// Get all files in directory
	TArray<FString> directoriesToSkip;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, directoriesToSkip, directoriesToSkip, false);
	PlatformFile.IterateDirectory(*directory, Visitor);
	TArray<FString> files;

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		const FString filePath = TimestampIt.Key();
		const FString fileName = FPaths::GetCleanFilename(filePath);
		bool shouldAddFile = true;

		// Check if filename starts with required characters
		if (!onlyFilesStartingWith.IsEmpty())
		{
			const FString left = fileName.Left(onlyFilesStartingWith.Len());

			if (!(fileName.Left(onlyFilesStartingWith.Len()).Equals(onlyFilesStartingWith)))
				shouldAddFile = false;
		}

		// Check if file extension is required characters
		if (!onlyFilesEndingWith.IsEmpty())
		{
			if (!(FPaths::GetExtension(fileName, false).Equals(onlyFilesEndingWith, ESearchCase::IgnoreCase)))
				shouldAddFile = false;
		}

		// Add full path to results
		if (shouldAddFile)
		{
			files.Add(fullPath ? filePath : fileName);
		}
	}

	return files;
}


void UImageLoaderManager::FindFiles(TArray<FString>& FoundFiles, const FString Directory, const FString FileExtension)
{
	IFileManager::Get().FindFiles(FoundFiles, *Directory, *FileExtension);
}
