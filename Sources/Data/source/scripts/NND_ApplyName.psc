Scriptname NND_ApplyName extends activemagiceffect  

Keyword Property NNDTitleless Auto

NND_Settings Property NNDSettings Auto

import Debug
import Utility
import JsonUtil
import StringUtil
import PapyrusUtil

;Store the original name of the NPCs that it had before renaming, so renaming could be disabled.
String originalName = ""

; Store generated name of the NPCs, so it can be re-enabled later.
String generatedName = ""

; Id of the last generation instance.
; When this value mismatches the one from NNDSettings, a new name will be generated regardless.
Int lastGeneratationId = -1

Event OnEffectStart(Actor akTarget, Actor akCaster)
    parent.OnEffectStart(akTarget, akCaster)
    If akTarget == None
        NNDTrace("Failed to rename an actor. akTarget is None", 2)
        Return
    EndIf

    String[] keywords = GetNNDKeywords(akTarget.GetLeveledActorBase())
    If keywords.Length == 0
        ; No applicable configs, so we don't need to do anything.
        Return
    EndIf
    
    ; Read previous values if any.
    originalName = StorageUtil.GetStringValue(akTarget, "NNDOriginalName")
    generatedName = StorageUtil.GetStringValue(akTarget, "NNDGeneratedName")
    lastGeneratationId = StorageUtil.GetIntValue(akTarget, "NNDGenerationId", -1)
    
    ; NNDTrace("Effect starting... Original name for actor " + akTarget + ": " + originalName)
    ; NNDTrace("Effect starting... Generated name for actor " + akTarget + ": " + generatedName)
    ; Pick original name only once in an effect lifetime.
    If originalName == "" && lastGeneratationId == -1
        originalName = akTarget.GetDisplayName()
        StorageUtil.SetStringValue(akTarget, "NNDOriginalName", originalName)
    EndIf
    
    ; If id is -1, then this effect is attempting to generate the name for the first time,
    ; so we should assign current GenerationId to make it up-to-date.
    If lastGeneratationId == -1
        lastGeneratationId = NNDSettings.GenerationId
        StorageUtil.SetIntValue(akTarget, "NNDGenerationId", lastGeneratationId)
    EndIf
    
    If generatedName == "" || lastGeneratationId != NNDSettings.GenerationId
        generatedName = PickNameFor(akTarget, keywords)
        StorageUtil.SetStringValue(akTarget, "NNDGeneratedName", generatedName)
    EndIf
    
    String displayedName = generatedName
    If !akTarget.GetLeveledActorBase().HasKeyword(NNDTitleless)
        displayedName = FormattedTitle(displayedName, originalName)
    EndIf
    
    If generatedName != "" && akTarget.SetDisplayName(displayedName, true)
        ; Update lastGenerationId only if it was successful, as otherwise the old name will be kept.
        lastGeneratationId = NNDSettings.GenerationId
        StorageUtil.SetIntValue(akTarget, "NNDGenerationId", lastGeneratationId)
        NNDTrace("Renaming " + originalName + " => " + displayedName) 
    Else
        NNDTrace("Failed to pick a name for actor " + akTarget + " (" + akTarget.GetDisplayName() + "). Falling back to original name", 1)
        If originalName != ""
            akTarget.SetDisplayName(originalName, true)
        EndIf
    EndIf
EndEvent

Event OnEffectFinish(Actor akTarget, Actor akCaster)
    
    If !isLoaded
        ; NNDTrace("Effect finishing... Tried to restore name original name, but effect was already unloaded along with actor", 1)
        ; Prevent any renamings in cases when OnEffectFinish fired from unloaing
        ; See notes in https://www.creationkit.com/index.php?title=OnEffectFinish_-_ActiveMagicEffect
        Return
    EndIf
    
    If generatedName != "" && originalName != "" && akTarget != None
        ; NNDTrace("Effect finishing... Trying to restore name of " + akTarget + " to " + originalName)
        akTarget.SetDisplayName(originalName, true)
    EndIf
    parent.OnEffectFinish(akTarget, akCaster)
EndEvent

Bool isLoaded = true

Event OnCellAttach()
    parent.OnCellAttach()
    ; NNDTrace("Effect attached")
    isLoaded = true
EndEvent

Event OnCellDetach()
    isLoaded = false
    ; NNDTrace("Effect detached")
    parent.OnCellDetach()
EndEvent

Event OnLoad()
    parent.OnLoad()
    ; NNDTrace("Effect loaded")
    isLoaded = true
EndEvent

Event OnUnload()
    isLoaded = false
    ; NNDTrace("Effect unloaded")
    parent.OnUnload()
EndEvent

