#pragma once
#include <memory>

class NonLinearBookImpl;

class Clipboard {
private:
    std::shared_ptr<NonLinearBookImpl> m_nonLinearBook;
    
    // Private constructor for singleton
    Clipboard() = default;
    
    // Delete copy constructor and assignment operator
    Clipboard(const Clipboard&) = delete;
    Clipboard& operator=(const Clipboard&) = delete;

public:
    static Clipboard& singleton();
    
    std::shared_ptr<NonLinearBookImpl> getNonLinearBook() const;
    void setNonLinearBook(std::shared_ptr<NonLinearBookImpl> nonLinearBook);
    void reset();
};