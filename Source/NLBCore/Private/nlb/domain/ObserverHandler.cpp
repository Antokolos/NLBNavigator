#include "nlb/domain/ObserverHandler.h"
#include "nlb/api/NLBObserver.h"
#include "nlb/util/UUID.h"

std::string ObserverHandler::addObserver(NLBObserver* observer) {
    const std::string observerId = UUID::randomUUID();
    m_observers[observerId] = observer;
    return observerId;
}

void ObserverHandler::removeObserver(const std::string& observerId) {
    m_observers.erase(observerId);
}

void ObserverHandler::notifyObservers() {
    for (const auto& pair : m_observers) {
        pair.second->updateView();
    }
}