//
//  reaper.cpp
//  Reaper Plugin
//
//  Created by Daniel Lindenfelser on 21.06.14.
//

#include <iostream>
#include <vector>

#define REAPERAPI_DECL
#include "reaper.h"
#undef REAPERAPI_DECL

#ifdef _WIN32
#include <windows.h>
#else
#include "WDL/swell/swell.h"
#endif

#define LOCALIZE_IMPORT_PREFIX "ultraschall_"
#ifdef LOCALIZE_IMPORT_PREFIX
#include "localize-import.h"
#endif
#include "localize.h"

#include "Chapter.h"
#include "Shownote.h"
#include "Soundboard.h"
#include "Auphonic.h"

// Globals
REAPER_PLUGIN_HINSTANCE g_hInst = NULL;
HWND g_hwndParent = NULL;

static WDL_IntKeyedArray<COMMAND_T*> g_commands; // no valdispose (cmds can be allocated in different ways)

int g_iFirstCommand = 0;
int g_iLastCommand = 0;

int WDL_STYLE_WantGlobalButtonBorders() { return 0; }
bool WDL_STYLE_WantGlobalButtonBackground(int *col) { return false; }
int WDL_STYLE_GetSysColor(int p) { return GetSysColor(p); }
void WDL_STYLE_ScaleImageCoords(int *x, int *y) { }
bool WDL_Style_WantTextShadows(int *col) { return false; }
bool WDL_STYLE_GetBackgroundGradient(double *gradstart, double *gradslope) { return false; }
LICE_IBitmap *WDL_STYLE_GetSliderBitmap2(bool vert) { return NULL; }
bool WDL_STYLE_AllowSliderMouseWheel() { return true; }
int WDL_STYLE_GetSliderDynamicCenterPos() { return 500; }

//JFB questionnable func: ok most of the time but, for ex.,
// 2 different cmds can share the same function pointer cmd->doCommand
int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), INT_PTR user, const char** pMenuText)
{
	for (int i=0; i<g_commands.GetSize(); i++)
	{
		if (COMMAND_T* cmd = g_commands.Enumerate(i, NULL, NULL))
		{
			if (cmd->doCommand == cmdFunc && cmd->user == user)
			{
				if (pMenuText)
					*pMenuText = cmd->menuText;
				return cmd->accel.accel.cmd;
			}
		}
	}
	return 0;
}

// 1) Get command ID from Reaper
// 2) Add keyboard accelerator (with localized action name) and add to the "action" list
int RegisterCmd(COMMAND_T* pCommand, const char* cFile, int cmdId)
{
    if (!pCommand || !pCommand->id || !pCommand->accel.desc || (!pCommand->doCommand && !pCommand->onAction)) return 0;
    
	// localized action name, if needed
	const char* defaultName = pCommand->accel.desc;
    
	if (!pCommand->uniqueSectionId && pCommand->doCommand)
	{
		if (!cmdId) cmdId = plugin_register("command_id", (void*)pCommand->id);
		if (cmdId)
		{
			pCommand->accel.accel.cmd = cmdId; // need to this before registering "gaccel"
			cmdId = (plugin_register("gaccel", &pCommand->accel) ? cmdId : 0);
		}
	}
	else if (pCommand->onAction)
	{
		static custom_action_register_t s;
		memset(&s, 0, sizeof(custom_action_register_t));
		s.idStr = pCommand->id;
		s.name = pCommand->accel.desc;
		s.uniqueSectionId = pCommand->uniqueSectionId;
		cmdId = plugin_register("custom_action", (void*)&s); // will re-use the known cmd ID, if any
	}
	else
		cmdId = 0;
	pCommand->accel.accel.cmd = cmdId;
    
	// now that it is registered, restore the default action name
	if (pCommand->accel.desc != defaultName) pCommand->accel.desc = defaultName;
    
	if (!cmdId) return 0;
    
	if (!g_iFirstCommand || g_iFirstCommand > cmdId) g_iFirstCommand = cmdId;
	if (cmdId > g_iLastCommand) g_iLastCommand = cmdId;
    
	g_commands.Insert(cmdId, pCommand);
    
	return pCommand->accel.accel.cmd;
}

