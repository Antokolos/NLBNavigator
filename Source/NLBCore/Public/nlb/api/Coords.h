#pragma once

class Coords {
public:
    virtual ~Coords() = default;

    // Pure virtual methods to be implemented by derived classes
    virtual float getLeft() const = 0;
    virtual float getTop() const = 0;
    virtual float getWidth() const = 0;
    virtual float getHeight() const = 0;
};
