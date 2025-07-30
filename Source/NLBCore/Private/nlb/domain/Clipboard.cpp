#include "nlb/domain/Clipboard.h"
#include "nlb/domain/NonLinearBookImpl.h"

Clipboard& Clipboard::singleton() {
    static Clipboard instance;
    return instance;
}

std::shared_ptr<NonLinearBookImpl> Clipboard::getNonLinearBook() const {
    return m_nonLinearBook;
}

void Clipboard::setNonLinearBook(std::shared_ptr<NonLinearBookImpl> nonLinearBook) {
    m_nonLinearBook = nonLinearBook;
}

// void Clipboard::reset() {
//     setNonLinearBook(nullptr);
// }

void Clipboard::reset() {
    m_nonLinearBook.reset();
}
