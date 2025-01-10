#ifndef NLB_EXCEPTIONS_H
#define NLB_EXCEPTIONS_H

#include <stdexcept>

class NLBConsistencyException : public std::runtime_error {
public:
    explicit NLBConsistencyException(const std::string& message)
        : std::runtime_error(message) {}
};

class NLBExportException : public std::runtime_error {
public:
    explicit NLBExportException(const std::string& message)
        : std::runtime_error(message) {}
};

class NLBIOException : public std::runtime_error {
public:
    explicit NLBIOException(const std::string& message)
        : std::runtime_error(message) {}
};

class NLBVCSException : public std::runtime_error {
public:
    explicit NLBVCSException(const std::string& message)
        : std::runtime_error(message) {}
};

#endif // NLB_EXCEPTIONS_H
