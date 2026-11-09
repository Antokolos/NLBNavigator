#include "NLBPlatformHooks.h"

#include "Misc/Paths.h"
#include "NLBUnrealFileSystem.h"
#include "nlb/util/FileUtils.h"

namespace
{
	FNLBPlatformHooks::FWrapper& Wrapper()
	{
		static FNLBPlatformHooks::FWrapper W;
		return W;
	}
}

TFunction<FString()> FNLBPlatformHooks::SaveUserId;

void FNLBPlatformHooks::SetFileSystemWrapper(FWrapper Wrap)
{
	Wrapper() = MoveTemp(Wrap);
	InstallFileSystem();
}

void FNLBPlatformHooks::InstallFileSystem()
{
	std::shared_ptr<IFileSystem> Fs = MakeNLBUnrealFileSystem();
	if (Wrapper())
	{
		if (std::shared_ptr<IFileSystem> Wrapped = Wrapper()(Fs)) Fs = Wrapped;
	}
	FileUtils::setFileSystem(Fs);
}

FString FNLBPlatformHooks::SavesRoot()
{
	return FPaths::ProjectSavedDir() / TEXT("NLBNavigator");
}

bool FNLBPlatformHooks::IsSavePath(const std::string& Utf8Path)
{
	FString Path(UTF8_TO_TCHAR(Utf8Path.c_str()));
	FPaths::NormalizeFilename(Path);
	for (FString Root : {SavesRoot(), FPaths::ConvertRelativePathToFull(SavesRoot())})
	{
		FPaths::NormalizeDirectoryName(Root);
		if (Path == Root || Path.StartsWith(Root + TEXT("/"))) return true;
	}
	return false;
}

FString FNLBPlatformHooks::UserSaveDir()
{
	const FString User = SaveUserId ? SaveUserId() : FString();
	return User.IsEmpty() ? SavesRoot() : SavesRoot() / User;
}
