#include "UnrealAudioBackend.h"

#include "Audio/AudioDecoder.h"
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
    // 1) импортированный ассет /Game/NLB/Sound/<имя> — если есть, он важнее файла из книги
    const FString AssetPath = FString::Printf(TEXT("/Game/NLB/Sound/%s.%s"), *Base, *Base);
    if (USoundWave* Asset = LoadObject<USoundWave>(nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_Quiet)) {
        Asset->bLooping = bLoop;
        return Asset;
    }
    // 2) файл из книги или пакета: ogg/mp3/wav раскодирует AudioDecoder (stb_vorbis, dr_mp3, dr_wav)
    if (!MediaSource) return nullptr;
    auto Bytes = std::make_shared<std::vector<uint8_t>>();
    if (!MediaSource->read(Ref, *Bytes) || Bytes->empty()) {
        UE_LOG(LogTemp, Warning, TEXT("NLB: нет звука %s в книге"), *File);
        return nullptr;
    }
    std::shared_ptr<nlb::audio::AudioDecoder> Decoder = nlb::audio::AudioDecoder::open(Ref.path, Bytes);
    if (!Decoder) {
        UE_LOG(LogTemp, Warning, TEXT("NLB: звук %s не раскодирован (поддерживаются ogg, mp3, wav)"), *File);
        return nullptr;
    }
    USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
    Wave->SetSampleRate(static_cast<uint32>(Decoder->sampleRate()));
    Wave->NumChannels = Decoder->channels();
    Wave->SoundGroup = SOUNDGROUP_Default;
    Wave->bLooping = false;  // зацикливание делает сам декодер (перемотка в начало)
    const int32 Channels = Decoder->channels();

    // Короткий эффект — целиком сразу; музыка и длинные звуки — потоком, по мере проигрывания
    if (!bLoop && Decoder->lengthSeconds() > 0 && Decoder->lengthSeconds() <= 10.0) {
        const std::vector<int16_t> Pcm = Decoder->decodeAll();
        Wave->Duration = static_cast<float>(Decoder->lengthSeconds());
        Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.data()), static_cast<int32>(Pcm.size() * sizeof(int16_t)));
        return Wave;
    }
    Wave->Duration = bLoop ? INDEFINITELY_LOOPING_DURATION : static_cast<float>(Decoder->lengthSeconds());
    // Подкачка: звуковой поток Unreal просит отсчёты, когда очередь кончается (вызов — в потоке
    // звука; декодер после первой порции используется только там)
    auto Feed = [Decoder, bLoop, Channels](USoundWaveProcedural* W, int32 SamplesRequired) {
        const size_t Frames = static_cast<size_t>(FMath::Max(SamplesRequired / FMath::Max(Channels, 1), 4096));
        std::vector<int16_t> Buffer(Frames * Channels);
        size_t Got = Decoder->read(Buffer.data(), Frames);
        if (Got < Frames && bLoop && Decoder->rewind()) {
            Got += Decoder->read(Buffer.data() + Got * Channels, Frames - Got);
        }
        if (Got > 0) {
            W->QueueAudio(reinterpret_cast<const uint8*>(Buffer.data()), static_cast<int32>(Got * Channels * sizeof(int16_t)));
        }
    };
    Feed(Wave, Decoder->sampleRate() * Channels / 2);  // первые полсекунды — сразу
    Wave->OnSoundWaveProceduralUnderflow.BindLambda(Feed);
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
