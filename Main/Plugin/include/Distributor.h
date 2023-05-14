#pragma once
#include "NameDefinition.h"

namespace NND
{
	enum NameFormat
	{
		/// Show full name with title.
		kFullName,

		/// Show only name without a title.
		kName,

		/// Show short name if available.
		kShortName
	};

	namespace Distribution
	{
		struct NNDData
		{
			RE::FormID formId;

			Name name{};
			Name title{};
			Name obscurity{};

			Name shortDisplayName{};

			Name displayName{};

			bool isUnique = false;
			bool isKnown = false;
			bool isTitleless = false;

			void UpdateDisplayName();

			NameRef GetName(NameFormat format) const;
		};

		inline std::unordered_map<RE::FormID, NNDData> names{};

		NameRef GetName(NameFormat format, const RE::TESNPC* npc, const char* originalName);
	}
}
