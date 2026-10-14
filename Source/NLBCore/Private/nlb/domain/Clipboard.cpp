#include "nlb/domain/Clipboard.h"
#include "nlb/domain/NonLinearBookImpl.h"

Clipboard& Clipboard::singleton() {
    static Clipboard instance;
    return instance;
}

NonLinearBookImpl* Clipboard::getNonLinearBook() const {
    return m_nonLinearBook;
}

void Clipboard::setNonLinearBook(NonLinearBookImpl* nonLinearBook) {
    m_nonLinearBook = nonLinearBook;
}

void Clipboard::reset() {
    setNonLinearBook(nullptr);
}

//void Clipboard::reset() {
//    m_nonLinearBook.reset();
//}
