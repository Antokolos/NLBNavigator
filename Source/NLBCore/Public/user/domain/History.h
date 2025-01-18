#pragma once

#include <list>
#include <string>
#include <memory>

class DecisionPoint;
class Link;

/*!
 * \class History
 * \brief Class for managing decision points history
 */
class History {
public:
    /*!
     * \brief Constant indicating that visit count should not be used
     */
    static const int DO_NOT_USE_VISIT_COUNT = -1;

    /*!
     * \brief Default constructor
     */
    History() = default;

    /*!
     * \brief Clears all decision points and resets the state
     */
    void clear();

    /*!
     * \brief Gets the list of decision points
     * \return List of decision points
     */
    const std::list<std::shared_ptr<DecisionPoint>>& getDecisionPoints() const;

    /*!
     * \brief Sets the list of decision points
     * \param decisionPoints New list of decision points
     */
    void setDecisionPoints(const std::list<std::shared_ptr<DecisionPoint>>& decisionPoints);

    /*!
     * \brief Sets the text for the decision point to be made
     * \param decisionPointToBeMadeText Text to be set
     */
    void setDecisionPointToBeMadeText(const std::string& decisionPointToBeMadeText);

    /*!
     * \brief Makes the pending decision
     */
    void makeDecision();

    /*!
     * \brief Predicts the count of decisions for a given decision point
     * \param decisionPoint Decision point to predict count for
     * \return Predicted count
     */
    int predictDecisionCount(const std::shared_ptr<DecisionPoint>& decisionPoint) const;

    /*!
     * \brief Gets the decision point that is pending
     * \return Pointer to the pending decision point
     */
    std::shared_ptr<DecisionPoint> getDecisionPointToBeMade() const;

    /*!
     * \brief Suggests a decision point to be made
     * \param decisionPointToBeMade The decision point to be made
     * \param rollback Whether to rollback to a previous state
     * \param visitCount The visit count to rollback to (use DO_NOT_USE_VISIT_COUNT if not rolling back)
     * \throw DecisionException if the suggested decision cannot be found
     */
    void suggestDecisionPointToBeMade(
        const std::shared_ptr<DecisionPoint>& decisionPointToBeMade,
        bool rollback,
        int visitCount
    );

    /*!
     * \brief Checks if the history contains a specific link
     * \param link Link to check for
     * \return true if the link exists in history, false otherwise
     */
    bool containsLink(const Link& link) const;

private:
    std::list<std::shared_ptr<DecisionPoint>> m_decisionPoints;
    std::shared_ptr<DecisionPoint> m_decisionPointToBeMade;
    std::string m_decisionPointToBeMadeText;
};