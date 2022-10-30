Scriptname NND_RenamingEffect extends activemagiceffect  

; I used to be an adventurer like you, then I took an arrow to the knee.

import Debug
import Utility
import JsonUtil
import StringUtil
import PapyrusUtil

FormList Property NNDSystemKeywords Auto

NND_Settings Property NNDSettings Auto

;Stores the original name of the NPCs that it had before renaming, so renaming could be disabled.
String _originalName = ""

; Stores generated name of the NPCs, so it can be re-enabled later.
String _generatedName = ""

; Id of the last generation instance.
; When this value mismatches the one from NNDSettings, a new name will be generated regardless.
Int _lastGeneratationId = -1

; Keys used for persistency.
String originalNameKey = "NNDOriginalName"
String generatedNameKey = "NNDGeneratedName"
String generationIdKey = "NNDGenerationId"

; Function that supplies custom key names for persistent data.
; Call it within OnInit event of a subclass to provide custom keys before they'll be used.
Function InitKeys(String originalName = "", String generatedName = "", String generationId = "")
    If originalName != ""
        originalNameKey = originalName
    EndIf
    If generatedName != ""
        generatedNameKey = generatedName
    EndIf
    If generationId != ""
        generationIdKey = generationId
    EndIf
EndFunction

;;;;; Overridable stuff
;     vvvvvvvvvvvvvvvvv

; Function that reports whether NNDKeywords required to be present
; in order for renaming to happen.
; Defaults to `true`.
;
; When `NeedsKeywords` function is overriden and returns `false`,
; it is assumed that `DecorateName` will be overriden as well and will return a default name to be applied.
Bool Function NeedsKeywords(Actor akTarget)
    Return true
EndFunction

; Utility function that provides more accurate logging by asking subclass its name.
String Function RenamingScriptName()
    Return "NNDRenamingEffect"
EndFunction

; Decorates newly generated name. Default implementation does nothing.
; Subclasses might override to provide customizations.
; Note: If `DecorateName` returns empty string then name is considered discarded and renaming terminates.
String Function DecorateName(Actor akTraget, String generatedName, String originalName)
    Return generatedName
EndFunction

; Determines whether OnEffectFinish should revert display name to the original one.
; This function should be overriden in subclasses to provide custom logic.
; Note: Call Parent to include common logic.
Bool Function ShouldRevertOnFinish(Actor akTarget)
    Return akTarget.GetDisplayName() != _originalName
EndFunction

; Proxy function that is called OnEffectStart to determine what name should be generated.
; If it fails, then no name will be applied.
;
; Default implementation simply calls PickNameFor, but subclasses may provide custom functionality to pick a name.
String Function GenerateName(Actor akTarget, String[] keywords)
    Return PickNameFor(akTarget, keywords)
EndFunction

;     ^^^^^^^^^^^^^^^^^
;;;;; Overridable stuff

