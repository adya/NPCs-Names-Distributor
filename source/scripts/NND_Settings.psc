Scriptname NND_Settings extends MCM_ConfigBase  

import Debug
import Utility
import JsonUtil
import StringUtil
import PapyrusUtil

; Title displaying style. Possible values are:
; 0 - None: Agnar Stormheart
; 1 - New Line: Agnar Stormheart 
;                Whiterun Guard
; 2 - Hyphen: Agnar Stormheart - Whiterun Guard
; 3 - Round Brackets: Agnar Stormheart (Whiterun Guard)
; 4 - Square Brackets: Agnar Stormheart [Whiterun Guard]
; 5 - Comma: Agnar Stormheart, Whiterun Guard
; 6 - Semicolon: Agnar Stormheart; Whiterun Guard
; 7 - Full Stop: Agnar Stormheart. Whiterun Guard
; Defaults to 1 - New Line.
Int Property TitleStyle Auto

; Global that toggles mod on/off.
GlobalVariable Property NNDRenamingEnabled Auto

; Global that will make all NNDApplyName effects restart.
GlobalVariable Property NNDNeedsRefresh Auto

Bool Property ShouldRegenerateNames Auto

; Array that holds list of valid configs.
String[] validConfigs

Event OnConfigInit()
    parent.OnConfigInit()
    NNDNeedsRefresh.SetValueInt(0)
    ShouldRegenerateNames = false
    ValidateConfigs()
EndEvent

Event OnGameReload()
    parent.OnGameReload()
    ReloadConfigs()
EndEvent

Event OnSettingChange(string a_ID)
    parent.OnSettingChange(a_ID)
    If a_ID == "iRenamingEnabled:General"
        NNDRenamingEnabled.SetValueInt(GetModSettingInt(a_ID))
    ElseIf a_ID == "iTitleStyle:General"
        TitleStyle = GetModSettingInt(a_ID)
        RefreshNames()
    EndIf
EndEvent

Function RefreshNames()
    NNDNeedsRefresh.SetValueInt(1)
    Utility.Wait(100)
    NNDNeedsRefresh.SetValueInt(0)
EndFunction

Function RegenerateNames()
    Debug.Trace("Queueing names regeneration")
    ShouldRegenerateNames = true
    ReloadConfigs()
    RefreshNames()
    ShouldRegenerateNames = false
EndFunction

Function ReloadConfigs()
    String[] configs = JsonInFolder("../" + configsDirectory + "/")
    Int index = 0
    While index < configs.Length
        Unload(configs[index], false)
        index += 1
    EndWhile
    ValidateConfigs()
EndFunction

String configsDirectory = "NPCsNamesDistributor"

String Function ConfigFileForKeyword(String NNDKeyword)
    Return "../" + configsDirectory + "/" + NNDKeyword + ".json"
EndFunction


; Function that goes through all configs present in configs directory and validates them.
; Once they validated KeywordHasValidConfig can be used to check if config can be processed.
Function ValidateConfigs()
    String[] configs = JsonInFolder("../" + configsDirectory + "/")
    Trace("Found " + configs.Length + " configs for NND:")
    Int index = 0
    While index < configs.Length
        Trace(configs[index])
        index += 1
    EndWhile

    String NNDGivenKey = "NND_Given"
    String NNDFamilyKey = "NND_Family"

    Trace("Validating NND configs...")
    index = 0
    While index < configs.Length
        String config = "../" + configsDirectory + "/" + configs[index]
        If !IsGood(config)
            Trace(configs[index] + " is malformed.", 1)
            configs[index] = ""
        ElseIf !CanResolvePath(config, NNDGivenKey) && !CanResolvePath(config, NNDFamilyKey)
            Trace(configs[index] + " doesn't define any names.", 1)
            configs[index] = ""
        EndIf

        index += 1
    EndWhile

    validConfigs = PapyrusUtil.ClearEmpty(configs)
EndFunction

; Check whether config for given keyword is valid and can be used to generate.
Bool Function KeywordHasValidConfig(String NNDKeyword)
    String config = NNDKeyword + ".json"
    Return validConfigs.Find(config) >= 0
EndFunction