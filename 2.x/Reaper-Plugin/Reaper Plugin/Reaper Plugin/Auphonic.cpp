/////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2014 The Ultraschall Project http://ultraschall.wikigeeks.de
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
/////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <cassert>
#include "json11.hpp"
#include "reaper.h"
#include "Auphonic.h"

#define AssertTrue(__condition__) \
assert((__condition__));

#define ErrorMessage(a) { \
char __messageBuffer__[4096] = {0}; \
sprintf(__messageBuffer__, "%s(%d): %s", __FILE__, __LINE__, (a)); \
ShowMessageBox(__messageBuffer__, "Auphonic JSON Import Error", MB_OK); }

namespace ultraschall { namespace auphonic {

const std::string GetFileContents(const std::string& filename);
char* AllocCopyString(const char* source);

const std::string&& MediaTrack_GetStringProperty(MediaTrack* track, const char* property);
void MediaTrack_SetStringProperty(MediaTrack* track, const char* property, const std::string& value);
   
int MediaTrack_GetIntProperty(MediaTrack* track, const char* property);
void MediaTrack_SetIntProperty(MediaTrack* track, const char* property, const int value);

double MediaTrack_GetDoubleProperty(MediaTrack* track, const char* property);
void MediaTrack_SetDoubleProperty(MediaTrack* track, const char* property, const double value);

bool MediaTrack_GetBoolProperty(MediaTrack* track, const char* property);
void MediaTrack_SetBoolProperty(MediaTrack* track, const char* property, const bool value);

const std::string&& MediaItem_GetStringProperty(MediaItem* item, const char* property);
void MediaItem_SetStringProperty(MediaItem* item, const char* property, const std::string& value);

template<class T> const T&& MediaTrack_GetProperty(MediaTrack* track, const char* property)
{
   AssertTrue(track != 0);
   AssertTrue(property != 0);
      
   T* resultPointer = reinterpret_cast<T*>(GetSetMediaTrackInfo(track, property, 0));
   AssertTrue(resultPointer != 0);
      
   T value = *resultPointer;
   return std::move(value);
}
   
template<> const std::string&& MediaTrack_GetProperty(MediaTrack* track, const char* property)
{
   AssertTrue(track != 0);
   AssertTrue(property != 0);
      
   char* resultPtr = reinterpret_cast<char*>(GetSetMediaTrackInfo(track, property, 0));
   std::string value(resultPtr);
   return std::move(value);
}
   
template<class T> const T&& MediaItem_GetProperty(MediaItem* item, const char* property)
{
   AssertTrue(item != 0);
   AssertTrue(property != 0);
      
   T* resultPointer = reinterpret_cast<T*>(GetSetMediaItemInfo(item, property, 0));
   AssertTrue(resultPointer != 0);
      
   T value = *resultPointer;
   return std::move(value);
}
   
template<> const std::string&& MediaItem_GetProperty(MediaItem* item, const char* property)
{
   AssertTrue(item != 0);
   AssertTrue(property != 0);
      
   char* resultPtr = reinterpret_cast<char*>(GetSetMediaItemInfo(item, property, 0));
   std::string value(resultPtr);
   return std::move(value);
}
   
   
static COMMAND_T g_commandTable[] =
{
   {{DEFACCEL, "Ultraschall Import Auphonic JSON" }, "ULTRASCHALL_IMPORT_AUPHONIC_JSON", ImportJson, },
   {{}, LAST_COMMAND, }, // Denote end of table
};
   
int Initialize()
{
	RegisterCommands(g_commandTable);
   return 1;
}
   
void ImportJson(COMMAND_T* pCommand)
{
   char* selectedFile = BrowseForFiles("Select Auphonic JSON file", 0, 0, false, "Auphonic JSON (*.json)\0*.json\0All Files (*.*)\0*.*\0");
   if((selectedFile != 0) && (strlen(selectedFile) > 0))
   {
      std::string rawData = GetFileContents(std::string(selectedFile));
      std::string errorMessage;
      json11::Json cookedData = json11::Json::parse(rawData, errorMessage);
      if(errorMessage.empty() == true)
      {
         std::map<std::string, std::set<ActivityItem>> trackActivities;
         
         const json11::Json statistics = cookedData["statistics"];
         for(const json11::Json& track : statistics["tracks"].array_items())
         {
            const std::string trackId = track["identifier"].string_value();
            if(trackId.empty() == false)
            {
               std::set<ActivityItem> activities;
               for(const json11::Json& activity : track["activity"].array_items())
               {
                  const double begin = activity.array_items()[0].number_value();
                  const double end = activity.array_items()[1].number_value();
                  activities.insert(ActivityItem(begin, end));
               }
               
               if(activities.empty() == false)
               {
                  trackActivities.insert(std::map<std::string, std::set<ActivityItem>>::value_type(trackId, activities));
               }
               else
               {
                  ErrorMessage("No activities for track");
               }
            }
            else
            {
               ErrorMessage("Found invalid track identifier");
            }
         }

         std::map<std::string, MediaTrack*> reaperTracks;
         for(int i = 0; i < GetNumTracks(); ++i)
         {
            MediaTrack* reaperTrack = GetTrack(0, i);
            AssertTrue(reaperTrack != 0);
            std::string trackName = MediaTrack_GetStringProperty(reaperTrack, "P_NAME");
            reaperTracks[trackName] = reaperTrack;
         }
         
         for(const std::map<std::string, std::set<ActivityItem>>::value_type& trackActivity : trackActivities)
         {
            const std::string& trackId = trackActivity.first;
            std::map<std::string, MediaTrack*>::iterator trackIterator = reaperTracks.find(trackId);
            if(trackIterator != reaperTracks.end())
            {
               MediaTrack* reaperTrack = trackIterator->second;
               if(reaperTrack != 0)
               {
                  MediaItem* reaperItem = AddMediaItemToTrack(reaperTrack);
                  if(reaperItem != 0)
                  {
                     const std::set<ActivityItem>& activities = trackActivity.second;
                     for(const ActivityItem& activity : activities)
                     {
                        SetMediaItemPosition(reaperItem, activity.Begin(), true);
                        SetMediaItemLength(reaperItem, activity.Duration(), true);
                        MediaItem_SetStringProperty(reaperItem, "P_NOTES", trackId);
                        
//                        int color = MediaTrack_GetIntProperty(reaperTrack, "I_CUSTOMCOLOR");
                        int color = MediaTrack_GetProperty<int>(reaperTrack, "I_CUSTOMCOLOR");
                        
                        char buffer[4096] = {0};
                        sprintf(buffer, "%s (%.2fs)", trackId.c_str(), activity.Duration());
                        const char* activityLabel = AllocCopyString(buffer);

                        AddProjectMarker2(0, true, activity.Begin(), activity.End(), activityLabel, 0, color);
                     }
                  }
                  else
                  {
                     ErrorMessage("Failed to add activity");
                  }
               }
               else
               {
                  ErrorMessage("Failed to find track");
               }
            }
         }
      }
      else
      {
         ErrorMessage(errorMessage.c_str());
      }
   }
}

const std::string GetFileContents(const std::string& filename)
{
   std::string contents;
      
   std::ifstream in(filename, std::ios::in | std::ios::binary);
   if(in)
   {
      in.seekg(0, std::ios::end);
      contents.resize(in.tellg());
      in.seekg(0, std::ios::beg);
      in.read(&contents[0], contents.size());
      in.close();
   }
      
   return contents;
}
   
char* AllocCopyString(const char* source)
{
   char* target = 0;
   if(source != 0)
   {
      const size_t sourceLength = strlen(source);
      if(sourceLength > 0)
      {
         target = new char[sourceLength + 1];
         if(target != 0)
         {
            strcpy(target, source);
            target[sourceLength] = '\0';
         }
      }
   }
   
   return target;
}

const std::string&& MediaTrack_GetStringProperty(MediaTrack* track, const char* property)
{
   char* resultPtr = reinterpret_cast<char*>(GetSetMediaTrackInfo(track, property, 0));
   std::string value(resultPtr);
   return std::move(value);
}

void MediaTrack_SetStringProperty(MediaTrack* track, const char* property, const std::string& value)
{
   char* valuePtr = AllocCopyString(value.c_str());
   GetSetMediaTrackInfo(track, property, valuePtr);
}

int MediaTrack_GetIntProperty(MediaTrack* track, const char* property)
{
   int* resultPtr = reinterpret_cast<int*>(GetSetMediaTrackInfo(track, property, 0));
   AssertTrue(resultPtr != 0);
   int value = *resultPtr;
   return value;
}
   
void MediaTrack_SetIntProperty(MediaTrack* track, const char* property, const int value)
{
   GetSetMediaTrackInfo(track, property, (void*)&value);
}

double MediaTrack_GetDoubleProperty(MediaTrack* track, const char* property)
{
   double* resultPtr = reinterpret_cast<double*>(GetSetMediaTrackInfo(track, property, 0));
   AssertTrue(resultPtr != 0);
   double value = *resultPtr;
   return value;
}
   
void MediaTrack_SetDoubleProperty(MediaTrack* track, const char* property, const double value)
{
   GetSetMediaTrackInfo(track, property, (void*)&value);
}
   
bool MediaTrack_GetBoolProperty(MediaTrack* track, const char* property)
{
   bool* resultPtr = reinterpret_cast<bool*>(GetSetMediaTrackInfo(track, property, 0));
   AssertTrue(resultPtr != 0);
   bool value = *resultPtr;
   return value;
}

void MediaTrack_SetBoolProperty(MediaTrack* track, const char* property, const bool value)
{
   GetSetMediaTrackInfo(track, property, (void*)&value);
}

const std::string&& MediaItem_GetStringProperty(MediaItem* item, const char* property)
{
   char* resultPtr = reinterpret_cast<char*>(GetSetMediaItemInfo(item, property, 0));
   std::string value(resultPtr);
   return std::move(value);
}
   
void MediaItem_SetStringProperty(MediaItem* item, const char* property, const std::string& value)
{
   char* valuePtr = AllocCopyString(value.c_str());
   GetSetMediaItemInfo(item, property, valuePtr);
}
  
}}

/////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2014 The Ultraschall Project http://ultraschall.wikigeeks.de
//
/////////////////////////////////////////////////////////////////////////////////


