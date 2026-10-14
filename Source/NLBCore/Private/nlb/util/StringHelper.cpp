#include "nlb/util/StringHelper.h"
#include "nlb/api/Constants.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/TextChunk.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <typeinfo>
#include <cstdarg>
#include <vector>
#include <cctype>

// Initialize static members
const std::regex StringHelper::LINE_PATTERN(R"([^\r\n]*)");
const std::regex StringHelper::VAR_PATTERN(R"(\$([^\s\$]*)\$)");
const std::string StringHelper::DELIMITER = ";";
const std::string StringHelper::EOL_STRING = "\n";
const std::string StringHelper::ACTION_TEXT_DEF = "^";

std::string StringHelper::replaceVariables(const std::string& pageText,
                                         const std::map<std::string, Any>& visitedVars) {
    std::ostringstream result;
    auto textChunks = getTextChunks(pageText);

    for (const auto& textChunk : textChunks) {
        switch (textChunk.getType()) {
            case TextChunk::ChunkType::TEXT:
                result << textChunk.getText();
                break;

            case TextChunk::ChunkType::ACTION_TEXT:
                // Support for action text - implementation depends on requirements
                result << textChunk.getText();
                break;

            case TextChunk::ChunkType::VARIABLE: {
                auto it = visitedVars.find(textChunk.getText());
                if (it != visitedVars.end()) {
                    result << anyToString(it->second);
                } else {
                    result << "UNDEFINED";
                }
                break;
            }

            case TextChunk::ChunkType::NEWLINE:
                result << EOL_STRING;
                break;
        }
    }
    return result.str();
}

std::vector<TextChunk> StringHelper::getTextChunks(const std::string& text) {
    std::vector<TextChunk> result;
    
    // Split text by lines manually to handle different line endings
    std::istringstream stream(text);
    std::string line;
    bool isFirstLine = true;
    
    while (std::getline(stream, line)) {
        if (!isFirstLine) {
            // Add newline chunk between lines
            TextChunk newlineChunk;
            newlineChunk.setText("");
            newlineChunk.setType(TextChunk::ChunkType::NEWLINE);
            result.push_back(newlineChunk);
        }
        isFirstLine = false;
        
        // Remove trailing whitespace from line
        while (!line.empty() && std::isspace(line.back())) {
            line.pop_back();
        }
        
        // Process variables in the line
        std::sregex_iterator varBegin(line.begin(), line.end(), VAR_PATTERN);
        std::sregex_iterator varEnd;
        
        size_t lastPos = 0;
        for (std::sregex_iterator varIt = varBegin; varIt != varEnd; ++varIt) {
            size_t varPos = varIt->position();
            
            // Add text before variable
            if (varPos > lastPos) {
                TextChunk textChunk;
                textChunk.setText(line.substr(lastPos, varPos - lastPos));
                textChunk.setType(TextChunk::ChunkType::TEXT);
                result.push_back(textChunk);
            }
            
            // Add variable chunk
            std::string varName = (*varIt)[1].str();
            TextChunk varChunk;
            varChunk.setText(varName);
            
            // Check if it's action text
            if (varName == ACTION_TEXT_DEF) {
                varChunk.setType(TextChunk::ChunkType::ACTION_TEXT);
            } else {
                varChunk.setType(TextChunk::ChunkType::VARIABLE);
            }
            result.push_back(varChunk);
            
            lastPos = varPos + varIt->length();
        }
        
        // Add remaining text after last variable
        if (lastPos < line.length()) {
            TextChunk textChunk;
            textChunk.setText(line.substr(lastPos));
            textChunk.setType(TextChunk::ChunkType::TEXT);
            result.push_back(textChunk);
        }
    }
    
    return result;
}

bool StringHelper::isEmpty(const std::string& str) {
    return str.empty();
}

bool StringHelper::notEmpty(const std::string& str) {
    return !isEmpty(str);
}

bool StringHelper::isEmpty(const MultiLangString& multiLangString) {
    for (const auto& text : multiLangString.values()) {
        if (!isEmpty(text)) {
            return false;
        }
    }
    return true;
}

bool StringHelper::notEmpty(const MultiLangString& multiLangString) {
    return !isEmpty(multiLangString);
}

