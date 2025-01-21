#pragma once

#include <string>

class NLBObserver;

/*!
 * @brief Interface for observable objects in the NLB system
 */
class NLBObservable {
public:
    virtual ~NLBObservable() = default;

    /*!
     * @brief Adds an observer to the observable object
     * @param observer Pointer to the observer to be added
     * @return Observer identifier string
     */
    virtual std::string addObserver(NLBObserver* observer) = 0;

    /*!
     * @brief Removes an observer from the observable object
     * @param observerId Identifier string of the observer to be removed
     */
    virtual void removeObserver(const std::string& observerId) = 0;

    /*!
     * @brief Notifies all registered observers about changes
     */
    virtual void notifyObservers() = 0;
};