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

			Name displayName{};

			std::string sourceNameDefinition{};

			NameRef GetNameInScope(NameDefinition::Scope scope);

			void UpdateDisplayName();
		};

		inline std::unordered_map<RE::FormID, NNDData> names{};

		NameRef GetName(const RE::TESNPC* npc);
	}
}
