#pragma once
#include "NameDefinition.h"

namespace NND
{
	namespace Distribution
	{
		struct NNDData
		{
			Name name{};
			Name title{};
			Name obscurity{};

			std::string sourceNameDefinition{};

			NameRef GetNameInScope(NameDefinition::Scope scope);
		};

		inline std::unordered_map<RE::FormID, NNDData> names{};

		NameRef GetName(const RE::TESNPC* npc);
	}
}
