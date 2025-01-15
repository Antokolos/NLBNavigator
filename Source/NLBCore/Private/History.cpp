#include "History.h"
#include "NLBExceptions.h"

void History::clear() {
    m_decisionPoints.clear();
    m_decisionPointToBeMade.reset();
}

const std::list<std::shared_ptr<DecisionPoint>>& History::getDecisionPoints() const {
    return m_decisionPoints;
}

void History::setDecisionPoints(const std::list<std::shared_ptr<DecisionPoint>>& decisionPoints) {
    m_decisionPoints = decisionPoints;
}

void History::setDecisionPointToBeMadeText(const std::string& decisionPointToBeMadeText) {
    m_decisionPointToBeMadeText = decisionPointToBeMadeText;
}

void History::makeDecision() {
    if (m_decisionPointToBeMade) {
        m_decisionPointToBeMade->setText(m_decisionPointToBeMadeText);
        for (const auto& decisionPoint : m_decisionPoints) {
            if (*decisionPoint == *m_decisionPointToBeMade) {
                m_decisionPointToBeMade->incVisitCount();
            }
        }
        m_decisionPoints.push_back(m_decisionPointToBeMade);
        m_decisionPointToBeMade.reset();
    }
}

int History::predictDecisionCount(const std::shared_ptr<DecisionPoint>& decisionPoint) const {
    int count = 1;
    for (const auto& currentDecisionPoint : m_decisionPoints) {
        if (*currentDecisionPoint == *decisionPoint) {
            count++;
        }
    }
    return count;
}

std::shared_ptr<DecisionPoint> History::getDecisionPointToBeMade() const {
    return m_decisionPointToBeMade;
}

void History::suggestDecisionPointToBeMade(
    const std::shared_ptr<DecisionPoint>& decisionPointToBeMade,
    bool rollback,
    int visitCount
) {
    if (decisionPointToBeMade && !m_decisionPoints.empty()) {
        bool suggestedOK = false;
        if (!rollback) {
            const auto& tailDecisionPoint = m_decisionPoints.back();
            for (const auto& decisionPoint : tailDecisionPoint->getPossibleNextDecisionPoints()) {
                if (*decisionPoint == *decisionPointToBeMade) {
                    suggestedOK = true;
                    break;
                }
            }
        }
        
        if (!suggestedOK || rollback) {
            // Suggested decision is not possible for current decision list
            // Try to search through the list of existing decisions to locate the last
            // occurrence of suggested decision in the past, if any
            // OR locate the decision with specified visit count in the past
            auto it = m_decisionPoints.rbegin();
            int i = m_decisionPoints.size() - 1;
            
            for (; it != m_decisionPoints.rend(); ++it, --i) {
                if (**it == *decisionPointToBeMade && 
                    (visitCount == DO_NOT_USE_VISIT_COUNT || visitCount == (*it)->getVisitCount())) {
                    m_decisionPointToBeMade = decisionPointToBeMade;
                    // Possible next decisions will be recreated again
                    m_decisionPointToBeMade->clearPossibleNextDecisionPoints();
                    // Visit count will be incremented in the makeDecision() call
                    m_decisionPointToBeMade->setVisitCount(1);
                    // Throw away unmatched decision tail
                    m_decisionPoints.erase(std::next(it).base(), m_decisionPoints.end());
                    return;
                }
            }
            throw DecisionException("Suggested decision cannot be found, please specify correct decision or restart");
        }
    }
    m_decisionPointToBeMade = decisionPointToBeMade;
}

bool History::containsLink(const Link& link) const {
    for (const auto& decisionPoint : m_decisionPoints) {
        if (link.getId() == decisionPoint->getLinkId()) {
            return true;
        }
    }
    return false;
}