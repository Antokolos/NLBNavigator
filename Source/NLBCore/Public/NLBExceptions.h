#ifndef NLB_EXCEPTIONS_H
#define NLB_EXCEPTIONS_H

#include <stdexcept>

class NLBConsistencyException : public std::runtime_error {
public:
    explicit NLBConsistencyException(const std::string& message)
        : std::runtime_error(message) {}
    explicit NLBConsistencyException(const std::string& message1, const std::string& message2)
        : std::runtime_error(message1) {}
};

class NLBExportException : public std::runtime_error {
public:
    explicit NLBExportException(const std::string& message)
        : std::runtime_error(message) {}
    explicit NLBExportException(const std::string& message1, const std::string& message2)
        : std::runtime_error(message1) {}
};

class NLBIOException : public std::runtime_error {
public:
    explicit NLBIOException(const std::string& message)
        : std::runtime_error(message) {}
    explicit NLBIOException(const std::string& message1, const std::string& message2)
        : std::runtime_error(message1) {}
};

class NLBVCSException : public std::runtime_error {
public:
    explicit NLBVCSException(const std::string& message)
        : std::runtime_error(message) {}
    explicit NLBVCSException(const std::string& message1, const std::string& message2)
        : std::runtime_error(message1) {}
};

class NLBFileManipulationException : public std::runtime_error {
public:
    explicit NLBFileManipulationException(const std::string& message)
        : std::runtime_error(message) {}
    explicit NLBFileManipulationException(const std::string& message1, const std::string& message2)
        : std::runtime_error(message1) {}
};

#endif // NLB_EXCEPTIONS_H
