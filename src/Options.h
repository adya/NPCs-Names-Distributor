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
		///	This includes both custom Title or default title if allowDefaultTitle = true.
		kTitle
	};

	namespace Options
	{
		struct GeneralSettings
		{
			bool enabled = true;
		};

		struct ObscuritySettings
		{
			bool enabled = true;
			bool greetings = false;
			bool obituary = false;
			bool stealing = false;
			Name defaultName = "[sex] [race]";
		};

		struct NameContextSettings
		{
			NameStyle kCrosshair = kDisplayName;
			NameStyle kCrosshairMinion = kTitle;
			NameStyle kSubtitles = kShortName;
			NameStyle kDialogue = kFullName;
			NameStyle kDialogueHistory = kFullName;
			NameStyle kInventory = kFullName;
			NameStyle kBarter = kShortName;
			NameStyle kEnemyHUD = kFullName;
			NameStyle kOther = kFullName;
		};

		struct DisplayNameSettings
		{
			static constexpr std::array defaultFormats = {
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
			std::string format = "[name] ([title])";
		};

		struct HotkeysSettings
		{
			std::string generateAll = "RCtrl+RShift+G";
			std::string generateTarget = "RCtrl+G";
			std::string toggleObscurity = "RCtrl+O";
			std::string toggleNames = "RCtrl+N";
			std::string reloadSettings = "RCtrl+L";
			std::string fixStuckName = "RCtrl+Backspace";
			std::string unsafeFixStuckName = "RCtrl+RShift+Backspace";
		};

		struct OptionsData
		{
			GeneralSettings     General;
			ObscuritySettings   Obscurity;
			NameContextSettings NameContext;
			DisplayNameSettings DisplayName;
			HotkeysSettings     Hotkeys;
		};

		inline OptionsData defaults;
		inline OptionsData custom;

		// Backward compatibility aliases
		namespace General
		{
			inline bool& enabled = custom.General.enabled;
		}

		namespace Obscurity
		{
			inline bool& enabled = custom.Obscurity.enabled;
			inline bool& greetings = custom.Obscurity.greetings;
			inline bool& obituary = custom.Obscurity.obituary;
			inline bool& stealing = custom.Obscurity.stealing;
			inline Name& defaultName = custom.Obscurity.defaultName;
		}

		namespace NameContext
		{
			inline NameStyle& kCrosshair = custom.NameContext.kCrosshair;
			inline NameStyle& kCrosshairMinion = custom.NameContext.kCrosshairMinion;
			inline NameStyle& kSubtitles = custom.NameContext.kSubtitles;
			inline NameStyle& kDialogue = custom.NameContext.kDialogue;
			inline NameStyle& kDialogueHistory = custom.NameContext.kDialogueHistory;
			inline NameStyle& kInventory = custom.NameContext.kInventory;
			inline NameStyle& kBarter = custom.NameContext.kBarter;
			inline NameStyle& kEnemyHUD = custom.NameContext.kEnemyHUD;
			inline NameStyle& kOther = custom.NameContext.kOther;
		}

		namespace DisplayName
		{
			constexpr auto&     defaultFormats = DisplayNameSettings::defaultFormats;
			inline std::string& format = custom.DisplayName.format;
		}

		namespace Hotkeys
		{
			inline std::string& generateAll = custom.Hotkeys.generateAll;
			inline std::string& generateTarget = custom.Hotkeys.generateTarget;
			inline std::string& toggleObscurity = custom.Hotkeys.toggleObscurity;
			inline std::string& toggleNames = custom.Hotkeys.toggleNames;
			inline std::string& reloadSettings = custom.Hotkeys.reloadSettings;
			inline std::string& fixStuckName = custom.Hotkeys.fixStuckName;
			inline std::string& unsafeFixStuckName = custom.Hotkeys.unsafeFixStuckName;
		}

		void Save();
		void Load();
	}
}
