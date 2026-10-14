#pragma once

#include <memory>
#include "nlb/api/DummyProgressData.h"

class ProgressData; // Forward declaration

class PartialProgressData : public DummyProgressData {
private:
    ProgressData* m_realProgressData;
    int m_startingProgress;
    int m_currentProgress;
    
    /**
     * Please note that when current progress is more than maximum progress, specified for progress control
     * (e.g. >= 100), then process will be considered finished, even in case when it is not.
     * This can result in prematurely thread termination, so please set maximum allowed progress to value
     * less than maximum progress, specified for progress control.
     */
    int m_maximumAllowedProgress;
    int m_itemsCountPerIncrement;
    int m_currentItemsCount;

public:
    PartialProgressData(
        ProgressData* realProgressData,
        int startingProgress,
        int maximumAllowedProgress,
        int itemsCountPerIncrement
    );

    void setRealProgressValue();
    
    int getStartingProgress() const;
    int getMaximumAllowedProgress() const;
    
    // Virtual destructor
    virtual ~PartialProgressData() = default;
};