
"""This module prompts the reaper user to open an mp4chaps chapters file.
   It parses the file and creates Empty Timeline items with the chapter text attached
"""
import os
import sys
import codecs
import os.path
import datetime
from ctypes import *
#import pdb

import ultraschall_functions as ULT

# Get the Reaper scripts folder and add it to pythons library path
script_path = os.path.join(os.path.expanduser("~"), 'Library', 'Application Support', 'REAPER', 'Scripts')
sys.path.append(script_path)
from sws_python64 import *


__author__ = "Malte Dreschert, Ralf Stockmann"
__copyright__ = "Copyright 2014, Ultraschall"
__credits__ = []
__license__ = "MIT"
__version__ = "0.1"
__maintainer__ = ""
__email__ = ""
__status__ = "Development"


#filepath = os.path.join(os.path.expanduser("~"), 'Desktop', 'freak-show-128.osf')

def createChapterTrack():
	"""create the shownote track"""
	RPR_InsertTrackAtIndex(RPR_GetNumTracks() + 1, True)
	RPR_UpdateArrange()
	track = RPR_GetTrack(0, RPR_GetNumTracks()-1)
	RPR_GetSetMediaTrackInfo_String(track, 'P_NAME', 'Chapters', True)
	return track

def createChapterItem(lines, track):
	"""iterate the Chapter content and create chapter items on the track
	"""

	lastposition = None
	maxlength = 30 # Standardlänge für Chapter-Einträge

	for line in lines:

		splitstring = line.strip().split(' ')

		# This line has no information
		if line == '\n':
			continue

		time = RPR_parse_timestr(splitstring[0])
		

		if lastposition == None:
			length = maxlength
		else:
			length = (lastposition-time)
			#if length > maxlength: # es folgt im maxlenght Abstand kein neuer Eintrag
			#	length = maxlength

		lastposition = time

		note = ' '.join(splitstring[1:len(splitstring)])

		item = RPR_AddMediaItemToTrack(track)
		RPR_SetMediaItemLength(item, length, False)
		RPR_SetMediaItemPosition(item, time, False)
		ULT_SetMediaItemNote(item, note.encode('ascii', 'replace'))

def loadChapterFile():
	"""new main function. used to bail out if the user cancels the import request"""


	# Show the open file dialog. 
	selected_file_tuple = RPR_GetUserFileNameForRead(None, 'Select Chapter file to import', 'mp4chaps')
	filepath = None
	if(selected_file_tuple[0] == 0):
		return
	else:
		filepath = selected_file_tuple[1]

	# open the file and read the content into an arrayreap
	with codecs.open(filepath, 'r', encoding='utf-8') as f:
		lines = f.readlines()

	# we start at the end to have the length information available
	lines.reverse()
	
	# check if there is a chapters track. If not create one
	track = ULT.getTrackByName('Chapters')
	if not track:
		track = createChapterTrack()

	# call the function that creates the chapter items
	createChapterItem(lines, track)

	# update the gui -> does not work right now
	RPR_UpdateArrange()

loadChapterFile()
