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

    String[] keywords = GetNNDKeywords(akTarget.GetLeveledActorBase())
    If keywords.Length == 0
        ; No applicable Name Definitions, so we don't need to do anything.
        Return
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



;;; Keys used in Name Definition files. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

String NNDGivenKey = "NND_Given"
String NNDFamilyKey = "NND_Family"
String NNDConjunctionsKey = "NND_Conjunctions"
String NNDFemaleKey = "NND_Female"
String NNDMaleKey = "NND_Male"
String NNDAnyKey = "NND_Any"
String NNDBehaviorKey = "NND_Behavior"
String NNDCombineKey = "NND_Combine"

String NNDNamesKey = "NND_Names"
String NNDChanceKey = "NND_Chance"

String NNDPrefixKey = "NND_Prefix"
String NNDSuffixKey = "NND_Suffix"

; Finds a name for the actor in one of the valid Name Definition files based on NND Keywords applied to that actor.
; If Name couldn't be picked an empty string will be returned.
String Function PickNameFor(Actor person, String[] NNDKeywords)
    String givenName = ""
    String familyName = ""
    String conjunction = ""
    
    Bool allowCombineFamily = false
    Bool allowCombineGiven = false

    ; Go through all found keywords in the priority order
    ; until we were able to pick the name (at least one part of it).
    ; Having both Given and Family names empty is considered a failure of the Name Definition,
    ; so it will be skipped and the next one (if any) will attempt to get the name.
    Int index = 0

    ; The condition here might look a bit complex, but it is derived from the following table of possible states:
    ; G - Given, F - Family, B - Both;
    ; "skip" means that we should go to next definition to find missing name, hence we check conditions where skip is present.
    ;
    ;         | not combine G |   combine G   | not combine F |   combine F   |
    ; empty G |   use F only  | skip G, use F |   use F only  |  use F only   |
    ; empty F |   use G only  |  use G only   |   use G Only  | use G, skip F |
    ; empty B |    skip B     |    skip B     |    skip B     |    skip B     |

    While (givenName == "" && familyName == "") || (givenName == "" && allowCombineGiven) || (familyName == "" && allowCombineFamily) && index < NNDKeywords.Length
        String NNDKeyword = NNDKeywords[index]
        traceForKeyword = NNDKeyword
        String definition = NNDSettings.NameDefinitionFileForKeyword(NNDKeyword)
        
        ; If there is at least one more pending Name Definitions in queue
        ; then we check definition's behavior to see if it allows skipping and evaluate the chance of it being skipped.
        Bool skipDefinition = false
        If (index < NNDKeywords.Length - 1)
            skipDefinition = GetChanceForList(definition, CreateKeyPath(NNDBehaviorKey, NNDChanceKey)) <= RandomChance()
        EndIf   

        If !skipDefinition   
            String genderKey = Either(person.GetLeveledActorBase().GetSex() == 1, NNDFemaleKey, NNDMaleKey)
            
            allowCombineGiven = GetPathBoolValue(definition, CreateKeyPath(NNDGivenKey, NNDBehaviorKey, NNDCombineKey))
            allowCombineFamily = GetPathBoolValue(definition, CreateKeyPath(NNDFamilyKey, NNDBehaviorKey, NNDCombineKey))
            
            ; Since we allow combining multiple Name Definitions we should now check if name has been already picked before attempting to read another one.
            
            If givenName == ""
                givenName = GetNameForKey(NNDGivenKey, genderKey, definition)
            EndIf
            
            If familyName == ""
                familyName = GetNameForKey(NNDFamilyKey, genderKey, definition)
            EndIf
            
            String conjunctionsKeyPath = CreateKeyPath(NNDConjunctionsKey, genderKey)
            Int conjunctionsCount = NamesCountInList(definition, conjunctionsKeyPath)
            If conjunctionsCount <= 0
                conjunctionsKeyPath = CreateKeyPath(NNDConjunctionsKey, NNDAnyKey)
                conjunctionsCount = NamesCountInList(definition, conjunctionsKeyPath)
            EndIf
            
            If conjunctionsCount > 0
                Int conjunctionIndex = RandomIndex(conjunctionsCount)
                conjunction = GetPathStringValue(definition, CreateIndexPath(conjunctionsKeyPath, conjunctionIndex))
            Else
                ; Default conjunction is a simple whitespace :) 
                conjunction = " "
            EndIf
        Else
            NNDTrace("Name Definition was randomly skipped")
        EndIf
            
        index += 1
    EndWhile
    traceForKeyword = ""

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
; If definition does not define names for specified genderKey, this function will attempt to pick a name in default "Any" category.
;
; - Parameter nameKey: Either NNDGivenKey or NNDFamilyKey
; - Parameter genderKey: One of NNDMaleKey, NNDFemaleKey or NNDAnyKey
; - Parameter definition: Filename of the Name Definition to be used.
;
; Returns empty string if name wasn't picked.
String Function GetNameForKey(String nameKey, String genderKey, String definition)
    String result = ""
    
    String genderKeyPath = CreateKeyPath(nameKey, genderKey)
    String namesKeyPath = CreateKeyPath(genderKeyPath, NNDNamesKey)
    Int namesCount = NamesCountInList(definition, namesKeyPath)
    
    ; If definition for specific gender was not found or has no names in it try to find default "Any" definition
    If namesCount <= 0
        NNDTrace("No " + nameKey + " names were found specific for " + genderKey + " in " + definition + ". Falling back to " + NNDAnyKey)
        genderKeyPath = CreateKeyPath(nameKey, NNDAnyKey)
        namesKeyPath = CreateKeyPath(genderKeyPath, NNDNamesKey)
        namesCount = NamesCountInList(definition, namesKeyPath)
    EndIf
    
    If namesCount > 0
        Int chance = GetChanceForList(definition, CreateKeyPath(genderKeyPath, NNDChanceKey))
        
        If chance > RandomChance()
            Int index = RandomIndex(namesCount)
            result = GetPathStringValue(definition, CreateIndexPath(namesKeyPath, index))
        EndIf
        
        ; Process prefixes and suffixes only if the base name was picked.
        If result != ""
            result = GetNameDecorationForKey(NNDPrefixKey, nameKey, genderKey, definition) + result + GetNameDecorationForKey(NNDSuffixKey, nameKey, genderKey, definition)
        Else
            NNDTrace(nameKey + " name was randomly skipped")
        EndIf
    Else
        NNDTrace("Couldn't find appropriate names list in " + definition + ". Both " + CreateKeyPath(nameKey, genderKey, NNDNamesKey) + " and " + CreateKeyPath(nameKey, NNDAnyKey, NNDNamesKey) + " are missing or empty", 1)
    EndIf
    Return result