// For each item in table call RegisterCommand
int RegisterCmds(COMMAND_T* pCommands, const char* cFile)
{
	int i = 0;
	while(pCommands[i].id != LAST_COMMAND)
		RegisterCmd(&pCommands[i++], cFile, 0);
	return 1;
}

COMMAND_T* GetCommandByID(int cmdId) {
	if (cmdId >= g_iFirstCommand && cmdId <= g_iLastCommand) // not enough to ensure it is a SWS action
		return g_commands.Get(cmdId, NULL);
	return NULL;
}

bool hookCommandProc(int iCmd, int flag)
{
    static WDL_PtrList<const char> sReentrantCmds;
    
    // Ignore commands that don't have anything to do with us from this point forward
	if (COMMAND_T* cmd = GetCommandByID(iCmd))
	{
		if (!cmd->uniqueSectionId && cmd->accel.accel.cmd==iCmd && cmd->doCommand)
		{
			if (sReentrantCmds.Find(cmd->id)<0)
			{
				sReentrantCmds.Add(cmd->id);
				cmd->fakeToggle = !cmd->fakeToggle;
				cmd->doCommand(cmd);
				sReentrantCmds.Delete(sReentrantCmds.Find(cmd->id));
				return true;
			}
		}
	}
    
    return false;
}

