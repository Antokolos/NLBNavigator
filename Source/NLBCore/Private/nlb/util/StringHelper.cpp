#include "nlb/util/StringHelper.h"
#include "nlb/api/Constants.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/TextChunk.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

// Initialize static members
const std::regex StringHelper::LINE_PATTERN(R"(^.*$)");  // In Java code, there was a command (?m) for multi-line mode enabling
const std::regex StringHelper::VAR_PATTERN(R"(\\$([^\\s\\$]*)\\$)");
const std::string StringHelper::DELIMITER = ";";

/*std::string StringHelper::replaceVariables(const std::string& pageText,
                                         const std::map<std::string, std::any>& visitedVars) {
    std::ostringstream result;
    auto textChunks = getTextChunks(pageText);

    for (const auto& textChunk : textChunks) {
        switch (textChunk.getType()) {
            case TextChunk::ChunkType::TEXT:
                result << textChunk.getText();
                break;

            case TextChunk::ChunkType::ACTION_TEXT:
                // TODO: support
                break;

            case TextChunk::ChunkType::VARIABLE: {
                auto it = visitedVars.find(textChunk.getText());
                if (it != visitedVars.end()) {
                    if (it->second.type() == typeid(double) ||
                        it->second.type() == typeid(int)) {
                        std::ostringstream numStr;
                        numStr << std::fixed << std::setprecision(3);
                        if (it->second.type() == typeid(double)) {
                            numStr << std::any_cast<double>(it->second);
                        } else {
                            numStr << std::any_cast<int>(it->second);
                        }
                        result << numStr.str();
                    } else {
                        result << std::any_cast<std::string>(it->second);
                    }
                } else {
                    result << "UNDEFINED";
                }
                break;
            }

            case TextChunk::ChunkType::NEWLINE:
                result << Constants::EOL_STRING;
                break;
        }
    }
    return result.str();
}*/

std::vector<TextChunk> StringHelper::getTextChunks(const std::string &text)
{
    std::vector<TextChunk> result;
    std::sregex_iterator lineIt(text.begin(), text.end(), LINE_PATTERN);
    std::sregex_iterator end;

    bool notFirst = false;
    for (; lineIt != end; ++lineIt)
    {
        if (notFirst)
        {
            TextChunk newlineChunk;
            newlineChunk.setText(nlb::Constants::EMPTY_STRING);
            newlineChunk.setType(TextChunk::ChunkType::NEWLINE);
            result.push_back(newlineChunk);
        }
        else
        {
            notFirst = true;
        }

        std::string line = lineIt->str();
        line = std::string(line.begin(),
                           std::find_if(line.rbegin(), line.rend(),
                                        [](char ch)
                                        { return !std::isspace(ch); })
                               .base());

        size_t start = 0;
        std::sregex_iterator varIt(line.begin(), line.end(), VAR_PATTERN);

        for (; varIt != end; ++varIt)
        {
            if (size_t(varIt->position()) > start)
            {
                TextChunk textChunk;
                textChunk.setText(line.substr(start, varIt->position() - start));
                textChunk.setType(TextChunk::ChunkType::TEXT);
                result.push_back(textChunk);
            }

            std::string variable = (*varIt)[1];
            TextChunk variableChunk;
            variableChunk.setText(variable);
            variableChunk.setType(variable == TextChunk::ACTION_TEXT_DEF ? TextChunk::ChunkType::ACTION_TEXT : TextChunk::ChunkType::VARIABLE);
            result.push_back(variableChunk);

            start = varIt->position() + varIt->length();
        }

        if (start < line.length())
        {
            TextChunk textChunk;
            textChunk.setText(line.substr(start));
            textChunk.setType(TextChunk::ChunkType::TEXT);
            result.push_back(textChunk);
        }
    }
    return result;
}

bool StringHelper::isEmpty(const std::string &str)
{
    return str.empty() || str == nlb::Constants::EMPTY_STRING;
}

bool StringHelper::notEmpty(const std::string &str)
{
    return !isEmpty(str);
}

bool StringHelper::isEmpty(const MultiLangString &multiLangString)
{
    for (const auto &text : multiLangString.values())
    {
        if (!isEmpty(text))
        {
            return false;
        }
    }
    return true;
}

std::string StringHelper::formatSequence(const std::vector<std::string> &strings)
{
    if (strings.empty())
    {
        return "";
    }

    std::ostringstream ss;
    for (size_t i = 0; i < strings.size() - 1; ++i)
    {
        ss << strings[i] << DELIMITER;
    }
    ss << strings.back();
    return ss.str();
}

std::vector<std::string> StringHelper::getItems(const std::string &sequenceString)
{
    if (sequenceString.empty())
    {
        return std::vector<std::string>();
    }

    std::vector<std::string> result;
    size_t start = 0;
    size_t end = sequenceString.find(DELIMITER);

    while (end != std::string::npos)
    {
        result.push_back(sequenceString.substr(start, end - start));
        start = end + DELIMITER.length();
        end = sequenceString.find(DELIMITER, start);
    }

    result.push_back(sequenceString.substr(start));
    return result;
}

std::string StringHelper::createRepeatedString(size_t length, const std::string &fill)
{
    if (length == 0 || fill.empty())
    {
        return "";
    }

    std::string result;
    result.reserve(length * fill.length());
    while (result.length() < length)
    {
        result += fill;
    }
    return result.substr(0, length);
}