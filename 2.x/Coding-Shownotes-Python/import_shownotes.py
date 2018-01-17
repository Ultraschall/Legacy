

import sys
import io
import os.path
import datetime
#import pdb

# Get the Reaper scripts folder and add it to pythons library path
script_path = os.path.join(os.path.expanduser("~"), 'Library', 'Application Support', 'REAPER', 'Scripts')
sys.path.append(script_path)
from sws_python64 import *

filepath = os.path.join(os.path.expanduser("~"), 'Desktop', 'freak-show-128.osf')

# iterate all tracks and find the one with the specified name
def getTrackByName(name):
	for i in range(RPR_GetNumTracks()):
		track = RPR_GetTrack(0, i)
		track_name = RPR_GetSetMediaTrackInfo_String(track, 'P_NAME', None, False)[3]
		if track_name == name:
			return track
		else:
			return None

# create the shownote track
def createShownoteTrack():
	RPR_InsertTrackAtIndex(RPR_GetNumTracks() + 1, True)
	RPR_UpdateArrange()
	track = RPR_GetTrack(0, RPR_GetNumTracks()-1)
	RPR_GetSetMediaTrackInfo_String(track, 'P_NAME', 'Shownotes', True)
	return track

# iterate the shownote content and create shownote items on the track
# for the moment I omit the shownotes starting with '-'
def createShownoteItem():

	lastposition = None

	for line in lines:

		splitstring = line.strip().split(' ')
		if line == '\n':
			continue
		if line.startswith('-'):
			continue

		if not line.startswith('-'):
			timestamp = datetime.datetime.fromtimestamp(int(splitstring[0]))
			position = (timestamp-starttime).total_seconds()

			if lastposition==None:
				length=3
			else:
				length = (lastposition-timestamp).total_seconds()
				length=3

			lastposition=timestamp

			note = ' '.join(splitstring[1:len(splitstring)])


			item = RPR_AddMediaItemToTrack(track)
			RPR_SetMediaItemLength(item, length, False)
			RPR_SetMediaItemPosition(item, position, False)
			POD_SetMediaItemNote(item, note.encode('ascii', 'replace'))

# open the file and read the content into an arrayreap
with io.open(filepath, 'r', encoding='utf-8') as f:
	lines = f.readlines()[15:]

# get the first starttime. This is our 0 time
starttime = datetime.datetime.fromtimestamp(int(lines[0].split(' ')[0]))

# we start at the end to have the length information available
lines.reverse()

# check if there is a shownote track. If not create one
track = getTrackByName('Shownotes')
if not track:
	track = createShownoteTrack()

createShownoteItem()
RPR_UpdateArrange()
