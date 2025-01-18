#pragma once
#include <string>

class ProgressData {
public:
    virtual ~ProgressData() = default;
    virtual void setProgressValue(int progress) = 0;
    virtual void setNoteText(const std::string& text) = 0;
};