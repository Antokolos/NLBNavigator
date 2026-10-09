#pragma once

#include <fstream>
#include <memory>
#include <streambuf>
#include <string>

/**
 * Журнал сессии: всё, что выводится в std::cout и std::cerr, плюс ввод игрока
 * дублируется в файл. Перевод каретки '\r' обрабатывается как в терминале (строка
 * переписывается), поэтому от индикатора загрузки в файле остаётся только итоговая строка.
 */
class SessionLog {
public:
    /// Путь — в UTF-8. Возвращает false, если файл не удалось открыть.
    bool open(const std::string& utf8Path);
    SessionLog();
    ~SessionLog();
    SessionLog(const SessionLog&) = delete;
    SessionLog& operator=(const SessionLog&) = delete;

    /// Введённая игроком строка: попадает в файл после приглашения ("Ваш выбор: 1")
    void recordInput(const std::string& line);
    bool isOpen() const { return m_file.is_open(); }

private:
    class TeeBuf;
    void writeChar(char c);
    void flushLine(bool newline);

    std::ofstream m_file;
    std::string m_line;
    std::unique_ptr<TeeBuf> m_coutTee, m_cerrTee;
    std::streambuf* m_coutOrig = nullptr;
    std::streambuf* m_cerrOrig = nullptr;
};
