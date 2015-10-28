//
//  Chapter_functions.h
//  reaper_Ultraschall
//
//  Created by Malte Dreschert on 04.05.14.
//
//

#ifndef reaper_Ultraschall_Chapter_functions_h
#define reaper_Ultraschall_Chapter_functions_h

#include <fstream>
#include <vector>

#include "Chapter.h"

#include "reaper.h"
#include "tinyxml2.h"

void ExportChapters(COMMAND_T*)
{
    int markerIndex = 0;
    bool isRegion;
    double pos;
    double regionEnd;
    const char *markerName;
    int index;
    char *charStr = new char[4096];
    char *projectName = new char[4096];
    
    
    EnumProjects(0, projectName, 4096);
    
    if ( strcmp(projectName, "") == 0 ) {
        ShowMessageBox("The Project was not safed", "Project location error", 0);
        return;
    }
    
    std::vector<std::string> chapterLines;
    
    while( EnumProjectMarkers( markerIndex++, &isRegion, &pos, &regionEnd, &markerName, &index) > 0 ) {
        format_timestr_pos(pos, charStr, 4096, 0);
        
        std::string a = _format_time(charStr);
        
        a.append((std::string)" " + markerName);
        
        chapterLines.push_back(a);
    }
    
    std::string sProjectName = std::string(projectName);
    auto chapterFilename = sProjectName.substr( 0, sProjectName.find('.', 0) );
    
    char* fn = new char[4096];
    BrowseForSaveFile("Choose Chapter file...", NULL, chapterFilename.c_str(), "Chapter file (*.mp4chaps)\0*.mp4chaps\0Text file (*.txt)\0*.txt\0", fn, 4096);
    
    
    std::ofstream chapterFile(fn, std::ios::out);
    
    for (std::vector<std::string>::iterator iter = chapterLines.begin(); iter != chapterLines.end(); ++iter)
    {
        chapterFile << iter->data() << "\n";
    }
    
    
    
    chapterFile.close();
    
    char buffer[4096];
    sprintf(buffer, "The file was saved in:\n%s", fn);
    ShowMessageBox(buffer, "Chapter file saved", 0);
    
    delete[](charStr);
    delete[](projectName);
    
}

void ExportChaptersAsSimpleChapters(COMMAND_T*)
{
    int markerIndex = 0;
    bool isRegion;
    double pos;
    double regionEnd;
    const char *markerName;
    int index;
    char *charStr = new char[4096];
    char *projectName = new char[4096];
    
    
    EnumProjects(0, projectName, 4096);
    
    if ( strcmp(projectName, "") == 0 ) {
        ShowMessageBox("The Project was not safed", "Project location error", 0);
        return;
    }
    
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *rootElement = doc.NewElement("psc:chapters");
    rootElement->SetAttribute("xmlns", "psc=\"http://podlove.org/simple-chapters\"");
    rootElement->SetAttribute("version", "1.2");
    
    doc.InsertFirstChild(rootElement);
    
    while( EnumProjectMarkers( markerIndex++, &isRegion, &pos, &regionEnd, &markerName, &index) > 0 ) {
        tinyxml2::XMLElement *chapterElement = doc.NewElement("psc:chapter");
        format_timestr_pos(pos, charStr, 4096, 0);

        chapterElement->SetAttribute("start", _format_time(charStr).c_str());
        chapterElement->SetAttribute("title", markerName);
        rootElement->InsertEndChild(chapterElement);
    }
    
    std::string sProjectName = std::string(projectName);
    auto chapterFilename = sProjectName.substr( 0, sProjectName.find('.', 0) );
    chapterFilename.append(".simplechapters");
    
    doc.SaveFile(chapterFilename.c_str());
    
    char buffer[4096];
    sprintf(buffer, "The file was saved in:\n%s", chapterFilename.c_str());
    ShowMessageBox(buffer, "Chapter file saved", 0);
    
    delete[](charStr);
    delete[](projectName);
}

void readChapterFile(char* fileName, MediaTrack* track )
{
    
    std::ifstream infile(fileName);
    int markerindex = 0;
    std::string line;
    
    while (std::getline(infile, line)) {
        
        
        std::string timeStamp = line.substr(0, line.find(' ', 0));
        std::string chapterText = line.substr(line.find(' ', 0)+1, line.length());
        
        std::string* test = &chapterText;
        
        double time = parse_timestr(timeStamp.c_str());
        
        
        MediaItem* item = AddMediaItemToTrack(track);
        if(item)
        {
            SetMediaItemPosition(item, time, true);
            SetMediaItemLength(item, 10, true);
            GetSetMediaItemInfo(item, "P_NOTES", (void*)test->c_str());
            AddProjectMarker2(0, false, time, 10, chapterText.c_str(), 0, 0xFFFF00);
        }
        markerindex++;
        
    }
    
    infile.close();
    
}


void ImportChapters(COMMAND_T*)
{
    char* chapterTrackName = (char*)"Chapters";
    
    auto path = BrowseForFiles("Select chapters file", NULL, NULL, false, "Chapter marker text-file (*.mp4chaps)\0*.mp4chaps\0Text file (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
    if(path) {
        MediaTrack* track = getTrackByName((char*)"Chapters");
        if(!track)
        {
            InsertTrackAtIndex(0, true);
            track = GetTrack(0,0);
            GetSetMediaTrackInfo(track, "P_NAME", chapterTrackName);
        }

        readChapterFile(path, track);
    }
    
    
    //free(path);
    
}

static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "Ultraschall Load chapter" }, "ULTRASCHALL_CHAPTER_LOAD", ImportChapters, },
	//{ { DEFACCEL, "Ultraschall Export Simplechapters" }, "ULTRASCHALL_CHAPTER_EXPORT_SIMPLE", ExportChaptersAsSimpleChapters, },
	{ { DEFACCEL, "Ultraschall Export chapters" }, "ULTRASCHALL_CHAPTER_EXPORT", ExportChapters, },
    { {}, LAST_COMMAND, }, // Denote end of table
};

int UltraschallChaptersInit()
{
	RegisterCommands(g_commandTable);
    
	return 1;
}


#endif
