#include "nlb/util/Json.h"

#include <cctype>
#include <cstdlib>

class JsonParser {
public:
    explicit JsonParser(const std::string& s) : m_s(s) {}

    std::shared_ptr<Json> value() {
        ws();
        if (m_p >= m_s.size()) return fail("неожиданный конец");
        const char c = m_s[m_p];
        auto v = std::make_shared<Json>();
        if (c == '{') {
            ++m_p;
            v->m_type = Json::Type::Object;
            ws();
            if (peek('}')) { ++m_p; return v; }
            while (true) {
                ws();
                std::string key;
                if (!string(key)) return nullptr;
                ws();
                if (!expect(':')) return nullptr;
                auto item = value();
                if (!item) return nullptr;
                v->m_object[key] = item;
                ws();
                if (peek(',')) { ++m_p; continue; }
                if (!expect('}')) return nullptr;
                return v;
            }
        }
        if (c == '[') {
            ++m_p;
            v->m_type = Json::Type::Array;
            ws();
            if (peek(']')) { ++m_p; return v; }
            while (true) {
                auto item = value();
                if (!item) return nullptr;
                v->m_array.push_back(item);
                ws();
                if (peek(',')) { ++m_p; continue; }
                if (!expect(']')) return nullptr;
                return v;
            }
        }
        if (c == '"') {
            v->m_type = Json::Type::String;
            return string(v->m_string) ? v : nullptr;
        }
        if (m_s.compare(m_p, 4, "true") == 0) { m_p += 4; v->m_type = Json::Type::Bool; v->m_bool = true; return v; }
        if (m_s.compare(m_p, 5, "false") == 0) { m_p += 5; v->m_type = Json::Type::Bool; return v; }
        if (m_s.compare(m_p, 4, "null") == 0) { m_p += 4; return v; }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            const char* start = m_s.c_str() + m_p;
            char* end = nullptr;
            v->m_type = Json::Type::Number;
            v->m_number = std::strtod(start, &end);
            m_p += static_cast<size_t>(end - start);
            return v;
        }
        return fail(std::string("неожиданный символ «") + c + "»");
    }

    bool atEnd() { ws(); return m_p >= m_s.size(); }
    std::string error;

private:
    void ws() {
        while (m_p < m_s.size() && std::isspace(static_cast<unsigned char>(m_s[m_p]))) ++m_p;
    }
    bool peek(char c) const { return m_p < m_s.size() && m_s[m_p] == c; }
    bool expect(char c) {
        if (peek(c)) { ++m_p; return true; }
        fail(std::string("ожидалось «") + c + "»");
        return false;
    }
    std::shared_ptr<Json> fail(const std::string& what) {
        if (error.empty()) error = what + " (позиция " + std::to_string(m_p) + ")";
        return nullptr;
    }
    static void utf8(std::string& out, unsigned cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    bool hex4(unsigned& v) {
        if (m_p + 4 > m_s.size()) return false;
        v = static_cast<unsigned>(std::strtoul(m_s.substr(m_p, 4).c_str(), nullptr, 16));
        m_p += 4;
        return true;
    }
    bool string(std::string& out) {
        if (!expect('"')) return false;
        while (m_p < m_s.size()) {
            const char c = m_s[m_p++];
            if (c == '"') return true;
            if (c != '\\') { out += c; continue; }
            if (m_p >= m_s.size()) break;
            const char e = m_s[m_p++];
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    unsigned cp = 0;
                    if (!hex4(cp)) { fail("неверное \\u"); return false; }
                    if (cp >= 0xD800 && cp < 0xDC00 && m_s.compare(m_p, 2, "\\u") == 0) {  // суррогатная пара
                        m_p += 2;
                        unsigned lo = 0;
                        if (!hex4(lo)) { fail("неверное \\u"); return false; }
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    }
                    utf8(out, cp);
                    break;
                }
                default: out += e; break;  // \" \\ \/
            }
        }
        fail("незакрытая строка");
        return false;
    }

    const std::string& m_s;
    size_t m_p = 0;
};

std::shared_ptr<Json> Json::parse(const std::string& text, std::string& error) {
    JsonParser p(text);
    auto v = p.value();
    if (v && !p.atEnd()) { error = "лишний текст после JSON"; return nullptr; }
    if (!v) error = p.error;
    return v;
}

const Json* Json::get(const std::string& key) const {
    auto it = m_object.find(key);
    return it == m_object.end() ? nullptr : it->second.get();
}