EndFunction

; Gets either prefix or suffix (depending on decorKey) for specified genderKeyPath.
;
; - Parameter decorKey: Either NNDPrefixKey or NNDSuffixKey
; - Parameter nameKey: Either NNDGivenKey or NNDFamilyKey
; - Parameter genderKey: One of NNDMaleKey, NNDFemaleKey or NNDAnyKey
; - Parameter definition: Filename of the Name Definition to be used.
;
; Returns empty string if prefix or suffix wasn't picked.
String Function GetNameDecorationForKey(String decorKey, String nameKey, String genderKey, String definition)
    String genderKeyPath = CreateKeyPath(nameKey, genderKey)
    String decorNamesKeyPath = CreateKeyPath(genderKeyPath, decorKey, NNDNamesKey)
    Int decorsCount = NamesCountInList(definition, decorNamesKeyPath)
    
    If decorsCount <= 0
        NNDTrace("No " + decorKey + " names were found specific for " + genderKey + ". Falling back to " + NNDAnyKey)
        genderKeyPath = CreateKeyPath(nameKey, NNDAnyKey)
        decorNamesKeyPath = CreateKeyPath(genderKeyPath, decorKey, NNDNamesKey)
        decorsCount = NamesCountInList(definition, decorNamesKeyPath)
    EndIf
    
    If decorsCount > 0
        Int decorChance = GetChanceForList(definition, CreateKeyPath(genderKeyPath, decorKey, NNDChanceKey))
        If decorChance > RandomChance()
            Int index = RandomIndex(decorsCount)
            Return GetPathStringValue(definition, CreateIndexPath(decorNamesKeyPath, index))
        Else
            NNDTrace(decorKey + " for " + genderKeyPath + " name was randomly skipped")
        EndIf
    EndIf
    
    Return ""
EndFunction

