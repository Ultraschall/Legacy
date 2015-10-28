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

#ifndef __ULTRASCHALL_PLATFORM_ACTIVITY_ITEM_H_INCL__
#define __ULTRASCHALL_PLATFORM_ACTIVITY_ITEM_H_INCL__

#include <set>

namespace ultraschall { namespace auphonic {

class ActivityItem
{
public:
   ActivityItem() :
      begin_(0.0), end_(0.0)
   {
   }
   
   ActivityItem(const double begin, const double end) :
      begin_(begin), end_(end)
   {
      if(end_ < begin_)
      {
         // TODO: throw RangeException();
      }
   }
   
   const bool operator<(const ActivityItem& rhs) const
   {
      return (Begin() < rhs.Begin());
   }
   
   const double Begin() const
   {
      return begin_;
   }
   
   const double End() const
   {
      return end_;
   }
   
   const double Duration() const
   {
      return End() - Begin();
   }
   
private:
   double begin_;
   double end_;
};
   
int Initialize();
void ImportJson(COMMAND_T* pCommand);
   
}}

#endif // __ULTRASCHALL_PLATFORM_ACTIVITY_ITEM_H_INCL__

/////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2014 The Ultraschall Project http://ultraschall.wikigeeks.de
//
/////////////////////////////////////////////////////////////////////////////////


