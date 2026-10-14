#pragma once

#include <string>
#include <vector>
#include <memory>

#include "nlb/api/Constants.h"
#include "nlb/util/StringHelper.h"

/*!
 * \brief Class representing a decision point in a non-linear book
 */
class DecisionPoint {
public:
    // Constructors
    DecisionPoint();
    DecisionPoint(const std::string& bookId, const std::string& fromPageId, const std::string& linkId);
    DecisionPoint(const std::string& bookId, const std::string& toPageId);

    // Getters and setters
    std::string getBookId() const { return m_bookId; }
    void setBookId(const std::string& bookId) { m_bookId = bookId; }

    std::string getFromPageId() const { return m_fromPageId; }
    void setFromPageId(const std::string& fromPageId) { m_fromPageId = fromPageId; }

    std::string getToPageId() const { return m_toPageId; }
    void setToPageId(const std::string& toPageId) { m_toPageId = toPageId; }

    std::string getLinkId() const { return m_linkId; }
    void setLinkId(const std::string& linkId) { m_linkId = linkId; }

    std::string getText() const { return m_text; }
    void setText(const std::string& text) { m_text = text; }

    int getVisitCount() const { return m_visitCount; }
    void setVisitCount(int visitCount) { m_visitCount = visitCount; }
    void incVisitCount() { ++m_visitCount; }

    const std::vector<DecisionPoint*>& getPossibleNextDecisionPoints() const {
        return m_possibleNextDecisionPoints;
    }

    void addPossibleNextDecisionPoint(const DecisionPoint* decisionPoint);
    void clearPossibleNextDecisionPoints() { m_possibleNextDecisionPoints.clear(); }

    bool isLinkInfo() const { return !StringHelper::isEmpty(m_linkId); }

    // Comparison operators
    bool operator==(const DecisionPoint& other) const;
    std::size_t hash() const;

private:
    std::string m_bookId;
    std::string m_fromPageId;
    std::string m_toPageId;
    std::string m_linkId;
    std::string m_text;
    int m_visitCount;
    std::vector<DecisionPoint*> m_possibleNextDecisionPoints;
};

// Custom hash function for std::unordered_map/set support
namespace std {
    template<>
    struct hash<DecisionPoint> {
        std::size_t operator()(const DecisionPoint& dp) const {
            return dp.hash();
        }
    };
}
