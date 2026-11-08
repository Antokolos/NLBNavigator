#include "NLBUnrealFileSystem.h"

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#include <mutex>
#include <string>
#include <vector>

namespace {
FString U(const std::string& S) { return FString(UTF8_TO_TCHAR(S.c_str())); }
std::string S(const FString& U) { return std::string(TCHAR_TO_UTF8(*U)); }
IPlatformFile& Files() { return FPlatformFileManager::Get().GetPlatformFile(); }

/// Чтение с произвольного места; звук может читать из другого потока — под замком
class FUnrealReader : public IFileSystem::Reader {
public:
    explicit FUnrealReader(IFileHandle* InHandle) : Handle(InHandle), Size(InHandle ? InHandle->Size() : 0) {}
    uint64_t size() override { return Size > 0 ? static_cast<uint64_t>(Size) : 0; }
    bool read(uint64_t Offset, size_t Count, char* Dst) override {
        if (Count == 0) return true;
        std::lock_guard<std::mutex> Lock(Mutex);
        return Handle && Handle->Seek(static_cast<int64>(Offset))
            && Handle->Read(reinterpret_cast<uint8*>(Dst), static_cast<int64>(Count));
    }
private:
    TUniquePtr<IFileHandle> Handle;
    int64 Size;
    std::mutex Mutex;
};

class FUnrealFileSystem : public IFileSystem {
public:
    bool info(const std::string& Path, Info& Out) override {
        const FFileStatData Stat = Files().GetStatData(*U(Path));
        if (!Stat.bIsValid) return false;
        Out.isDirectory = Stat.bIsDirectory;
        Out.size = Stat.FileSize > 0 ? static_cast<uint64_t>(Stat.FileSize) : 0;
        Out.mtime = Stat.ModificationTime.GetTicks();
        return true;
    }
    std::vector<std::string> list(const std::string& Dir) override {
        std::vector<std::string> Names;
        Files().IterateDirectory(*U(Dir), [&Names](const TCHAR* Child, bool) {
            Names.push_back(S(FPaths::GetCleanFilename(Child)));
            return true;
        });
        return Names;
    }
    bool readAll(const std::string& Path, std::string& Out) override {
        TUniquePtr<IFileHandle> H(Files().OpenRead(*U(Path)));
        if (!H) return false;
        const int64 Size = H->Size();
        if (Size < 0) return false;
        Out.resize(static_cast<size_t>(Size));
        return Size == 0 || H->Read(reinterpret_cast<uint8*>(&Out[0]), Size);
    }
    std::unique_ptr<Reader> openRead(const std::string& Path) override {
        IFileHandle* H = Files().OpenRead(*U(Path));
        if (!H) return nullptr;
        return std::make_unique<FUnrealReader>(H);
    }
    bool writeAll(const std::string& Path, const std::string& Data, bool bAppend) override {
        TUniquePtr<IFileHandle> H(Files().OpenWrite(*U(Path), bAppend));
        if (!H) return false;
        return Data.empty() || H->Write(reinterpret_cast<const uint8*>(Data.data()), static_cast<int64>(Data.size()));
    }
    bool makeDirectory(const std::string& Path) override {
        return Files().CreateDirectory(*U(Path)) || Files().DirectoryExists(*U(Path));
    }
    bool removeFile(const std::string& Path) override { return Files().DeleteFile(*U(Path)); }
    bool removeDirectory(const std::string& Path) override { return Files().DeleteDirectory(*U(Path)); }
    bool rename(const std::string& From, const std::string& To) override {
        const FString Dst = U(To);
        if (Files().FileExists(*Dst) && !Files().DeleteFile(*Dst)) return false;  // замена
        return Files().MoveFile(*Dst, *U(From));
    }
};
}

std::shared_ptr<IFileSystem> MakeNLBUnrealFileSystem() {
    return std::make_shared<FUnrealFileSystem>();
}