std::string StringHelper::formatSequence(const std::vector<std::string>& strings) {
    if (strings.empty()) {
        return "";
    }

    std::ostringstream ss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) {
            ss << DELIMITER;
        }
        ss << strings[i];
    }
    return ss.str();
}

std::vector<std::string> StringHelper::getItems(const std::string& sequenceString) {
    if (sequenceString.empty()) {
        return std::vector<std::string>();
    }

    std::vector<std::string> result;
    size_t start = 0;
    size_t end = sequenceString.find(DELIMITER);

    while (end != std::string::npos) {
        result.push_back(sequenceString.substr(start, end - start));
        start = end + DELIMITER.length();
        end = sequenceString.find(DELIMITER, start);
    }

    result.push_back(sequenceString.substr(start));
    return result;
}

std::string StringHelper::createRepeatedString(size_t length, const std::string& fill) {
    if (length == 0 || fill.empty()) {
        return "";
    }

    std::string result;
    result.reserve(length * fill.length());
    
    // Repeat fill string until we have enough characters
    while (result.length() < length) {
        if (result.length() + fill.length() <= length) {
            result += fill;
        } else {
            // Add partial fill to reach exact length
            result += fill.substr(0, length - result.length());
            break;
        }
    }
    
    return result;
}

std::vector<std::string> StringHelper::tokenize(const std::string& str, const std::string& delimiters) {
    std::vector<std::string> tokens;
    
    if (str.empty() || delimiters.empty()) {
        if (!str.empty()) {
            tokens.push_back(str);
        }
        return tokens;
    }
    
    size_t start = 0;
    size_t end = str.find_first_of(delimiters);
    
    while (end != std::string::npos) {
        if (end > start) {
            // Add non-empty token
            tokens.push_back(str.substr(start, end - start));
        }
        
        start = end + 1;
        end = str.find_first_of(delimiters, start);
    }
    
    // Add the last token if there's remaining text
    if (start < str.length()) {
        tokens.push_back(str.substr(start));
    }
    
    return tokens;
}

std::string StringHelper::anyToString(const Any& value) {
    try {
        if (value.empty()) {
            return "EMPTY";
        }

        // Проверяем различные типы
        if (value.is_type<std::string>()) {
            return value.cast<std::string>();
        } else if (value.is_type<int>()) {
            return std::to_string(value.cast<int>());
        } else if (value.is_type<double>()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << value.cast<double>();
            return oss.str();
        } else if (value.is_type<float>()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << value.cast<float>();
            return oss.str();
        } else if (value.is_type<bool>()) {
            return value.cast<bool>() ? "true" : "false";
        } else if (value.is_type<long>()) {
            return std::to_string(value.cast<long>());
        } else if (value.is_type<long long>()) {
            return std::to_string(value.cast<long long>());
        } else if (value.is_type<short>()) {
            return std::to_string(value.cast<short>());
        } else if (value.is_type<unsigned int>()) {
            return std::to_string(value.cast<unsigned int>());
        } else if (value.is_type<unsigned long>()) {
            return std::to_string(value.cast<unsigned long>());
        } else if (value.is_type<unsigned long long>()) {
            return std::to_string(value.cast<unsigned long long>());
        } else if (value.is_type<const char*>()) {
            const char* str = value.cast<const char*>();
            return str ? std::string(str) : "NULL";
        } else if (value.is_type<char*>()) {
            char* str = value.cast<char*>();
            return str ? std::string(str) : "NULL";
        }

        // Для неизвестных типов возвращаем имя типа
        return std::string("UNKNOWN_TYPE:") + value.type().name();

    } catch (const std::exception& e) {
        return std::string("CONVERSION_ERROR:") + e.what();
    }
}

std::string StringHelper::anyToString(const Any& value, int precision) {
    try {
        if (value.empty()) {
            return "EMPTY";
        }

        if (value.is_type<double>()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(precision) << value.cast<double>();
            return oss.str();
        } else if (value.is_type<float>()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(precision) << value.cast<float>();
            return oss.str();
        }

        // Для других типов используем стандартный метод
        return anyToString(value);

    } catch (const std::exception& e) {
        return std::string("CONVERSION_ERROR:") + e.what();
    }
}

std::string StringHelper::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