extern "C"
{
    
    REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
    {
        if (rec)
        {
            if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
                return 0;
            
            #define IMPAPI(x) if (!(*((void **)&x) = rec->GetFunc(#x))) return 0;
            IMPAPI(AddExtensionsMainMenu);
            IMPAPI(AddMediaItemToTrack);
            IMPAPI(AddProjectMarker);
            IMPAPI(AddProjectMarker2);
            IMPAPI(AddTakeToMediaItem);
            /* deprecated
             IMPAPI(AddTempoTimeSigMarker);
             */
            IMPAPI(adjustZoom);
            IMPAPI(ApplyNudge);
            IMPAPI(AttachWindowTopmostButton);
            IMPAPI(AttachWindowResizeGrip);
            IMPAPI(Audio_RegHardwareHook);
            IMPAPI(AudioAccessorValidateState);
            IMPAPI(CoolSB_GetScrollInfo);
            IMPAPI(CoolSB_SetScrollInfo);
            IMPAPI(CountActionShortcuts);
            IMPAPI(CountMediaItems);
            IMPAPI(CountSelectedMediaItems);
            IMPAPI(CountSelectedTracks);
            IMPAPI(CountTakes);
            IMPAPI(CountTCPFXParms);
            IMPAPI(CountTempoTimeSigMarkers);
            IMPAPI(CountTracks);
            IMPAPI(CountTrackMediaItems);
            IMPAPI(CountTrackEnvelopes);
            IMPAPI(CreateLocalOscHandler);
            IMPAPI(CreateNewMIDIItemInProj);
            IMPAPI(CreateTakeAudioAccessor);
            IMPAPI(CreateTrackAudioAccessor);
            IMPAPI(CSurf_FlushUndo);
            IMPAPI(CSurf_GoEnd);
            IMPAPI(CSurf_OnMuteChange);
            IMPAPI(CSurf_OnPanChange);
            IMPAPI(CSurf_OnSelectedChange);
            IMPAPI(CSurf_OnTrackSelection);
            IMPAPI(CSurf_OnVolumeChange);
            IMPAPI(CSurf_OnWidthChange);
            IMPAPI(CSurf_TrackFromID);
            IMPAPI(CSurf_TrackToID);
            IMPAPI(DeleteActionShortcut);
            IMPAPI(DeleteProjectMarker);
            IMPAPI(DeleteProjectMarkerByIndex);
            IMPAPI(DeleteTakeStretchMarkers);
            IMPAPI(DeleteTrack);
            IMPAPI(DeleteTrackMediaItem);
            IMPAPI(DestroyAudioAccessor);
            IMPAPI(DestroyLocalOscHandler);
            IMPAPI(DoActionShortcutDialog);
            IMPAPI(Dock_UpdateDockID);
            IMPAPI(DockIsChildOfDock);
            IMPAPI(DockWindowActivate);
            IMPAPI(DockWindowAdd);
            IMPAPI(DockWindowAddEx);
            IMPAPI(DockWindowRefresh);
            IMPAPI(DockWindowRemove);
            IMPAPI(EnsureNotCompletelyOffscreen);
            IMPAPI(EnumProjectMarkers);
            IMPAPI(EnumProjectMarkers2);
            IMPAPI(EnumProjectMarkers3);
            IMPAPI(EnumProjects);
            IMPAPI(file_exists);
            IMPAPI(format_timestr);
            IMPAPI(format_timestr_pos);
            IMPAPI(format_timestr_len);
            IMPAPI(FreeHeapPtr);
            IMPAPI(GetActionShortcutDesc);
            IMPAPI(GetActiveTake);
            IMPAPI(GetAppVersion);
            IMPAPI(GetAudioAccessorEndTime);
            IMPAPI(GetAudioAccessorHash);
            IMPAPI(GetAudioAccessorSamples);
            IMPAPI(GetAudioAccessorStartTime);
            IMPAPI(GetColorThemeStruct);
            IMPAPI(GetContextMenu);
            IMPAPI(GetCurrentProjectInLoadSave);
            IMPAPI(GetCursorContext);
            IMPAPI(GetCursorPosition);
            IMPAPI(GetCursorPositionEx);
            IMPAPI(GetEnvelopeName);
            IMPAPI(GetExePath);
            IMPAPI(GetFocusedFX);
            IMPAPI(GetHZoomLevel);
            IMPAPI(GetIconThemePointer);
            IMPAPI(GetIconThemeStruct);
            IMPAPI(GetInputChannelName);
            IMPAPI(GetItemEditingTime2);
            IMPAPI(GetLastTouchedFX);
            IMPAPI(GetLastTouchedTrack);
            IMPAPI(GetMainHwnd);
            IMPAPI(GetMasterMuteSoloFlags);
            IMPAPI(GetMasterTrack);
            IMPAPI(GetMediaItem);
            IMPAPI(GetMediaItem_Track);
            IMPAPI(GetMediaItemInfo_Value);
            IMPAPI(GetMediaItemNumTakes);
            IMPAPI(GetMediaItemTake);
            IMPAPI(GetMediaItemTakeByGUID);
            IMPAPI(GetMediaItemTake_Item);
            IMPAPI(GetMediaItemTake_Source);
            IMPAPI(GetMediaItemTake_Track);
            IMPAPI(GetMediaItemTakeInfo_Value);
            IMPAPI(GetMediaSourceFileName);
            IMPAPI(GetMediaSourceType);
            IMPAPI(GetMediaTrackInfo_Value);
            IMPAPI(get_midi_config_var);
            IMPAPI(GetMouseModifier);
            IMPAPI(GetNumTracks);
            IMPAPI(GetOutputChannelName);
            IMPAPI(GetPeakFileName);
            IMPAPI(GetPeaksBitmap);
            IMPAPI(GetPlayPosition);
            IMPAPI(GetPlayPosition2);
            IMPAPI(GetPlayPositionEx);
            IMPAPI(GetPlayPosition2Ex);
            IMPAPI(GetPlayState);
            IMPAPI(GetPlayStateEx);
            IMPAPI(GetProjectPath);
            /*JFB commented: err in debug output "plugin_getapi fail:GetProjectStateChangeCount" - last check: v4.33rc1
             IMPAPI(GetProjectStateChangeCount);
             */
            IMPAPI(GetProjectTimeSignature2);
            IMPAPI(GetResourcePath);
            IMPAPI(GetSelectedMediaItem);
            IMPAPI(GetSelectedTrack);
            IMPAPI(GetSelectedTrackEnvelope);
            IMPAPI(GetSet_ArrangeView2);
            IMPAPI(GetSetEnvelopeState);
            IMPAPI(GetSetMediaItemInfo);
            IMPAPI(GetSetMediaItemTakeInfo);
            IMPAPI(GetSetMediaTrackInfo);
            IMPAPI(GetSetObjectState);
            IMPAPI(GetSetObjectState2);
            IMPAPI(GetSetRepeat);
            IMPAPI(GetTempoTimeSigMarker);
            IMPAPI(GetTakeEnvelopeByName);
            IMPAPI(GetTakeStretchMarker);
            IMPAPI(GetSetTrackSendInfo);
            IMPAPI(GetSetTrackState);
            IMPAPI(GetSet_LoopTimeRange);
            IMPAPI(GetSet_LoopTimeRange2);
            IMPAPI(GetSubProjectFromSource);
            IMPAPI(GetTake);
            IMPAPI(GetTakeNumStretchMarkers);
            IMPAPI(GetTCPFXParm);
            IMPAPI(GetToggleCommandState);
            IMPAPI(GetToggleCommandState2);
            IMPAPI(GetTrack);
            IMPAPI(GetTrackGUID);
            IMPAPI(GetTrackEnvelope);
            IMPAPI(GetTrackEnvelopeByName);
            IMPAPI(GetTrackInfo);
            IMPAPI(GetTrackMediaItem);
            IMPAPI(GetTrackMIDINoteNameEx);
            IMPAPI(GetTrackNumMediaItems);
            IMPAPI(GetTrackUIVolPan);
            IMPAPI(GetUserInputs);
            IMPAPI(get_config_var);
            IMPAPI(get_ini_file);
            IMPAPI(GR_SelectColor);
            IMPAPI(GSC_mainwnd);
            IMPAPI(guidToString);
            IMPAPI(Help_Set);
            IMPAPI(HiresPeaksFromSource);
            IMPAPI(InsertMedia);
            IMPAPI(InsertTrackAtIndex);
            IMPAPI(IsMediaExtension);
            IMPAPI(kbd_enumerateActions);
            IMPAPI(kbd_formatKeyName);
            IMPAPI(kbd_getCommandName);
            IMPAPI(kbd_getTextFromCmd);
            IMPAPI(KBD_OnMainActionEx);
            IMPAPI(kbd_reprocessMenu);
            IMPAPI(kbd_RunCommandThroughHooks);
            IMPAPI(kbd_translateAccelerator);
            IMPAPI(Main_OnCommand);
            IMPAPI(Main_OnCommandEx);
            IMPAPI(Main_openProject);
            IMPAPI(MainThread_LockTracks);
            IMPAPI(MainThread_UnlockTracks);
            IMPAPI(MarkProjectDirty);
            IMPAPI(MIDI_CountEvts);
            IMPAPI(MIDI_DeleteCC);
            IMPAPI(MIDI_DeleteEvt);
            IMPAPI(MIDI_DeleteNote);
            IMPAPI(MIDI_DeleteTextSysexEvt);
            IMPAPI(MIDI_EnumSelCC);
            IMPAPI(MIDI_EnumSelEvts);
            IMPAPI(MIDI_EnumSelNotes);
            IMPAPI(MIDI_EnumSelTextSysexEvts);
            IMPAPI(MIDI_eventlist_Create);
            IMPAPI(MIDI_eventlist_Destroy);
            IMPAPI(MIDI_GetCC);
            IMPAPI(MIDI_GetEvt);
            IMPAPI(MIDI_GetNote);
            IMPAPI(MIDI_GetPPQPosFromProjTime);
            IMPAPI(MIDI_GetProjTimeFromPPQPos);
            IMPAPI(MIDI_GetTextSysexEvt);
            IMPAPI(MIDI_InsertCC);
            IMPAPI(MIDI_InsertEvt);
            IMPAPI(MIDI_InsertNote);
            IMPAPI(MIDI_InsertTextSysexEvt);
            IMPAPI(MIDI_SetCC);
            IMPAPI(MIDI_SetEvt);
            IMPAPI(MIDI_SetNote);
            IMPAPI(MIDI_SetTextSysexEvt);
            IMPAPI(MIDIEditor_GetActive);
            IMPAPI(MIDIEditor_GetMode);
            IMPAPI(MIDIEditor_GetSetting_int);
            IMPAPI(MIDIEditor_GetTake);
            IMPAPI(MIDIEditor_LastFocused_OnCommand);
            IMPAPI(MIDIEditor_OnCommand);
            IMPAPI(mkpanstr);
            IMPAPI(mkvolpanstr);
            IMPAPI(mkvolstr);
            IMPAPI(MoveEditCursor);
            IMPAPI(MoveMediaItemToTrack);
            IMPAPI(OnPauseButton);
            IMPAPI(OnPlayButton);
            IMPAPI(OnStopButton);
            IMPAPI(NamedCommandLookup);
            IMPAPI(parse_timestr_len);
            IMPAPI(parse_timestr_pos);
            IMPAPI(PCM_Sink_CreateEx);
            IMPAPI(PCM_Source_CreateFromFile);
            IMPAPI(PCM_Source_CreateFromFileEx);
            IMPAPI(PCM_Source_CreateFromSimple);
            IMPAPI(PCM_Source_CreateFromType);
            IMPAPI(PCM_Source_GetSectionInfo);
            IMPAPI(PlayPreview);
            IMPAPI(PlayPreviewEx);
            IMPAPI(PlayTrackPreview);
            IMPAPI(PlayTrackPreview2Ex);
            IMPAPI(plugin_getFilterList);
            IMPAPI(plugin_getImportableProjectFilterList);
            IMPAPI(plugin_register);
            IMPAPI(PreventUIRefresh);
            IMPAPI(projectconfig_var_addr);
            IMPAPI(projectconfig_var_getoffs);
            IMPAPI(RefreshToolbar);
#ifdef _WIN32
            IMPAPI(RemoveXPStyle);
#endif
            IMPAPI(RenderFileSection);
            IMPAPI(Resampler_Create);
            IMPAPI(screenset_register);
            IMPAPI(screenset_registerNew);
            IMPAPI(screenset_unregister);
            IMPAPI(SectionFromUniqueID);
            IMPAPI(SelectProjectInstance);
            IMPAPI(SendLocalOscMessage);
            IMPAPI(SetActiveTake);
            IMPAPI(SetCurrentBPM);
            IMPAPI(SetEditCurPos);
            IMPAPI(SetEditCurPos2);
            IMPAPI(SetMediaItemInfo_Value);
            IMPAPI(SetMediaItemLength);
            IMPAPI(SetMediaItemPosition);
            IMPAPI(SetMediaItemTakeInfo_Value);
            IMPAPI(SetMediaTrackInfo_Value);
            IMPAPI(SetMixerScroll);
            IMPAPI(SetMouseModifier);
            IMPAPI(SetOnlyTrackSelected);
            IMPAPI(SetProjectMarkerByIndex);
            IMPAPI(SetProjectMarker);
            IMPAPI(SetProjectMarker2);
            IMPAPI(SetProjectMarker3);
            IMPAPI(SetTempoTimeSigMarker);
            IMPAPI(SetTakeStretchMarker);
            IMPAPI(SetTrackSelected);
            IMPAPI(ShowActionList);
            IMPAPI(ShowConsoleMsg);
            IMPAPI(ShowMessageBox);
            IMPAPI(SnapToGrid);
            IMPAPI(SplitMediaItem);
            IMPAPI(StopPreview);
            IMPAPI(StopTrackPreview);
            IMPAPI(stringToGuid);
            IMPAPI(TimeMap_GetDividedBpmAtTime);
            IMPAPI(TimeMap_GetTimeSigAtTime);
            IMPAPI(TimeMap_QNToTime);
            IMPAPI(TimeMap_timeToQN);
            IMPAPI(TimeMap2_beatsToTime);
            IMPAPI(TimeMap2_GetDividedBpmAtTime);
            IMPAPI(TimeMap2_GetNextChangeTime);
            IMPAPI(TimeMap2_QNToTime);
            IMPAPI(TimeMap2_timeToBeats);
            IMPAPI(TimeMap2_timeToQN);
            IMPAPI(TrackFX_FormatParamValue);
            IMPAPI(TrackFX_GetByName);
            IMPAPI(TrackFX_GetChainVisible);
            IMPAPI(TrackFX_GetEnabled);
            IMPAPI(TrackFX_GetFloatingWindow);
            IMPAPI(TrackFX_GetCount);
            IMPAPI(TrackFX_GetFXName);
            IMPAPI(TrackFX_GetFXGUID);
            IMPAPI(TrackFX_GetNumParams);
            IMPAPI(TrackFX_GetOpen);
            IMPAPI(TrackFX_GetParam);
            IMPAPI(TrackFX_GetParamName);
            IMPAPI(TrackFX_GetPreset);
            IMPAPI(TrackFX_GetPresetIndex);
            IMPAPI(TrackFX_NavigatePresets);
            IMPAPI(TrackFX_SetEnabled);
            IMPAPI(TrackFX_SetOpen);
            IMPAPI(TrackFX_SetParam);
            IMPAPI(TrackFX_SetPreset);
            IMPAPI(TrackFX_SetPresetByIndex);
            IMPAPI(TrackFX_Show);
            IMPAPI(TrackList_AdjustWindows);
            IMPAPI(Undo_BeginBlock);
            IMPAPI(Undo_BeginBlock2);
            IMPAPI(Undo_CanRedo2);
            IMPAPI(Undo_CanUndo2);
            IMPAPI(Undo_DoUndo2);
            IMPAPI(Undo_EndBlock);
            IMPAPI(Undo_EndBlock2);
            IMPAPI(Undo_OnStateChange);
            IMPAPI(Undo_OnStateChange_Item);
            IMPAPI(Undo_OnStateChange2);
            IMPAPI(Undo_OnStateChangeEx);
            IMPAPI(Undo_OnStateChangeEx2);
            IMPAPI(UpdateArrange);
            IMPAPI(UpdateItemInProject);
            IMPAPI(UpdateTimeline);
            IMPAPI(ValidatePtr);

            
            if (!rec->Register("hookcommand", (void*)hookCommandProc))
                return 0;
            
            g_hInst = hInstance;
            g_hwndParent = GetMainHwnd();
            
            // Call plugin specific init
            if (!UltraschallChaptersInit())
                return 0;
            if (!UltraschallShownotesInit())
                return 0;
            if (!UltraschallSoundboardInit())
                return 0;
            if(ultraschall::auphonic::Initialize() == 0)
            {
                return 0;
            }
            
            // our plugin registered, return success
            
            return 1;
        }
        else
        {
            return 0;
        }
    }
};

