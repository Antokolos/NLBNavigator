#pragma once

#include <string>

class MediaExportParameters;

class MediaFile {
public:
    enum class Type {
        Image,
        Sound
    };

    virtual ~MediaFile() = default;

    // Core interface methods
    virtual std::string getFileName() const = 0;
    virtual std::string getRedirect() const = 0;
    virtual std::string getConstrId() const = 0;
    virtual bool isFlagged() const = 0;
    virtual MediaExportParameters getMediaExportParameters() const = 0;

    // Comparable implementation
    virtual bool operator<(const MediaFile& other) const = 0;
};