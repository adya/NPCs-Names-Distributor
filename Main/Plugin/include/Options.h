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
			constexpr std::array defaultFormats = {
				"[name]"sv,                // 0
				"[name][break][title]"sv,  // 1
				"[name] ([title])"sv,      // 2
				"[name] [[title]]"sv,      // 3
				"[name], [title]"sv,       // 4
				"[name]; [title]"sv,       // 5
				"[name]. [title]"sv,       // 6
				"[name] [title]"sv         // 7
			};

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
