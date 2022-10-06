Scriptname NND_ApplyName extends NND_RenamingEffect  

Keyword Property NNDTitleless Auto

String Function DecorateName(Actor akTarget, String generatedName, String originalName)
    If !akTarget.GetLeveledActorBase().HasKeyword(NNDTitleless)
        Return FormattedTitle(generatedName, originalName)
    Else
        Return generatedName
    EndIf
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