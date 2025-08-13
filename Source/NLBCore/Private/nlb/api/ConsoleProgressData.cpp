#include "nlb/api/ConsoleProgressData.h"
#include <stdio.h>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void ConsoleProgressData::setProgressValue(int progress)
{
    double percentage = progress / 100.0;
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", progress, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

void ConsoleProgressData::setNoteText(const std::string& text)
{
}