;;; Below is where all the magic happens ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;; Keys used in config files. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

String NNDGivenKey = "NND_Given"
String NNDFamilyKey = "NND_Family"
String NNDConjunctionsKey = "NND_Conjunctions"
String NNDFemaleKey = "NND_Female"
String NNDMaleKey = "NND_Male"
String NNDAnyKey = "NND_Any"

String NNDNamesKey = "NND_Names"
String NNDChanceKey = "NND_Chance"

String NNDPrefixKey = "NND_Prefix"
String NNDSuffixKey = "NND_Suffix"

; Finds a name for the actor in one of defined NND config files based on NND Keywords applied to that actor.
; If Name couldn't be picked an empty string will be returned.
String Function PickNameFor(Actor person, String[] NNDKeywords)
    String givenName = ""
    String familyName = ""
    String conjunction = ""
    
    ; Go through all found keywords in the priority order
    ; until we were able to pick the name (at least one part of it).
    ; Having both Given and Family names empty is considered a failure of the config file,
    ; so it will be skipped and the next one (if any) will attempt to get the name.
    Int index = 0
    While givenName == "" && familyName == "" && index < NNDKeywords.Length
        String NNDKeyword = NNDKeywords[index]
        If NNDSettings.KeywordHasValidConfig(NNDKeyword)
            String config = NNDSettings.ConfigFileForKeyword(NNDKeyword)
            String genderKey = Either(person.GetLeveledActorBase().GetSex() == 1, NNDFemaleKey, NNDMaleKey)
            
            givenName = GetNameForKey(NNDGivenKey, genderKey, config)
            familyName = GetNameForKey(NNDFamilyKey, genderKey, config)
            
            String conjunctionsKeyPath = CreateKeyPath(NNDConjunctionsKey, genderKey)
            Int conjunctionsCount = NamesCountInList(config, conjunctionsKeyPath)
            If conjunctionsCount <= 0
                conjunctionsKeyPath = CreateKeyPath(NNDConjunctionsKey, NNDAnyKey)
                conjunctionsCount = NamesCountInList(config, conjunctionsKeyPath)
            EndIf
            
            If conjunctionsCount > 0
                Int conjunctionIndex = RandomIndex(conjunctionsCount)
                conjunction = GetPathStringValue(config, CreateIndexPath(conjunctionsKeyPath, conjunctionIndex))
            Else
                ; Default conjunction is a simple whitespace :) 
                conjunction = " "
            EndIf
        Else
            NNDTrace("Config " + NNDKeyword + ".json is either missing or malformed. Check that it is present in Data/SKSE/Plugins/NPCsNamesDistributor/ and contains a valid JSON")
        EndIf
        index += 1
    EndWhile
    
    ; Build a full name, connecting given and family names only if both were picked.
    If givenName != ""
        If familyName != ""
            Return givenName + conjunction + familyName  
        Else
            Return givenName
        EndIf
    Else ; Regardles of whether family is picked or not, return it as a final result.
        Return familyName
    Endif
EndFunction

; Applies title style according to selected options in NNDSettings.
String Function FormattedTitle(String name, String title)
    If NNDSettings.IsInvertedTitleStyle
        String tmp = name
        name = title
        title = tmp
    EndIf
    If NNDSettings.TitleStyle == 1
        Return name + "\n" + title
    ElseIf NNDSettings.TitleStyle == 2
        Return name + " - " + title
    ElseIf NNDSettings.TitleStyle == 3
        Return name + " (" + title + ")"
    ElseIf NNDSettings.TitleStyle == 4
        Return name + " [" + title + "] "
    ElseIf NNDSettings.TitleStyle == 5
        Return name + ", " + title
    ElseIf NNDSettings.TitleStyle == 6
        Return name + "; " + title
    ElseIf NNDSettings.TitleStyle == 7
        Return name + ". " + title
    Else
        Return name
    EndIf
EndFunction

