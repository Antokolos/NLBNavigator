#pragma once

#include <algorithm>
#include <iterator>
#include <random>
#include <sstream>
#include <iomanip>

class NLBUUID {
public:
    static std::string randomUUID() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        
        for (int i = 0; i < 8; i++) 
            ss << std::setw(1) << dis(gen);
        ss << "-";
        for (int i = 0; i < 4; i++) 
            ss << std::setw(1) << dis(gen);
        ss << "-4";
        for (int i = 0; i < 3; i++) 
            ss << std::setw(1) << dis(gen);
        ss << "-" << std::setw(1) << dis2(gen);
        for (int i = 0; i < 3; i++) 
            ss << std::setw(1) << dis(gen);
        ss << "-";
        for (int i = 0; i < 12; i++) 
            ss << std::setw(1) << dis(gen);
        
        return ss.str();
    }
};