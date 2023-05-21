#pragma once
#include "NameDefinition.h"

namespace NND
{
	enum NameStyle : uint8_t
	{
		/// Show a full name with a title.
		kDisplayName,

		/// Show only a name without a title.
		kFullName,

		/// Show short name if available, otherwise fallback to kFullName.
		kShortName,

		/// Show only a title if available, otherwise fallback to kFullName.
		///
		///	This includes both custom Title or default title if isTitleless = false.
		kTitle
	};

	namespace Options
	{
		namespace Obscurity
		{
			inline bool enabled = true;
			inline Name defaultName = "???";
		}

		namespace NameContext
		{
			inline auto kCrosshair = kDisplayName;
			inline auto kCrosshairMinion = kTitle;

			inline auto kSubtitles = kShortName;
			inline auto kDialogue = kFullName;

			inline auto kInventory = kFullName;

			inline auto kBarter = kShortName;

			inline auto kEnemyHUD = kFullName;

			inline auto kOther = kFullName;
		}

		namespace DisplayName
		{
			/// Format string for DisplayName.
			///	Supports placeholders:
			///	- [name]: Substitutes full name
			///	- [title]: Substitutes title
			///	- [break]: Substitutes new line.
			inline std::string format = "[name] ([title])";
		}

		void Load();
	}
}