; Gets either Given or Family name (depending on nameKey) for specified gender.
; If config does not define names for specified genderKey, this function will attempt to pick a name in default "Any" category.
;
; - Parameter nameKey: Either NNDGivenKey or NNDFamilyKey
; - Parameter genderKey: One of NNDMaleKey, NNDFemaleKey or NNDAnyKey
; - Parameter config: Filename of the config
;
; Returns empty string if name wasn't picked.
String Function GetNameForKey(String nameKey, String genderKey, String config)
    String result = ""
    
    String genderKeyPath = CreateKeyPath(nameKey, genderKey)
    String namesKeyPath = CreateKeyPath(genderKeyPath, NNDNamesKey)
    Int namesCount = NamesCountInList(config, namesKeyPath)
    
    ; If definition for specific gender was not found or has no names in it try to find default "Any" definition
    If namesCount <= 0
        NNDTrace("No " + nameKey + " names were found specific for " + genderKey + " in " + config + ". Falling back to " + NNDAnyKey)
        genderKeyPath = CreateKeyPath(nameKey, NNDAnyKey)
        namesKeyPath = CreateKeyPath(genderKeyPath, NNDNamesKey)
        namesCount = NamesCountInList(config, namesKeyPath)
    EndIf
    
    If namesCount > 0
        Int chance = GetChanceForList(config, CreateKeyPath(genderKeyPath, NNDChanceKey))
        
        If chance > RandomChance()
            Int index = RandomIndex(namesCount)
            result = GetPathStringValue(config, CreateIndexPath(namesKeyPath, index))
        EndIf
        
        ; Process prefixes and suffixes only if the base name was picked.
        If result != ""
            result = GetNameDecorationForKey(NNDPrefixKey, nameKey, genderKey, config) + result + GetNameDecorationForKey(NNDSuffixKey, nameKey, genderKey, config)
        Else
            NNDTrace(nameKey + " name was randomly skipped")
        EndIf
    Else
        NNDTrace("Couldn't find appropriate names list in " + config + ". Both " + CreateKeyPath(nameKey, genderKey, NNDNamesKey) + " and " + CreateKeyPath(nameKey, NNDAnyKey, NNDNamesKey) + " are missing or empty", 1)
    EndIf
    Return result
EndFunction

; Gets either prefix or suffix (depending on decorKey) for specified genderKeyPath.
;
; - Parameter decorKey: Either NNDPrefixKey or NNDSuffixKey
; - Parameter nameKey: Either NNDGivenKey or NNDFamilyKey
; - Parameter genderKey: One of NNDMaleKey, NNDFemaleKey or NNDAnyKey
; - Parameter config: Filename of the config
;
; Returns empty string if prefix or suffix wasn't picked.
String Function GetNameDecorationForKey(String decorKey, String nameKey, String genderKey, String config)
    String genderKeyPath = CreateKeyPath(nameKey, genderKey)
    String decorNamesKeyPath = CreateKeyPath(genderKeyPath, decorKey, NNDNamesKey)
    Int decorsCount = NamesCountInList(config, decorNamesKeyPath)
    
    If decorsCount <= 0
        NNDTrace("No " + decorKey + " names were found specific for " + genderKey + ". Falling back to " + NNDAnyKey)
        genderKeyPath = CreateKeyPath(nameKey, NNDAnyKey)
        decorNamesKeyPath = CreateKeyPath(genderKeyPath, decorKey, NNDNamesKey)
        decorsCount = NamesCountInList(config, decorNamesKeyPath)
    EndIf
    
    If decorsCount > 0
        Int decorChance = GetChanceForList(config, CreateKeyPath(genderKeyPath, decorKey, NNDChanceKey))
        If decorChance > RandomChance()
            Int index = RandomIndex(decorsCount)
            Return GetPathStringValue(config, CreateIndexPath(decorNamesKeyPath, index))
        Else
            NNDTrace(decorKey + " for " + genderKeyPath + " name was randomly skipped")
        EndIf
    EndIf
    
    Return ""
EndFunction

