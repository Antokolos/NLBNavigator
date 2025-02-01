#include "nlb/vcs/Author.h"

Author::Author(const std::string& name, const std::string& email)
    : m_name(name), m_email(email) {}

std::string Author::getName() const {
    return m_name;
}

void Author::setName(const std::string& name) {
    m_name = name;
}

std::string Author::getEmail() const {
    return m_email;
}

void Author::setEmail(const std::string& email) {
    m_email = email;
}