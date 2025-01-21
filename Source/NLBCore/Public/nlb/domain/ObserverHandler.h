#pragma once

#include <string>
#include <map>

class NLBObserver;

class ObserverHandler {
private:
    std::map<std::string, NLBObserver*> m_observers;

public:
    std::string addObserver(NLBObserver* observer);

    void removeObserver(const std::string& observerId);

    void notifyObservers();
};