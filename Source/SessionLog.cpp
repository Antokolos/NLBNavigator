#include "SessionLog.h"

#include "nlb/util/FileUtils.h"

#include <iostream>

/// streambuf, который пишет в исходный поток и в журнал
class SessionLog::TeeBuf : public std::streambuf {
public:
    TeeBuf(std::streambuf* original, SessionLog& log) : m_original(original), m_log(log) {}

protected:
    int overflow(int c) override {
        if (c == traits_type::eof()) {
            return traits_type::not_eof(c);
        }
        m_log.writeChar(static_cast<char>(c));
        return m_original->sputc(static_cast<char>(c));
    }
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        for (std::streamsize i = 0; i < n; ++i) {
            m_log.writeChar(s[i]);
        }
        return m_original->sputn(s, n);
    }
    int sync() override {
        m_log.m_file.flush();
        return m_original->pubsync();
    }

private:
    std::streambuf* m_original;
    SessionLog& m_log;
};

SessionLog::SessionLog() = default;

bool SessionLog::open(const std::string& utf8Path) {
    m_file.open(FileUtils::nativePath(utf8Path), std::ios::binary | std::ios::trunc);
    if (!m_file.is_open()) {
        return false;
    }
    m_coutOrig = std::cout.rdbuf();
    m_cerrOrig = std::cerr.rdbuf();
    m_coutTee = std::make_unique<TeeBuf>(m_coutOrig, *this);
    m_cerrTee = std::make_unique<TeeBuf>(m_cerrOrig, *this);
    std::cout.rdbuf(m_coutTee.get());
    std::cerr.rdbuf(m_cerrTee.get());
    return true;
}

SessionLog::~SessionLog() {
    if (m_coutOrig) std::cout.rdbuf(m_coutOrig);
    if (m_cerrOrig) std::cerr.rdbuf(m_cerrOrig);
    if (m_file.is_open()) {
        if (!m_line.empty()) flushLine(true);
        m_file.close();
    }
}

void SessionLog::writeChar(char c) {
    if (c == '\r') {
        m_line.clear();  // как в терминале: строка будет переписана
    } else if (c == '\n') {
        flushLine(true);
    } else {
        m_line += c;
    }
}

void SessionLog::flushLine(bool newline) {
    m_file << m_line;
    if (newline) m_file << '\n';
    m_line.clear();
    m_file.flush();
}

void SessionLog::recordInput(const std::string& line) {
    if (!m_file.is_open()) return;
    m_line += line;
    flushLine(true);
}
