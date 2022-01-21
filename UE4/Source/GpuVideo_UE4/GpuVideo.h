#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "RenderResource.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Compression/lz4.h"
#include "GpuVideo.generated.h"

 // GpuVideoのデータ
UCLASS(Blueprintable, BlueprintType)
class GPUVIDEO_UE4_API UGpuVideo : public UObject
{
	GENERATED_BODY()

public:
	UGpuVideo() {
	}

	~UGpuVideo() {
	}

	struct Lz4Block {
	public:
		uint64_t address;
		uint64_t size;
	};

	// 動画の総フレーム数
	int frameCount;
	float framePerSeconds;
	int frameBytes;
	int format;

	// 動画を表示するテクスチャ
	UTexture2D* texture;
	FTexture2DMipMap* mipmap;
	FUpdateTextureRegion2D* region;

	// 動画のデータ
	TArray<uint8> buffer;
	TArray<Lz4Block> blocks;
	uint8* lz4Buffer;
	uint8* textureBuffer;

	// 動画の画素数
	int width;
	int height;

};


UCLASS()
class GPUVIDEO_UE4_API UGpuVideoFunction : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	template<class T>
	static void Seek(const TArray<uint8>* buffer, int seekHead, T& result) {
		unsigned char bytes[sizeof(T)];
		T value;

		for (int i = 0; i < sizeof(T); i++) {
			bytes[i] = buffer->GetData()[seekHead + i];
		}

		FMemory::Memcpy(&value, bytes, sizeof(T));
		result = value;
	}

	static EPixelFormat IntToPixelFormat(int format) {
		switch (format) {
		case 1:
			return EPixelFormat::PF_DXT1;
		case 3:
			return EPixelFormat::PF_DXT3;
		case 5:
			return EPixelFormat::PF_DXT5;
		default:
			return EPixelFormat::PF_ETC2_RGBA;
		}
	}

	UFUNCTION(BlueprintCallable, Category = "GpuVideo")
		static UGpuVideo* LoadGV(FString filePath) {
		UGpuVideo* video = NewObject<UGpuVideo>();
		FFileHelper::LoadFileToArray(video->buffer, *filePath);

		int head = 0;

		// Chunk読み取り
		Seek(&video->buffer, head, video->width);
		head += sizeof(video->width);
		Seek(&video->buffer, head, video->height);
		head += sizeof(video->height);
		Seek(&video->buffer, head, video->frameCount);
		head += sizeof(video->frameCount);
		Seek(&video->buffer, head, video->framePerSeconds);
		head += sizeof(video->framePerSeconds);
		Seek(&video->buffer, head, video->format);
		head += sizeof(video->format);
		Seek(&video->buffer, head, video->frameBytes);

		// フレームのデータを読み出す
		head = video->buffer.Num() - video->frameCount * sizeof(UGpuVideo::Lz4Block);

		uint64_t maxSize = 0;

		video->blocks.Empty();
		for (int i = 0; i < video->frameCount; i++) {
			UGpuVideo::Lz4Block block;

			Seek(&video->buffer, head, block.address);
			head += sizeof(block.address);
			Seek(&video->buffer, head, block.size);
			head += sizeof(block.size);

			video->blocks.Add(block);
			maxSize = maxSize < block.size ? block.size : maxSize;
		}

		video->lz4Buffer = new uint8[maxSize];
		video->textureBuffer = new uint8[video->frameBytes];

		// UEのテクスチャを作る
		video->texture = UTexture2D::CreateTransient(video->width, video->height, IntToPixelFormat(video->format));
		//video->texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
		video->texture->SRGB = 1;
		video->texture->Filter = TextureFilter::TF_Bilinear;
		video->texture->AddToRoot(); // GC回避
		video->texture->UpdateResource();
		video->mipmap = new FTexture2DMipMap();
		video->mipmap->SizeX = video->width;
		video->mipmap->SizeY = video->height;
		video->texture->PlatformData->Mips.Add(video->mipmap);
		video->texture->PlatformData->SizeX = video->width;
		video->texture->PlatformData->SizeY = video->height;
		video->texture->PlatformData->SetNumSlices(1);
		video->texture->PlatformData->PixelFormat = IntToPixelFormat(video->format);
		video->region = new FUpdateTextureRegion2D(0, 0, 0, 0, video->width, video->height);

		video->AddToRoot();

		return video;
	}

	UFUNCTION(BlueprintCallable, Category = "GpuVideo")
	static UTexture2D* RenderGpuVideoToTexture(float time, UGpuVideo* video) {
		int frame = video->framePerSeconds * time;
		frame = FMath::Clamp(frame, 0, video->frameCount - 1);

		auto block = video->blocks.GetData()[frame];

		uint8* bufferPtr = video->buffer.GetData();
		bufferPtr += block.address;
		FMemory::Memcpy(video->lz4Buffer, bufferPtr, block.size);

		// LZ4で展開する
		LZ4_decompress_safe((char*)video->lz4Buffer, (char*)video->textureBuffer, block.size, video->frameBytes);

		// Textureにコピーする
		video->texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		uint8* data = (uint8*)(video->texture->PlatformData->Mips[0].BulkData.Realloc(video->frameBytes));
		FMemory::Memcpy(data, video->textureBuffer, video->frameBytes);
		video->texture->PlatformData->Mips[0].BulkData.Unlock();

		// Textureを動的に更新する
		video->texture->UpdateTextureRegions(0, 1, video->region, video->width * 2, 4, data);

		return video->texture;
	}

	UFUNCTION(BlueprintCallable, Category = "GpuVideo")
	static float GetGpuVideoLength(UGpuVideo* video) {
		return video->frameCount / video->framePerSeconds;
	}
};
