#include "UnrealAudioBackend.h"

#include "Audio.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"

FUnrealAudioBackend::FUnrealAudioBackend(UObject* WorldContext, const nlb::presentation::IMediaSource* Media)
    : Context(WorldContext), MediaSource(Media) {}

USoundBase* FUnrealAudioBackend::Load(const nlb::presentation::MediaRef& Ref, bool bLoop) {
    const FString File = FPaths::GetCleanFilename(UTF8_TO_TCHAR(Ref.path.c_str()));
    const FString Base = FPaths::GetBaseFilename(File);
    // 1) импортированный ассет /Game/NLB/Sound/<имя>
    const FString AssetPath = FString::Printf(TEXT("/Game/NLB/Sound/%s.%s"), *Base, *Base);
    if (USoundWave* Asset = LoadObject<USoundWave>(nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet)) {
        Asset->bLooping = bLoop;
        return Asset;
    }
    // 2) WAV прямо из книги
    if (!File.EndsWith(TEXT(".wav"), ESearchCase::IgnoreCase) || !MediaSource) {
        UE_LOG(LogTemp, Warning, TEXT("NLB: звук %s не найден — импортируйте его в /Game/NLB/Sound"), *File);
        return nullptr;
    }
    std::vector<uint8_t> Bytes;
    if (!MediaSource->read(Ref, Bytes) || Bytes.empty()) return nullptr;
    FWaveModInfo Info;
    if (!Info.ReadWaveInfo(Bytes.data(), static_cast<uint32>(Bytes.size()))) return nullptr;
    USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
    Wave->SetSampleRate(*Info.pSamplesPerSec);
    Wave->NumChannels = *Info.pChannels;
    Wave->Duration = INDEFINITELY_LOOPING_DURATION;
    Wave->SoundGroup = SOUNDGROUP_Default;
    Wave->bLooping = false;
    TArray<uint8> Pcm(Info.SampleDataStart, static_cast<int32>(Info.SampleDataSize));
    Wave->QueueAudio(Pcm.GetData(), Pcm.Num());
    if (bLoop) {
        // зациклить: когда очередь кончается, положить те же данные ещё раз
        Wave->OnSoundWaveProceduralUnderflow.BindLambda([Pcm](USoundWaveProcedural* W, int32) {
            W->QueueAudio(Pcm.GetData(), Pcm.Num());
        });
    }
    return Wave;
}

void FUnrealAudioBackend::playMusic(const nlb::presentation::MediaRef& track, bool loop, int fadeInMs) {
    UObject* Ctx = Context.Get();
    USoundBase* Sound = Load(track, loop);
    if (!Ctx || !Sound) return;
    if (Music.IsValid()) Music->Stop();
    Music = UGameplayStatics::SpawnSound2D(Ctx, Sound, 1.0f, 1.0f, 0.0f, nullptr, true, false);
    if (Music.IsValid() && fadeInMs > 0) Music->FadeIn(fadeInMs / 1000.0f);
}

void FUnrealAudioBackend::stopMusic(int fadeOutMs) {
    if (!Music.IsValid()) return;
    if (fadeOutMs > 0) Music->FadeOut(fadeOutMs / 1000.0f, 0.0f);
    else Music->Stop();
}

void FUnrealAudioBackend::playEffect(const nlb::presentation::MediaRef& sound) {
    UObject* Ctx = Context.Get();
    USoundBase* Sound = Load(sound, false);
    if (Ctx && Sound) UGameplayStatics::PlaySound2D(Ctx, Sound, EffectsVolume);
}

void FUnrealAudioBackend::stopEffects() {
    // PlaySound2D не возвращает компонент; короткие эффекты доиграют сами
}

void FUnrealAudioBackend::setMusicVolume(float volume) {
    if (Music.IsValid()) Music->SetVolumeMultiplier(volume);
}

void FUnrealAudioBackend::setEffectsVolume(float volume) {
    EffectsVolume = volume;
}
