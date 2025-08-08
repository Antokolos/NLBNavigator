#pragma once
#include <memory>

class NonLinearBookImpl;

class Clipboard {
private:
    NonLinearBookImpl* m_nonLinearBook;
    
    // Private constructor for singleton
    Clipboard() = default;
    
    // Delete copy constructor and assignment operator
    Clipboard(const Clipboard&) = delete;
    Clipboard& operator=(const Clipboard&) = delete;

public:
    static Clipboard& singleton();
    
    NonLinearBookImpl* getNonLinearBook() const;
    void setNonLinearBook(NonLinearBookImpl* nonLinearBook);
    void reset();
};