Event OnEffectStart(Actor akTarget, Actor akCaster)
    If akTarget == None || akTarget.GetLeveledActorBase() == None
        NNDTrace("Failed to rename an actor. akTarget is None", 2)
        Return
    EndIf

    traceForName = _originalName

    ; Read previous values if any.
    _originalName = StorageUtil.GetStringValue(akTarget, originalNameKey)
    _generatedName = StorageUtil.GetStringValue(akTarget, generatedNameKey)
    _lastGeneratationId = StorageUtil.GetIntValue(akTarget, generationIdKey, -1)
    
    NNDTrace("Effect starting... Original name for actor " + akTarget + ": " + _originalName)
    NNDTrace("Effect starting... Generated name for actor " + akTarget + ": " + _generatedName)
   
    ; Pick original name only once in an effect lifetime.
    If _originalName == "" && _lastGeneratationId == -1
        _originalName = akTarget.GetDisplayName()
        StorageUtil.SetStringValue(akTarget, originalNameKey, _originalName)
    EndIf
    
    ; If id is -1, then this effect is attempting to generate the name for the first time,
    ; so we should assign current GenerationId to make it up-to-date.
    If _lastGeneratationId == -1
        _lastGeneratationId = NNDSettings.GenerationId
        StorageUtil.SetIntValue(akTarget, generationIdKey, _lastGeneratationId)
    EndIf

    String[] keywords = GetNNDKeywords(akTarget.GetLeveledActorBase())
    Bool hasKeywords = keywords.Length > 0

    If hasKeywords
        If _generatedName == "" || _lastGeneratationId != NNDSettings.GenerationId
            _generatedName = GenerateName(akTarget, keywords)
            StorageUtil.SetStringValue(akTarget, generatedNameKey, _generatedName)
        EndIf
    ElseIf NeedsKeywords(akTarget)
        ; No applicable Name Definitions and we need them, so we don't do anything.
        Return
    EndIf

    String displayedName = DecorateName(akTarget, _generatedName, _originalName)

    If displayedName == ""
        NNDTrace("Failed to pick a name for actor " + akTarget + " (" + akTarget.GetDisplayName() + ")", 1)
        Return
    EndIf

    ; Avoid trying to set display name if it is the same as what's already displayed. This will prevent capitalization issues.
    If displayedName == akTarget.GetDisplayName()
        NNDTrace("Actor already has the same display name '" + displayedName + "': " + akTarget + " (" + akTarget.GetDisplayName() + ")", 1)
        Return
    EndIf

    If akTarget.SetDisplayName(displayedName, false)
        ; Update lastGenerationId only if it was successful, as otherwise the old name will be kept.
        _lastGeneratationId = NNDSettings.GenerationId
        StorageUtil.SetIntValue(akTarget, generationIdKey, _lastGeneratationId)
        NNDTrace("Renaming " + _originalName + " => " + displayedName) 
    Else
        NNDTrace("Failed to rename an actor " + akTarget + " (" + akTarget.GetDisplayName() + "). Falling back to original name", 1)
        If _originalName != "" && _originalName != akTarget.GetDisplayName()
            akTarget.SetDisplayName(_originalName, false)
            NNDTrace("Reverting " + akTarget.GetDisplayName() + " => " + displayedName)
        Else
            NNDTrace("Couldn't revert original name for actor " + akTarget + " (" + akTarget.GetDisplayName() + ")!", 2)
        EndIf
    EndIf
EndEvent

; Let me guess… someone stole your sweetroll?

Event OnEffectFinish(Actor akTarget, Actor akCaster)
    If !isLoaded
        ; NNDTrace("Effect finishing... Tried to restore name original name, but effect was already unloaded along with actor", 1)
        ; Prevent any renamings in cases when OnEffectFinish fired from unloaing
        ; See notes in https://www.creationkit.com/index.php?title=OnEffectFinish_-_ActiveMagicEffect
        Return
    EndIf
    
    If _originalName != "" && akTarget != None
        ; Nested If to avoid calling ShouldRevertOnFinish with None akTarget.
        If ShouldRevertOnFinish(akTarget)
            NNDTrace("Effect finishing... Trying to restore name of " + akTarget + " to " + _originalName)
            akTarget.SetDisplayName(_originalName, false)
        EndIf
    EndIf
    parent.OnEffectFinish(akTarget, akCaster)
EndEvent

;;; Detecting loaded state ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; See OnEffectFinish for details.

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

;;; Below is where all the magic happens ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;; Keys used in Name Definition files. ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

String NNDGivenKey = "NND_Given"
String NNDFamilyKey = "NND_Family"
String NNDConjunctionsKey = "NND_Conjunctions"
String NNDFemaleKey = "NND_Female"
String NNDMaleKey = "NND_Male"
String NNDAnyKey = "NND_Any"
String NNDBehaviorKey = "NND_Behavior"
String NNDCombineKey = "NND_Combine"
String NNDObscuredKey = "NND_Obscuring"

String NNDNamesKey = "NND_Names"
String NNDChanceKey = "NND_Chance"

String NNDPrefixKey = "NND_Prefix"
String NNDSuffixKey = "NND_Suffix"

; Finds a name for the actor in one of the valid Name Definition files based on NND Keywords applied to that actor.
; If Name couldn't be picked an empty string will be returned.
; - Parameter      person: A person for whom the name will be picked. Used mostly to figure out appropriate sex.
; - Parameter NNSKeywords: An array of Name Definitions to be considered when picking the name.
; - Parameter    obscured: Flag indicating whether to pick names only from Obscured Name Definitions. 
;                          Defaults to `false`, which will pick names only from regular Definitions.
String Function PickNameFor(Actor person, String[] NNDKeywords, Bool obscured = false)
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

    While ((givenName == "" && familyName == "") || (givenName == "" && allowCombineGiven) || (familyName == "" && allowCombineFamily)) && index < NNDKeywords.Length
        String NNDKeyword = NNDKeywords[index]
        traceForKeyword = NNDKeyword
        String definition = NNDSettings.NameDefinitionFileForKeyword(NNDKeyword)
        
        ; Ensure that we're using obscured Name Definitions only when obscured name is requested.
        Bool isObscuring = GetPathBoolValue(definition, CreateKeyPath(NNDBehaviorKey, NNDObscuredKey))

        If isObscuring == obscured
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

