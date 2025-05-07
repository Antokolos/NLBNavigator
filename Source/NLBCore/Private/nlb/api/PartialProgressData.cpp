#include "nlb/api/PartialProgressData.h"
#include "nlb/api/ProgressData.h" // Actual header for ProgressData class

PartialProgressData::PartialProgressData(
    std::shared_ptr<ProgressData> realProgressData,
    int startingProgress,
    int maximumAllowedProgress,
    int itemsCountPerIncrement
) : 
    m_realProgressData(realProgressData),
    m_startingProgress(startingProgress),
    m_currentProgress(startingProgress),
    m_maximumAllowedProgress(maximumAllowedProgress),
    m_itemsCountPerIncrement(itemsCountPerIncrement),
    m_currentItemsCount(0)
{
}

void PartialProgressData::setRealProgressValue() {
    m_currentItemsCount++;
    if (m_currentItemsCount % m_itemsCountPerIncrement == 0) {
        m_currentProgress++;
        if (m_currentProgress <= m_maximumAllowedProgress) {
            m_realProgressData->setProgressValue(m_currentProgress);
        }
    }
}

int PartialProgressData::getStartingProgress() const {
    return m_startingProgress;
}

int PartialProgressData::getMaximumAllowedProgress() const {
    return m_maximumAllowedProgress;
}