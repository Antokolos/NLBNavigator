#pragma once

#include "CoreMinimal.h"
#include "nlb/presentation/AudioBackend.h"
#include "nlb/presentation/MediaSource.h"

class UAudioComponent;
class USoundBase;
class UObject;

/**
 * Звук плеера средствами Unreal. Звук ищется сначала среди ассетов проекта
 * (/Game/NLB/Sound/<имя файла без расширения> — необязательная замена, если нужна обработка
 * средствами Unreal), затем берётся из книги или пакета .nlbpack: ogg, mp3 и wav раскодирует
 * AudioDecoder и отдаёт в USoundWaveProcedural — музыку потоком, короткие эффекты целиком.
 */
class FUnrealAudioBackend : public nlb::presentation::IAudioBackend {
public:
    FUnrealAudioBackend(UObject* WorldContext, const nlb::presentation::IMediaSource* Media);
    void SetMedia(const nlb::presentation::IMediaSource* Media) { MediaSource = Media; }

    void playMusic(const nlb::presentation::MediaRef& track, bool loop, int fadeInMs) override;
    void stopMusic(int fadeOutMs) override;
    void playEffect(const nlb::presentation::MediaRef& sound) override;
    void stopEffects() override;
    void setMusicVolume(float volume) override;
    void setEffectsVolume(float volume) override;

private:
    USoundBase* Load(const nlb::presentation::MediaRef& Ref, bool bLoop);

    TWeakObjectPtr<UObject> Context;
    const nlb::presentation::IMediaSource* MediaSource;
    TWeakObjectPtr<UAudioComponent> Music;
    float EffectsVolume = 1.0f;
};