; Finds all NND keywords that represent name categories.
; Returned array is sorted base on priorities of these categories if there are more than one.
;
; There are only 4 types of Keywords priorities that can be set. Here is the list:
; - NNDKeyword_Forced
; - NNDKeyword_Faction
; - NNDKeyword_Class
; - NNDKeyword_Race (_Race can be ommitted as it is the default)
String[] Function GetNNDKeywords(Form person)
    
    ; Create a placeholder for building up the queue.
    String[] keywordsQueue = CreateStringArray(4, "")
    
    Int kwLength = person.GetNumKeywords()
    Int index = 0
    While index < kwLength
        Keyword kw = person.GetNthKeyword(index)
        ; NNDTrace("Analyzing keyword " + kw)
        If kw != None
            String kwName = kw.GetString()
            ; Find the first NND keyword that represents a category with appropriate names for the actor.
            If kw != NNDTitleless && Find(kwName, "NND") == 0
                
                ; NNDTrace(kwName + " is a configuration keyword")
                Int forcedIndex = -1
                Int factionIndex = -1
                Int classIndex = -1
                Int raceIndex = -1
                
                If keywordsQueue[0] == ""
                    forcedIndex = Find(kwName, "_Forced")
                    ; NNDTrace("_Forced suffix index is " + forcedIndex)
                EndIf
                If keywordsQueue[1] == ""
                    factionIndex = Find(kwName, "_Faction")
                    ; NNDTrace("_Faction suffix index is " + factionIndex)
                EndIf
                If keywordsQueue[2] == ""
                    classIndex = Find(kwName, "_Class")
                    ; NNDTrace("_Class suffix index is " + classIndex)
                EndIf
                If keywordsQueue[3] == ""
                    raceIndex = Find(kwName, "_Race")
                    ; NNDTrace("_Race suffix index is " + raceIndex)
                    
                    ; If race not found use whole keyword as default race priority.
                    ; Other priorities will be handled before checking the race priority, so no conflict there.
                    If forcedIndex == -1 && factionIndex == -1 && classIndex == -1 && raceIndex == -1
                        raceIndex = 0
                        ; NNDTrace("Keyword " + kwName + " doesn't have any supported suffixes, it will be considered a Race keyword")
                    EndIf
                EndIf
                
                If forcedIndex != -1
                    keywordsQueue[0] = Substring(kwName, 0, forcedIndex)
                    ; NNDTrace("Setting Forced NNDKeyword" + keywordsQueue[0])
                ElseIf factionIndex != -1
                    keywordsQueue[1] = Substring(kwName, 0, factionIndex) 
                    ; NNDTrace("Setting Faction NNDKeyword" + keywordsQueue[1])
                ElseIf classIndex != -1
                    keywordsQueue[2] = Substring(kwName, 0, classIndex) 
                    ; NNDTrace("Setting Class NNDKeyword" + keywordsQueue[2])
                ElseIf raceIndex != -1
                    keywordsQueue[3] = Substring(kwName, 0, raceIndex)
                    ; NNDTrace("Setting Race NNDKeyword" + keywordsQueue[3])
                ElseIf keywordsQueue[0] != "" && keywordsQueue[1] != "" && keywordsQueue[2] != "" && keywordsQueue[3] != ""
                    ; NNDTrace("All supported overwrites found: " + keywordsQueue)
                    
                    ; If all all spots in the queue have been set there is no need to continue the search.
                    Return keywordsQueue
                EndIf            
            EndIf
        EndIf
        index += 1
    EndWhile
    
    ; Return only unique keywords that are present in the queue, so
    ; ["NNDKeyword1", "", "NNDKeyword3", "NNDKeyword1"] will become ["NNDKeyword1", "NNDKeyword3"]
    Return RemoveDupeString(ClearEmpty(keywordsQueue))
EndFunction

;;; Utility functions used to make life easier while writing this sciprt.

; Creates a JSON path using given keys.
; First key is considered to be the root's key and will be prepended with "." if it's not already. (e.g. ".key1.key2.key3.key4")
; Empty keys are ignored.
String Function CreateKeyPath(String key1, String key2, String key3 = "", String key4 = "")
    String result = key1
    If key2 != ""
        result += "." + key2
    EndIf
    If key3 != ""
        result += "." + key3
    EndIf
    If key4 != ""
        result += "." + key4
    EndIf
    Return result
EndFunction

; Creates an indexed JSON path (e.g. "my.key.path[0]").
String Function CreateIndexPath(String keyPath, Int index)
    Return keyPath + "[" + index + "]"
EndFunction

; Reads Chance value at specified keyPath and automatically sanitizes invalid values, by enforcing 0-100 boundaries.
Int Function GetChanceForList(String config, String keyPath)
    Return Clamp(GetPathIntValue(config, keyPath, 100), 0, 100)
EndFunction

; Reads count of entries in the array located at specified keyPath.
Int Function NamesCountInList(String config, String keyPath)
    Int count = PathCount(config, keyPath)
    If count > 0
        Return count
    Else
        Return 0
    EndIf
EndFunction

; Returns either the first or second string based on evaluated condition.
String Function Either(Bool condition, String first, String second)
    If condition
        Return first
    Else
        Return second
    EndIf
EndFunction

; Returns value if it is within [min; max] range, otherwise the nearest boundary value is returned.
Int Function Clamp(Int value, Int min = 0, Int max = 100)
    If value < min
        Return min
    ElseIf value > max
        Return max
    Else
        Return value
    EndIf
EndFunction

; Returns values within [0; 99] range to ensure that boundary chances gurantee the result.
; So that 0 chance never passes the check, and 100 will always pass.
Int Function RandomChance()
    Return RandomInt(0, 99)
EndFunction

; Returns an index randomly picked for an array of given size.
Int Function RandomIndex(Int size)
    Return RandomInt(0, size - 1)
EndFunction

; Logs trace with a distincive prefix for easier reading through logs.
Function NNDTrace(String sMessage, Int level = 0)
    Trace("NNDApplyName: " + sMessage + ".", level)
EndFunction