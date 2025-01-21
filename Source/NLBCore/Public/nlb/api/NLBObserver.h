#pragma once

class NLBObserver {
public:
    virtual void updateView() = 0;
    virtual ~NLBObserver() = default;
};