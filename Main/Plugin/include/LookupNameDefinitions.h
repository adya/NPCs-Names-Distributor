#pragma once
#include "NameDefinition.h"

namespace NND
{
	/// Loads all Name Definitions located at Data/SKSE/Plugins/NPCsNamesDistributor.
	/// Returns flag indicating whether at least one Name Definition had been loaded without errors.
	bool LoadNameDefinitions();

    /// A map of Name Definitions grouped by their names.
    ///	This map is populated by LoadNameDefinitions().
    inline std::unordered_map<std::string, NameDefinition> loadedDefinitions{};
}
