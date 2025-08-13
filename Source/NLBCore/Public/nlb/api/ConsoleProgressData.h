#pragma once

#include "ProgressData.h"

/*!
 * This class represents dummy progress data implementation that does nothing.
 */
class ConsoleProgressData : public ProgressData {
public:
    virtual ~ConsoleProgressData() = default;
    void setProgressValue(int progress) override;
    void setNoteText(const std::string& text) override;
};
