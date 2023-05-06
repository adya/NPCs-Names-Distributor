#pragma once
#include "NameDefinition.h"

namespace NND
{
	void Install();

	/// Returns flag indicating whether at least one NameDefinition had been loaded.
	bool LoadNameDefinitions();

	inline std::unordered_map<std::string, NameDefinition> definitions{};
}
