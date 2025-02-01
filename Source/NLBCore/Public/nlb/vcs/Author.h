#pragma once

#include <string>

class Author {
private:
    std::string m_name;
    std::string m_email;

public:
    Author(const std::string& name, const std::string& email);

    std::string getName() const;
    void setName(const std::string& name);

    std::string getEmail() const;
    void setEmail(const std::string& email);
};
