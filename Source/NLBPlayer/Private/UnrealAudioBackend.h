#pragma once

#include "CoreMinimal.h"
#include "nlb/presentation/AudioBackend.h"
#include "nlb/presentation/MediaSource.h"

class UAudioComponent;
class USoundBase;
class UObject;

/**
 * Звук плеера средствами Unreal. Звук книги ищется сначала среди ассетов проекта:
 * /Game/NLB/Sound/<имя файла без расширения> (импортируйте туда ogg/mp3 из sound/ книги —
 * сжатые форматы Unreal декодирует только для импортированных ассетов). WAV из книги
 * воспроизводится и без импорта — через USoundWaveProcedural.
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
