#include "UnrealAudioBackend.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "AudioDevice.h"

FUnrealAudioBackend::FUnrealAudioBackend(UObject* WorldContext, const nlb::presentation::IMediaSource* Media)
    : Context(WorldContext), MediaSource(Media) {}

USoundWave* FUnrealAudioBackend::Load(const nlb::presentation::MediaRef& Ref, bool bLoop) {
    const FString Key = UTF8_TO_TCHAR(Ref.path.c_str());
    if (TWeakObjectPtr<USoundWave>* Found = Cache.Find(Key)) {
        if (Found->IsValid()) { (*Found)->bLooping = bLoop; return Found->Get(); }
    }
    std::vector<uint8_t> Bytes;
    if (!MediaSource || !MediaSource->read(Ref, Bytes) || Bytes.empty()) return nullptr;
    // Звук из книги без импорта: сжатые данные OGG отдаются звуковой волне напрямую.
    // Для WAV используется разбор заголовка движком (FWaveModInfo)
    USoundWave* Wave = NewObject<USoundWave>(GetTransientPackage());
    if (Key.EndsWith(TEXT(".wav"), ESearchCase::IgnoreCase)) {
        FWaveModInfo Info;
        if (!Info.ReadWaveInfo(Bytes.data(), static_cast<int32>(Bytes.size()))) return nullptr;
        Wave->NumChannels = *Info.pChannels;
        Wave->SetSampleRate(*Info.pSamplesPerSec);
        Wave->RawPCMDataSize = Info.SampleDataSize;
        Wave->RawPCMData = static_cast<uint8*>(FMemory::Malloc(Info.SampleDataSize));
        FMemory::Memcpy(Wave->RawPCMData, Info.SampleDataStart, Info.SampleDataSize);
        Wave->Duration = static_cast<float>(Info.SampleDataSize) / (Wave->NumChannels * 2 * *Info.pSamplesPerSec);
    } else {
        // OGG/MP3: декодирование средствами платформы — через импорт в ассеты проекта
        // (рекомендуется для поставки: звуки книги лучше заранее импортировать в Content).
        UE_LOG(LogTemp, Warning, TEXT("NLB: звук %s — импортируйте его в Content (нужен декодер)"), *Key);
        return nullptr;
    }
    Wave->bLooping = bLoop;
    Wave->SoundGroup = SOUNDGROUP_Default;
    Cache.Add(Key, Wave);
    return Wave;
}

void FUnrealAudioBackend::playMusic(const nlb::presentation::MediaRef& track, bool loop, int fadeInMs) {
    UObject* Ctx = Context.Get();
    USoundWave* Wave = Load(track, loop);
    if (!Ctx || !Wave) return;
    if (Music.IsValid()) Music->Stop();
    Music = UGameplayStatics::SpawnSound2D(Ctx, Wave, 1.0f, 1.0f, 0.0f, nullptr, true, false);
    if (Music.IsValid() && fadeInMs > 0) Music->FadeIn(fadeInMs / 1000.0f);
}

void FUnrealAudioBackend::stopMusic(int fadeOutMs) {
    if (!Music.IsValid()) return;
    if (fadeOutMs > 0) Music->FadeOut(fadeOutMs / 1000.0f, 0.0f);
    else Music->Stop();
}

void FUnrealAudioBackend::playEffect(const nlb::presentation::MediaRef& sound) {
    UObject* Ctx = Context.Get();
    if (USoundWave* Wave = Load(sound, false); Ctx && Wave) {
        UGameplayStatics::PlaySound2D(Ctx, Wave, EffectsVolume);
    }
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