; Sorry lass, I’ve got important things to do. We’ll speak another time.

; Gets either Given or Family name (depending on nameKey) for specified gender.
; If definition does not define names for specified genderKey, this function will attempt to pick a name in default "Any" category.
;
; - Parameter    nameKey: Either NNDGivenKey or NNDFamilyKey
; - Parameter  genderKey: One of NNDMaleKey, NNDFemaleKey or NNDAnyKey
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
; - Parameter   decorKey: Either NNDPrefixKey or NNDSuffixKey
; - Parameter    nameKey: Either NNDGivenKey or NNDFamilyKey
; - Parameter  genderKey: One of NNDMaleKey, NNDFemaleKey or NNDAnyKey
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

; I've been huntin' and fishin' in these parts for years.

; Finds all NND keywords that are associated with valid Name Definitions.
; Returned array is sorted base on priorities defined for these keywords if there are more than one.
; Within each priority group keywords are sorted alphabetically.
;
; Note that NND Keywords that are associated with invalid Name Definitions will be skipped.
;
; There are only 4 types of Keywords priorities that can be set. Here is the list:
; - NNDKeyword_Forced
; - NNDKeyword_Faction
; - NNDKeyword_Class
; - NNDKeyword_Race (_Race can be ommitted as it is the default)
String[] Function GetNNDKeywords(Form person)
    Int kwLength = person.GetNumKeywords()

    If kwLength == 0
        Return CreateStringArray(0)
    EndIf

    ; Create placeholders for building up the queue for each priority.
    String[] forcedKeywordsQueue = CreateStringArray(kwLength, "")
    String[] factionKeywordsQueue = CreateStringArray(kwLength, "")
    String[] classKeywordsQueue = CreateStringArray(kwLength, "")
    String[] raceKeywordsQueue = CreateStringArray(kwLength, "")
 
    Int index = 0
    While index < kwLength
        traceForKeyword = ""
        Keyword kw = person.GetNthKeyword(index)
        NNDTrace("Analyzing keyword " + kw)
        If kw != None
            String kwName = kw.GetString()
            traceForKeyword = kwName
            ; Find the first NND keyword that represents a category with appropriate names for the actor.
            If !NNDSystemKeywords.HasForm(kw) && Find(kwName, "NND") == 0
                
                Int forcedIndex = Find(kwName, "_Forced")
                Int factionIndex = Find(kwName, "_Faction")
                Int classIndex = Find(kwName, "_Class")
                Int raceIndex = Find(kwName, "_Race")
                
                ; If race not found use whole keyword as default race priority.
                ; Other priorities will be handled before checking the race priority, so no conflict there.
                If forcedIndex == -1 && factionIndex == -1 && classIndex == -1 && raceIndex == -1
                    raceIndex = 0
                    NNDTrace("Keyword doesn't have any supported priorities, it will be considered a Race keyword")
                EndIf
                
                If forcedIndex != -1
                    kwName = Substring(kwName, 0, forcedIndex)
                    forcedKeywordsQueue[index] = kwName
                    NNDTrace("Setting Forced NNDKeyword")
                ElseIf factionIndex != -1
                    kwName = Substring(kwName, 0, factionIndex)
                    factionKeywordsQueue[index] = kwName
                    NNDTrace("Setting Faction NNDKeyword")
                ElseIf classIndex != -1
                    kwName = Substring(kwName, 0, classIndex)
                    classKeywordsQueue[index] = kwName
                    NNDTrace("Setting Class NNDKeyword")
                ElseIf raceIndex != -1
                    kwName = Substring(kwName, 0, raceIndex)
                    raceKeywordsQueue[index] = kwName
                    NNDTrace("Setting Race NNDKeyword")
                EndIf  
                
                
            EndIf
        EndIf
        index += 1
    EndWhile

    traceForKeyword = ""

    forcedKeywordsQueue = ClearKeywordsQueue(forcedKeywordsQueue)
    factionKeywordsQueue = ClearKeywordsQueue(factionKeywordsQueue)
    classKeywordsQueue = ClearKeywordsQueue(classKeywordsQueue)
    raceKeywordsQueue = ClearKeywordsQueue(raceKeywordsQueue)

    ; Merge all queues together.
    Return MergeStringArraySafe(MergeStringArraySafe(forcedKeywordsQueue, factionKeywordsQueue), MergeStringArraySafe(classKeywordsQueue, raceKeywordsQueue), true)
EndFunction