; Finds all NND keywords that are associated with valid Name Definitions.
; Returned array is sorted base on priorities defined for these keywords if there are more than one.
;
; Note that NND Keywords that are associated with invalid Name Definitions will be skipped.
;
; There are only 4 types of Keywords priorities that can be set. Here is the list:
; - NNDKeyword_Forced
; - NNDKeyword_Faction
; - NNDKeyword_Class
; - NNDKeyword_Race (_Race can be ommitted as it is the default)
String[] Function GetNNDKeywords(Form person)
    traceForKeyword = ""

    ; Create a placeholder for building up the queue.
    String[] keywordsQueue = CreateStringArray(4, "")
    
    Int kwLength = person.GetNumKeywords()
    Int index = 0
    While index < kwLength
        Keyword kw = person.GetNthKeyword(index)
        NNDTrace("Analyzing keyword " + kw)
        If kw != None
            String kwName = kw.GetString()
            traceForKeyword = kwName
            ; Find the first NND keyword that represents a category with appropriate names for the actor.
            If kw != NNDTitleless && Find(kwName, "NND") == 0
                
                Int forcedIndex = -1
                Int factionIndex = -1
                Int classIndex = -1
                Int raceIndex = -1
                
                If keywordsQueue[0] == ""
                    forcedIndex = Find(kwName, "_Forced")
                    NNDTrace("_Forced suffix index is " + forcedIndex)
                EndIf
                If keywordsQueue[1] == ""
                    factionIndex = Find(kwName, "_Faction")
                    NNDTrace("_Faction suffix index is " + factionIndex)
                EndIf
                If keywordsQueue[2] == ""
                    classIndex = Find(kwName, "_Class")
                    NNDTrace("_Class suffix index is " + classIndex)
                EndIf
                If keywordsQueue[3] == ""
                    raceIndex = Find(kwName, "_Race")
                    NNDTrace("_Race suffix index is " + raceIndex)
                    
                    ; If race not found use whole keyword as default race priority.
                    ; Other priorities will be handled before checking the race priority, so no conflict there.
                    If forcedIndex == -1 && factionIndex == -1 && classIndex == -1 && raceIndex == -1
                        raceIndex = 0
                        NNDTrace("Keyword doesn't have any supported priorities, it will be considered a Race keyword")
                    EndIf
                EndIf
                
                Int queueIndex = -1

                If forcedIndex != -1
                    queueIndex = 0
                    kwName = Substring(kwName, 0, forcedIndex)
                    NNDTrace("Setting Forced NNDKeyword")
                ElseIf factionIndex != -1
                    queueIndex = 1
                    kwName = Substring(kwName, 0, factionIndex)
                    NNDTrace("Setting Faction NNDKeyword")
                ElseIf classIndex != -1
                    queueIndex = 2
                    kwName = Substring(kwName, 0, classIndex)
                    NNDTrace("Setting Class NNDKeyword")
                ElseIf raceIndex != -1
                    queueIndex = 3
                    kwName = Substring(kwName, 0, raceIndex)
                    NNDTrace("Setting Race NNDKeyword")
                ElseIf keywordsQueue[0] != "" && keywordsQueue[1] != "" && keywordsQueue[2] != "" && keywordsQueue[3] != ""
                    NNDTrace("All supported overwrites found: " + keywordsQueue)
                    
                    ; If all all spots in the queue have been set there is no need to continue the search.
                    Return keywordsQueue
                EndIf  
                
                If !NNDSettings.KeywordHasValidDefinition(kwName)
                    NNDTrace("Name Definition " + kwName + ".json is either missing or malformed. Check that it is present in Data/SKSE/Plugins/NPCsNamesDistributor/ and contains a valid JSON", 2)
                ElseIf queueIndex != -1
                    keywordsQueue[queueIndex] = kwName
                Else
                    NNDTrace("Another NNDKeyword with the same priority is already added to queue. " + kwName + " will be skipped.", 1)
                EndIf
            EndIf
        EndIf
        index += 1
    EndWhile

    traceForKeyword = ""
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
Int Function GetChanceForList(String definition, String keyPath)
    Return Clamp(GetPathIntValue(definition, keyPath, 100), 0, 100)
EndFunction

; Reads count of entries in the array located at specified keyPath.
Int Function NamesCountInList(String definition, String keyPath)
    Int count = PathCount(definition, keyPath)
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

; Assign a keyword that is currently being processed to this property,
; so that NNDTrace would add it as a tag for easier understanding.
;
; Remember to clear its value as soon as you're done with keywords, so NNDTrace wouldn't tag irrelevant messages.
String traceForKeyword = ""

; Logs trace with a distincive prefix for easier reading through logs.
; Severity is one of the following:
; 0 - Info
; 1 - Warning
; 2 - Error
Function NNDTrace(String sMessage, Int level = 0)
    String msg = "NNDApplyName: "
    If level == 1
        msg += "[WARNING] "
    ElseIf level == 2
        msg += "[ERROR] "
    Endif

    If originalName != ""
        msg += "[" + originalName + "] "
    EndIf
    If traceForKeyword != ""
        msg += "[" + traceForKeyword + "] "
    EndIf
    msg += sMessage + "."
    Trace(msg, level)
EndFunction