#pragma once

#include <algorithm>
#include <iterator>
#include <random>
#include <string>

class NLBUUID {
public:
    static std::string randomUUID() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        // без std::stringstream: потоки тянут локаль libc++, а UUID создаётся очень часто (и мог
        // создаваться при загрузке библиотеки) — шестнадцатеричные цифры пишем сами
        static const char HEX[] = "0123456789abcdef";
        std::string s;
        s.reserve(36);
        auto digits = [&](int n) { for (int i = 0; i < n; i++) s += HEX[dis(gen)]; };
        digits(8);
        s += '-';
        digits(4);
        s += "-4";
        digits(3);
        s += '-';
        s += HEX[dis2(gen)];
        digits(3);
        s += '-';
        digits(12);
        return s;
    }
};