Coding-Shownotes-Python
=======================

Python scripts for working with Shownotes in Reaper

This is a first version of a shownote importer for Reaper.

To be used, the SWS-extension fork is needed for writing text into the empty items, which is not standard in the 
Reascript Api.

To launch, load the script as an actiopn and execute.

The script will look for a track named 'Shownotes'. If it does not find one, it will create a new one.
It will then load the file freak-show-128.osf from the desktop and populate the Shownote track with the shownotes.

Known issues:
- There are problems with unicode -> ascii conversion at the moment. These might be fixed with Python version 3.
- We need a better way to access the file, either through a finder or through a nomenclature.
- Shownotes starting with '-' are not imported yet.
- The length of the shownote items is not yet correct.
- I already found some errors with strangley overlapping items.


