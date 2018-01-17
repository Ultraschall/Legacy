//
//  Shownote.h
//  Reaper Plugin
//
//  Created by Daniel Lindenfelser on 22.06.14.
//

#ifndef Reaper_Plugin_Shownote_h
#define Reaper_Plugin_Shownote_h

#include <string>

#include "reaper.h"

struct shownoteData
{
    double pos;
    std::string note;
};

void ExportShownotes(COMMAND_T*);
void ImportShownotes(COMMAND_T*);

int UltraschallShownotesInit();
#endif
