#pragma once

#include <string>
#include <map>
#include <memory>

class NLBObserver;

class ObserverHandler {
private:
    std::map<std::string, std::shared_ptr<NLBObserver>> m_observers;

public:
    std::string addObserver(std::shared_ptr<NLBObserver> observer);

    void removeObserver(const std::string& observerId);

    void notifyObservers();
};