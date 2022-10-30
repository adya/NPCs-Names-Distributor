Scriptname NND_ObscureName extends NND_RenamingEffect  
{ 
    The gist :)
    OnEffectStart
        1. Try read known flag from storage for the actor
        2. If set, then immediately remove the perk
        3. Otherwise proceed with applying obscure name
        4. Generate one from NND definitions
            4.1. Save the obscure name
        5. If no custom obscured name was generated then fall back to preferred default obscure title.

    OnActivate
        1. Set known flag to storage    
        2. Remove the perk

    OnEffectFinish
        1. Restore originalName ?? this might conflict with the regular naming tracker as it will kick in right after perk removal
        As a workaround we might want to check if actor has name tracked on it and if renaming is enabled. 
        If both of this true then don't restore the name.
}

Keyword Property NNDKnown Auto

Perk Property NNDNameTracker Auto

GlobalVariable Property NNDRenamingEnabled Auto

String NNDKnownKey = "NNDIsKnown"

import PO3_SKSEFunctions

Event OnActivate(ObjectReference akActionRef)
    parent.OnActivate(akActionRef)
    Actor akActor = akActionRef as Actor
    Actor akTarget = GetTargetActor()
    If akActor == Game.GetPlayer()
        NNDTrace("Talking to " + akTarget)
        StorageUtil.SetIntValue(akTarget, NNDKnownKey, 1)
        NNDTrace("Removing obscurity")
        AddKeywordToRef(akTarget, NNDKnown)
    EndIf
EndEvent

Event OnInit()
    InitKeys(generatedName = "NNDObscuredName", generationId = "NNDObscuredGenerationId")
EndEvent

; Here we check for whether this actor is already known and immediately finish. Otherwise, proceed to general renaming logic.
Event OnEffectStart(Actor akTarget, Actor akCaster)
    If akTarget == None || akTarget.GetLeveledActorBase() == None
        NNDTrace("Failed to obscure an actor's name. akTarget is None", 2)
        Return
    EndIf

    Int isObscured = StorageUtil.GetIntValue(akTarget, NNDKnownKey)
    String traceName = akTarget.GetDisplayName()

    NNDTrace("Starting obscured naming. " + NNDKnownKey + " = " + isObscured, traceName = traceName)

    If isObscured > 0
        NNDTrace("Already known", traceName = traceName)
        AddKeywordToRef(akTarget, NNDKnown)
    Else
        parent.OnEffectStart(akTarget, akCaster)
    EndIf
EndEvent

String Function RenamingScriptName()
    Return "NNDObscureName"
EndFunction

String Function GenerateName(Actor akTarget, String[] keywords)
    Return PickNameFor(akTarget, keywords, true)
EndFunction

Bool Function NeedsKeywords(Actor akTarget)
    Return false
EndFunction

String Function DecorateName(Actor akTraget, String generatedName, String originalName)
; We will use this fucntion to provide a default obscure name if custom one wasn't generated.
    If generatedName != ""
        Return generatedName
    EndIf
    
    If NNDSettings.ObscurityStyle == 1
        Return "???"
    ElseIf NNDSettings.ObscurityStyle == 2
        Return akTraget.GetRace().GetName()
    Else
        Return originalName
    EndIf
EndFunction

Bool Function ShouldRevertOnFinish(Actor akTarget)
    ; Rename only if Actor doesn't have regular name tracker and renaming is enabled.
    ; In this case tracker will automatically apply correct name.
    Return parent.ShouldRevertOnFinish(akTarget) && (!akTarget.HasPerk(NNDNameTracker) || NNDRenamingEnabled.GetValueInt() == 0)
EndFunction
