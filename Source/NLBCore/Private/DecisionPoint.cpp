#include "DecisionPoint.h"

DecisionPoint::DecisionPoint()
    : m_visitCount(1)
{
}

DecisionPoint::DecisionPoint(const std::string& bookId, const std::string& fromPageId, const std::string& linkId)
    : DecisionPoint()
{
    m_bookId = bookId;
    m_fromPageId = fromPageId;
    m_toPageId = nlb::Constants::EMPTY_STRING;
    m_linkId = linkId;
}

DecisionPoint::DecisionPoint(const std::string& bookId, const std::string& toPageId)
    : DecisionPoint()
{
    m_bookId = bookId;
    m_fromPageId = nlb::Constants::EMPTY_STRING;
    m_toPageId = toPageId;
    m_linkId = nlb::Constants::EMPTY_STRING;
}

void DecisionPoint::addPossibleNextDecisionPoint(const std::shared_ptr<DecisionPoint>& decisionPoint) {
    m_possibleNextDecisionPoints.push_back(decisionPoint);
}

bool DecisionPoint::operator==(const DecisionPoint& other) const {
    if (this == &other) return true;
    return m_bookId == other.m_bookId &&
           m_fromPageId == other.m_fromPageId &&
           m_linkId == other.m_linkId &&
           m_toPageId == other.m_toPageId;
}

std::size_t DecisionPoint::hash() const {
    std::size_t result = std::hash<std::string>{}(m_bookId);
    result = 31 * result + std::hash<std::string>{}(m_fromPageId);
    result = 31 * result + std::hash<std::string>{}(m_toPageId);
    result = 31 * result + std::hash<std::string>{}(m_linkId);
    return result;
}