#include "nlb/vcs/DummyVCSAdapter.h"

void DummyVCSAdapter::initRepo(const std::string& path) {
	// Empty implementation - does nothing
}

void DummyVCSAdapter::openRepo(const std::string& path) {
	// Empty implementation - does nothing
}

void DummyVCSAdapter::closeAdapter() {
	// Empty implementation - does nothing
}

bool DummyVCSAdapter::getDirAddFlag() {
	return false;
}

bool DummyVCSAdapter::getAddModifiedFilesFlag() {
	return false;
}

VCSAdapter::Status DummyVCSAdapter::getStatus(const std::string& path) {
	return Status::Unknown;
}

void DummyVCSAdapter::add(const std::string& path) {
	// Empty implementation - does nothing
}

bool DummyVCSAdapter::remove(const std::string& path) {
	return false;
}

void DummyVCSAdapter::reset(const std::string& path) {
	// Empty implementation - does nothing
}

void DummyVCSAdapter::commit(const std::string& message) {
	// Empty implementation - does nothing
}

void DummyVCSAdapter::pull(
	const std::string& userName, 
	const std::string& password, 
	std::shared_ptr<ProgressData> progressData
) {
	// Empty implementation - does nothing
}

void DummyVCSAdapter::push(
	const std::string& userName, 
	const std::string& password, 
	std::shared_ptr<ProgressData> progressData
) {
	// Empty implementation - does nothing
}