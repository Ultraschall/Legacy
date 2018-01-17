//
//  Chapter.h
//  Reaper Plugin
//
//  Created by Daniel Lindenfelser on 22.06.14.
//

#ifndef Reaper_Plugin_Chapter_h
#define Reaper_Plugin_Chapter_h

#include "reaper.h"

void ExportChapters(COMMAND_T*);
void ExportChaptersAsSimpleChapters(COMMAND_T*);
void ImportChapters(COMMAND_T*);

int UltraschallChaptersInit();

#endif
