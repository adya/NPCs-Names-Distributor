Scriptname NND_Settings extends MCM_ConfigBase  

import Debug
import Utility
import JsonUtil
import StringUtil
import PapyrusUtil

;;; Public stuff ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; This is what is used by NND_ApplyName script

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
; 8 - Space: Agnar Stormheart Whiterun Guard
; Defaults to 1 - New Line.
Int Property TitleStyle
    Int Function Get()
        Return GetModSettingInt(sTitleStyleKey)
    EndFunction
    Function Set(Int iStyle)
        SetModSettingInt(sTitleStyleKey, iStyle)
    EndFunction
EndProperty

; Inverses position of name and title
; (e.g. "Agnar Stormheart - Whiterun Guard" becomes "Whiterun Guard - Agnar Stormheart")
Bool Property IsInvertedTitleStyle
    Bool Function Get()
        Return GetModSettingBool(sTitleInversionKey)
    EndFunction
    Function Set(Bool isInverted)
        SetModSettingBool(sTitleInversionKey, isInverted)
    EndFunction
EndProperty

; An arbitrary number used to identify current generation instance.
; Every time use clicks "Regenerate" action this id will increment, forcins all NNDApplyName effects to generate new names upon next refresh.
Int Property GenerationId
    Int Function Get()
        Return StorageUtil.GetIntValue(self, "GenerationId", 0)
    EndFunction
    Function Set(Int id)
        StorageUtil.SetIntValue(self, "GenerationId", id)
    EndFunction
EndProperty

; Checks whether config for given keyword is valid and can be used to generate.
Bool Function KeywordHasValidConfig(String NNDKeyword)
    String config = NNDKeyword + ".json"
    Return validConfigs.Find(config) >= 0
EndFunction

; Returns a path to JSON config file associated with specified NNDKeyword.
String Function ConfigFileForKeyword(String NNDKeyword)
    Return "../" + configsDirectory + "/" + NNDKeyword + ".json"
EndFunction

;;; Config Events ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Event OnConfigInit()
    parent.OnConfigInit()
    NNDNeedsRefresh.SetValueInt(0)
    ValidateConfigs()
EndEvent

Event OnGameReload()
    parent.OnGameReload()
    ReloadConfigs()
    RefreshNames()
EndEvent

Event OnConfigOpen()
    parent.OnConfigOpen()
    ConfigureTitleStyleOptions()
EndEvent

Event OnSettingChange(string a_ID)
    parent.OnSettingChange(a_ID)
    If a_ID == sTitleStyleKey
        RefreshNames()
    ElseIf a_ID == sTitleInversionKey
        SetMenuOptions(sTitleStyleKey, GetTitleStyleOptions(IsInvertedTitleStyle), GetTitleStyleShortNamesOptions())
    EndIf    
EndEvent

;;; Internal stuff ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Global that will make all NNDApplyName effects restart when toggled.
GlobalVariable Property NNDNeedsRefresh Auto

; Simple flag that hides "Regenerate" button once it was tapped,
; to avoid unnecessary calls and provide UI feedback that action was performed.
Bool Property bGenerationQueued Auto

; Array that holds a list of valid configs that were detected since last reload.
String[] validConfigs

String sTitleStyleKey = "iTitleStyle:General"
String sTitleInversionKey = "bTitleInvert:General"

; Default configs directory.
String configsDirectory = "NPCsNamesDistributor"

Function RefreshNames()
    If NNDNeedsRefresh.GetValueInt() == 1
        ; Refreshing was already queued and awaiting when menu will be closed.
        Return
    EndIf
    NNDNeedsRefresh.SetValueInt(1)
    ; Give some time for effects to wear off before reapplying them.
    Utility.Wait(1)
    NNDNeedsRefresh.SetValueInt(0)
EndFunction

; This action is linked to the entry in MCM config.
Function RegenerateNames()
    NNDTrace("Queueing names regeneration")
    
    bGenerationQueued = true
    RefreshMenu()
    
    GenerationId += 1
    ReloadConfigs()
    RefreshNames()
    
    bGenerationQueued = false
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

; Function that goes through all configs present in configs directory and validates them.
; Once they validated KeywordHasValidConfig can be used to check if config can be processed.
Function ValidateConfigs()
    String[] configs = JsonInFolder("../" + configsDirectory + "/")
    NNDTrace("Found " + configs.Length + " configs for NND:")
    Int index = 0
    While index < configs.Length
        NNDTrace(configs[index])
        index += 1
    EndWhile

    String NNDGivenKey = "NND_Given"
    String NNDFamilyKey = "NND_Family"

    NNDTrace("Validating NND configs...")
    index = 0
    While index < configs.Length
        String config = "../" + configsDirectory + "/" + configs[index]
        If !IsGood(config)
            NNDTrace(configs[index] + " is malformed.", 1)
            configs[index] = ""
        ElseIf !CanResolvePath(config, NNDGivenKey) && !CanResolvePath(config, NNDFamilyKey)
            NNDTrace(configs[index] + " doesn't define any names.", 1)
            configs[index] = ""
        EndIf

        index += 1
    EndWhile

    validConfigs = PapyrusUtil.ClearEmpty(configs)
EndFunction

; Sets correct Title Style options that correspond to currently inversion state.
Function ConfigureTitleStyleOptions()
    If IsInvertedTitleStyle
        SetMenuOptions(sTitleStyleKey, GetTitleStyleOptions(true), GetTitleStyleShortNamesOptions())
    Else
        SetMenuOptions(sTitleStyleKey, GetTitleStyleOptions(false), GetTitleStyleShortNamesOptions())
    EndIf
EndFunction

; Returns a list of title style options appropriate for inversion flag.
String[] Function GetTitleStyleOptions(Bool inverted = false)
    If inverted
        Return StringSplit("$TitleExampleInvertedNone,$TitleExampleInvertedNewLine,$TitleExampleInvertedHyphen,$TitleExampleInvertedRoundBrackets,$TitleExampleInvertedSquareBrackets,$TitleExampleInvertedComma,$TitleExampleInvertedSemicolon,$TitleExampleInvertedFullStop,$TitleExampleInvertedSpace")
    Else
        Return StringSplit("$TitleExampleNone,$TitleExampleNewLine,$TitleExampleHyphen,$TitleExampleRoundBrackets,$TitleExampleSquareBrackets,$TitleExampleComma,$TitleExampleSemicolon,$TitleExampleFullStop,$TitleExampleSpace")
    EndIf
EndFunction

; Returns a list of title style short names to display in the menu.
String[] Function GetTitleStyleShortNamesOptions()
    Return StringSplit("$TitleStyleNone,$TitleStyleNewLine,$TitleStyleHyphen,$TitleStyleRound,$TitleStyleSquare,$TitleStyleComma,$TitleStyleSemicolon,$TitleStyleFullStop,$TitleStylespace")
EndFunction

; Logs NNDTrace with a distincive prefix for easier reading through logs.
Function NNDTrace(String sMessage, Int level = 0)
    Trace("NNDSettings: " + sMessage + ".", level)
EndFunction