; Checks whether given keyword has a valid associated Name Definition and can be used.
Bool Function IsValidKeyword(String kwName)
    If !NNDSettings.KeywordHasValidDefinition(kwName)
        NNDTrace("Name Definition " + kwName + ".json is either missing or malformed. Check that it is present in Data/SKSE/Plugins/NPCsNamesDistributor/ and contains a valid JSON", 2)
        Return false
    EndIf
    Return true
EndFunction

;;; Utility functions used to make life easier while writing this sciprt.

; Finalizes keywords queue by removing empty placeholders and duplicates.
; The resulting array is a sorted queue of keywords that were present in the original queue.
; For example, ["NNDKeyword1", "", "NNDKeyword3", "NNDKeyword1"] will become ["NNDKeyword1", "NNDKeyword3", "NNDKeyword1"]
; Note that unique keywords will be filtered only during Merge stage.
String[] Function ClearKeywordsQueue(String[] queue)
    queue = ClearEmptySafe(queue)
    If queue.Length == 0
        Return queue
    EndIf
    SortStringArray(queue)
    Return queue
EndFunction

; This is a "safe" implementation of ClearEmpty function from PapyrusUtil.
; Unfortunately, ClearEmpty from PapyrusUtil seems to return None when all values in the array were empty.
; This breaks papyrus, as it cannot cast None back to String[] and there is nothing to be done on my side.
; For example, ClearEmpty(["", "", ""]) would return None, rather than an empty array: [].
String[] Function ClearEmptySafe(String[] stringArray)
    Int arrSize = stringArray.Length

    If arrSize == 0
        Return stringArray
    EndIf

    Int index = 0
    Int emptyCount = 0
    ; Count empty strings in the array
    While index < arrSize
        If stringArray[index] == ""
            emptyCount += 1
        EndIf
        index += 1
    EndWhile

    ; If all are empty then simply return an empty array
    If emptyCount == arrSize
        Return CreateStringArray(0)
    EndIf

    ; Otherwise it is safe to call ClearEmpty
    Return ClearEmpty(stringArray)
EndFunction

; This is a "safe" implementation of MergeStringArray function from PapyrusUtil.
; Unfortunately, MergeStringArray from PapyrusUtil doesn't play well with empty arrays that might be passed into it.
; In such cases it returns None and this breaks papyrus, as it cannot cast None back to String[] and there is nothing to be done on my side.
String[] Function MergeStringArraySafe(String[] first, String[] second, Bool removeDupes = false)
    If first.Length == 0
        Return second
    EndIf

    If second.Length == 0
        Return first
    EndIf

    Return MergeStringArray(first, second, removeDupes)
EndFunction

; I got to thinking… maybe I'm the Dragonborn and I just don't know it yet!

; Creates a JSON path using given keys (e.g. "key1.key2.key3.key4")
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

; Returns either the first or the second String based on evaluated condition.
String Function Either(Bool condition, String first, String second)
    If condition
        Return first
    Else
        Return second
    EndIf
EndFunction

; Returns either the first or the second Int based on evaluated condition.
Int Function EitherInt(Bool condition, Int first, Int second)
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

; Never should have come here!

; Assign a keyword that is currently being processed to this property,
; so that NNDTrace would add it as a tag for easier understanding.
;
; Remember to clear its value as soon as you're done with keywords, so NNDTrace wouldn't tag irrelevant messages.
String traceForKeyword = ""

; Assign a name of the target actor that is currently being processed to this property,
; so that NNDTrace would add it as a tag for easier understanding.
String traceForName = ""

; Logs trace with a distincive prefix for easier reading through logs.
; - Parameter traceName: Specific name of the actor to associate traced log with.
; - Parameter     level: Severity is one of the following:
;                           0 - Info
;                           1 - Warning
;                           2 - Error
Function NNDTrace(String sMessage, Int level = 0, String traceName = "")
    String msg = RenamingScriptName() + ": "
    If level == 1
        msg += "[WARNING] "
    ElseIf level == 2
        msg += "[ERROR] "
    Endif

    If traceName != ""
        msg += "[" + traceName + "] "
    ElseIf traceForName != ""
        msg += "[" + traceForName + "] "
    EndIf
    If traceForName == "" && _originalName != ""
        msg += "[" + _originalName + "] "
    EndIf
    If traceForKeyword != ""
        msg += "[" + traceForKeyword + "] "
    EndIf
    msg += sMessage + "."
    Trace(msg, level)
EndFunction

; You'’'re either the bravest person I've ever met… or the biggest fool.
; The end :)