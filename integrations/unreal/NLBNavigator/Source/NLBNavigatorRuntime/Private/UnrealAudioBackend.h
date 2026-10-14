#pragma once

#include "CoreMinimal.h"
#include "nlb/presentation/AudioBackend.h"
#include "nlb/presentation/MediaSource.h"

class UAudioComponent;
class USoundWave;
class UObject;

/**
 * Звук NLBNavigator средствами Unreal: USoundWaveProcedural не нужен — ogg/wav из книги
 * импортируются на лету через USoundWave с данными в памяти (для ogg — сжатый формат OGG).
 * Музыка — один UAudioComponent, эффекты — PlaySound2D. Громкость — VolumeMultiplier.
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
    USoundWave* Load(const nlb::presentation::MediaRef& Ref, bool bLoop);

    TWeakObjectPtr<UObject> Context;
    const nlb::presentation::IMediaSource* MediaSource;
    TWeakObjectPtr<UAudioComponent> Music;
    TMap<FString, TWeakObjectPtr<USoundWave>> Cache;
    float EffectsVolume = 1.0f;
};
