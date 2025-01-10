#include "Link.h"

Link::Link(const std::string& id, const std::string& description)
    : id(id), description(description) {}

const std::string& Link::getId() const {
    return id;
}

const std::string& Link::getDescription() const {
    return description;
}

const std::string& Link::getTargetPageId) const {
    return targetPageId;
}

void Link::setId(const std::string& newId) {
    id = newId;
}

void Link::setDescription(const std::string& newDescription) {
    description = newDescription;
}

void Link::setTargetPageId(const std::string& newTargetPageId) {
    targetPageId = newTargetPageId;
}
