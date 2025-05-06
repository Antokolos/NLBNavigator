#pragma once

#include "ProgressData.h"

/*!
 * This class represents dummy progress data implementation that does nothing.
 */
class DummyProgressData : public ProgressData {
public:
    virtual ~DummyProgressData() = default;
    void setProgressValue(int progress) override {};
    void setNoteText(const std::string& text) override {};
};