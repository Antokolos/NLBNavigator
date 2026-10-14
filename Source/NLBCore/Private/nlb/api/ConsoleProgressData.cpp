#include "nlb/api/ConsoleProgressData.h"
#include <cstdio>
#include <iostream>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void ConsoleProgressData::setProgressValue(int progress)
{
    double percentage = progress / 100.0;
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "\r%3d%% [%.*s%*s]", progress, lpad, PBSTR, rpad, "");
    std::cout << buf;
    if (progress >= 100) {
        std::cout << "\n";  // индикатор завершён — дальнейший вывод с новой строки
    }
    std::cout.flush();
}

void ConsoleProgressData::setNoteText(const std::string& text)
{
}