MediaTrack* getTrackByName(char* trackName)
{
    
    for (int i = 0; i < GetNumTracks(); ++i) {
        
        MediaTrack* track = GetTrack(0,i);
        
        char* currentTrackName = (char*)GetSetMediaTrackInfo(track, "P_NAME", NULL);
        
        if(strcmp(currentTrackName, trackName) == 0)
        {
            free(currentTrackName);
            return track;
        }
        
        
    }
    
    
    return NULL;
    
}

std::string _format_time(char* aTime)
{
    char * pch;
    pch = std::strtok(aTime, ":");
    std::string bbb = std::string(aTime);
    std::vector<std::string> items;
    
    while(pch != NULL)
    {
        items.push_back(pch);
        pch = std::strtok(NULL, ":");
    }
    
    
    std::reverse(items.begin(), items.end());
    
    struct time times;
    
    times.s =     times.m =     times.h = 0;
    
    times.ms = items[0].substr(items[0].find("."), std::string::npos).c_str();
    items[0].erase(items[0].find("."), std::string::npos);
    
    
    
    for (int i = 0; i < items.size(); ++i) {
        switch (i) {
            case 0:
                times.s = std::atoi(items[0].c_str());
                break;
            case 1:
                times.m = std::atoi(items[1].c_str());
                break;
            case 2:
                times.h = std::atoi(items[2].c_str());
                break;
                
            default:
                break;
        }
    }
    
    char buffer[13];
    
    sprintf(buffer, "%02d:%02d:%02d%s", times.h, times.m, times.s, times.ms.c_str());
    
    return std::string(buffer);
    
}
