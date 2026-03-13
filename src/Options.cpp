#include "Options.h"

#include "Hotkeys.h"

namespace NND
{
	constexpr const char* NameStyleToString(NameStyle style) {
		switch (style) {
		case kDisplayName:
			return "display";
		case kFullName:
			return "full";
		case kShortName:
			return "short";
		case kTitle:
			return "title";
		default:
			return "full";
		}
	}

	constexpr std::string name(const NameStyle& style) {
		switch (style) {
		case kDisplayName:
			return "Display Name";
		default:
		case kFullName:
			return "Full Name";
		case kShortName:
			return "Short Name";
		case kTitle:
			return "Title";
		}
	}

	template <typename T>
	void SetOrDeleteValue(CSimpleIniA& ini, const char* section, const char* key, const T& value, const T& defaultValue) {}

	template <>
	void SetOrDeleteValue<bool>(CSimpleIniA& ini, const char* section, const char* key, const bool& value, const bool& defaultValue) {
		if (value != defaultValue) {
			ini.SetBoolValue(section, key, value);
		} else {
			ini.Delete(section, key, true);
		}
	}

	template <>
	void SetOrDeleteValue<const char*>(CSimpleIniA& ini, const char* section, const char* key, const char* const& value, const char* const& defaultValue) {
		if (strcmp(value, defaultValue) != 0) {
			ini.SetValue(section, key, value);
		} else {
			ini.Delete(section, key, true);
		}
	}

	template <>
	void SetOrDeleteValue<std::string>(CSimpleIniA& ini, const char* section, const char* key, const std::string& value, const std::string& defaultValue) {
		if (value != defaultValue) {
			ini.SetValue(section, key, value.c_str());
		} else {
			ini.Delete(section, key, true);
		}
	}

	template <>
	void SetOrDeleteValue<NameStyle>(CSimpleIniA& ini, const char* section, const char* key, const NameStyle& value, const NameStyle& defaultValue) {
		if (value != defaultValue) {
			ini.SetValue(section, key, NameStyleToString(value));
		} else {
			ini.Delete(section, key, true);
		}
	}

	bool ReadStyle(const CSimpleIniA& ini, const char* name, NameStyle& style) {
		if (const auto rawName = ini.GetValue("NameContext", name)) {
			if (clib_util::string::iequals(rawName, "display")) {
				style = kDisplayName;
				return true;
			}
			if (clib_util::string::iequals(rawName, "full")) {
				style = kFullName;
				return true;
			}
			if (clib_util::string::iequals(rawName, "short")) {
				style = kShortName;
				return true;
			}
			if (clib_util::string::iequals(rawName, "title")) {
				style = kTitle;
				return true;
			}
			logger::warn("WARN: Unexpected value for NameContext:{}. Got '{}', but expected either: 'display', 'full', 'short', 'title'", name, rawName);
		}
		return false;
	}

	bool ReadHotkey(const CSimpleIniA& ini, const char* name, std::string& pattern, Hotkeys::KeyCombination& hotkey) {
		if (const auto rawPattern = ini.GetValue("Hotkeys", name)) {
			if (hotkey.SetPattern(rawPattern)) {
				pattern = rawPattern;
				return true;
			}
			logger::error("Failed to set '{}' as a hotkey for {}", pattern, name);
		}
		return false;
	}

	void Options::Load() {
		logger::info("{:*^30}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			General::enabled = ini.GetBoolValue("General", "bEnabled", General::enabled);

			Obscurity::enabled = ini.GetBoolValue("Obscurity", "bEnabled", Obscurity::enabled);
			Obscurity::greetings = ini.GetBoolValue("Obscurity", "bGreetings", Obscurity::greetings);
			Obscurity::obituary = ini.GetBoolValue("Obscurity", "bObituary", Obscurity::obituary);
			Obscurity::stealing = ini.GetBoolValue("Obscurity", "bStealing", Obscurity::stealing);
			if (const auto defaultName = ini.GetValue("Obscurity", "sDefaultName", Obscurity::defaultName.data()); defaultName != empty) {
				Obscurity::defaultName = defaultName;
			} else {
				logger::warn("Obscurity:sDefaultName cannot be empty. Previous value ('{}') will be used.", Obscurity::defaultName);
			}

			if (const auto format = ini.GetValue("DisplayName", "sFormat"); format && format != empty) {
				DisplayName::format = format;
			} else if (const auto formatIndex = ini.GetLongValue("DisplayName", "iFormat", -1); formatIndex >= 0 && formatIndex < DisplayName::defaultFormats.size()) {
				DisplayName::format = DisplayName::defaultFormats[formatIndex];
			}

			ReadStyle(ini, "sCrosshair", NameContext::kCrosshair);
			ReadStyle(ini, "sCrosshairMinion", NameContext::kCrosshairMinion);
			ReadStyle(ini, "sSubtitles", NameContext::kSubtitles);
			ReadStyle(ini, "sDialogue", NameContext::kDialogue);
			ReadStyle(ini, "sDialogueHistory", NameContext::kDialogueHistory);
			ReadStyle(ini, "sInventory", NameContext::kInventory);
			ReadStyle(ini, "sBarter", NameContext::kBarter);
			ReadStyle(ini, "sEnemyHUD", NameContext::kEnemyHUD);
			ReadStyle(ini, "sOther", NameContext::kOther);

			const auto manager = NND::Hotkeys::Manager::GetSingleton();
			ReadHotkey(ini, "sToggleNames", Hotkeys::toggleNames, manager->toggleNames);
			ReadHotkey(ini, "sToggleObscurity", Hotkeys::toggleObscurity, manager->toggleObscurity);
			ReadHotkey(ini, "sGenerateNames", Hotkeys::generateAll, manager->generateAll);
			ReadHotkey(ini, "sGenerateNameTarget", Hotkeys::generateTarget, manager->generateTarget);
			ReadHotkey(ini, "sReloadSettings", Hotkeys::reloadSettings, manager->reloadSettings);
			ReadHotkey(ini, "sFixStuckName", Hotkeys::fixStuckName, manager->fixStuckName);
			ReadHotkey(ini, "sUnsafeFixStuckName", Hotkeys::unsafeFixStuckName, manager->unsafeFixStuckName);

		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("General:");
		logger::info("\tNames distribution {}", General::enabled ? "enabled" : "disabled");

		logger::info("Hotkeys:");
		logger::info("\tToggle Names: {}", Hotkeys::toggleNames);
		logger::info("\tToggle Obscurity: {}", Hotkeys::toggleObscurity);
		logger::info("\tReload Settings: {}", Hotkeys::reloadSettings);
		logger::info("\tRegenerate All Names: {}", Hotkeys::generateAll);
		logger::info("\tRegenerate Target Name: {}", Hotkeys::generateTarget);
		logger::info("\tFix Stuck Names (pre NND 2.0): {}", Hotkeys::fixStuckName);
		logger::info("\tUnsafe Fix Stuck Names (pre NND 2.0): {}", Hotkeys::unsafeFixStuckName);
		logger::info("");

		logger::info("Obscurity:");
		logger::info("\tObscuring Names {}", Obscurity::enabled ? "enabled" : "disabled");
		logger::info("\tCan be revealed in dialogue: {}", Obscurity::greetings ? "By both Player and NPC" : "Only by Player");
		logger::info("\tCan be revealed when looting: {}", Obscurity::obituary ? "Yes" : "No");
		logger::info("\tCan be revealed when pickpocketing: {}", Obscurity::stealing ? "Yes" : "No");
		logger::info("\tDefault Name: {}", Obscurity::defaultName);
		logger::info("");

		logger::info("Display Name:");
		logger::info("\tFormat: {}", DisplayName::format);
		logger::info("");

		logger::info("Name Contexts:");
		logger::info("\tCrosshair: {}", name(NameContext::kCrosshair));
		logger::info("\tCrosshair Minion: {}", name(NameContext::kCrosshairMinion));
		logger::info("\tSubtitles: {}", name(NameContext::kSubtitles));
		logger::info("\tDialogue: {}", name(NameContext::kDialogue));
		logger::info("\tDialogue History: {}", name(NameContext::kDialogueHistory));
		logger::info("\tInventory: {}", name(NameContext::kInventory));
		logger::info("\tBarter: {}", name(NameContext::kBarter));
		logger::info("\tEnemy HUD: {}", name(NameContext::kEnemyHUD));
		logger::info("\tOther: {}", name(NameContext::kOther));
	}

	void Options::Save() {
		const std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini)";
		CSimpleIniA                 ini{};
		ini.SetUnicode();
		ini.LoadFile(options.string().c_str());

		SetOrDeleteValue(ini, "General", "bEnabled", custom.General.enabled, defaults.General.enabled);

		SetOrDeleteValue(ini, "Obscurity", "bEnabled", custom.Obscurity.enabled, defaults.Obscurity.enabled);
		SetOrDeleteValue(ini, "Obscurity", "bGreetings", custom.Obscurity.greetings, defaults.Obscurity.greetings);
		SetOrDeleteValue(ini, "Obscurity", "bObituary", custom.Obscurity.obituary, defaults.Obscurity.obituary);
		SetOrDeleteValue(ini, "Obscurity", "bStealing", custom.Obscurity.stealing, defaults.Obscurity.stealing);
		SetOrDeleteValue(ini, "Obscurity", "sDefaultName", custom.Obscurity.defaultName, defaults.Obscurity.defaultName);

		SetOrDeleteValue(ini, "DisplayName", "sFormat", custom.DisplayName.format, defaults.DisplayName.format);

		SetOrDeleteValue(ini, "NameContext", "sCrosshair", custom.NameContext.kCrosshair, defaults.NameContext.kCrosshair);
		SetOrDeleteValue(ini, "NameContext", "sCrosshairMinion", custom.NameContext.kCrosshairMinion, defaults.NameContext.kCrosshairMinion);
		SetOrDeleteValue(ini, "NameContext", "sSubtitles", custom.NameContext.kSubtitles, defaults.NameContext.kSubtitles);
		SetOrDeleteValue(ini, "NameContext", "sDialogue", custom.NameContext.kDialogue, defaults.NameContext.kDialogue);
		SetOrDeleteValue(ini, "NameContext", "sDialogueHistory", custom.NameContext.kDialogueHistory, defaults.NameContext.kDialogueHistory);
		SetOrDeleteValue(ini, "NameContext", "sInventory", custom.NameContext.kInventory, defaults.NameContext.kInventory);
		SetOrDeleteValue(ini, "NameContext", "sBarter", custom.NameContext.kBarter, defaults.NameContext.kBarter);
		SetOrDeleteValue(ini, "NameContext", "sEnemyHUD", custom.NameContext.kEnemyHUD, defaults.NameContext.kEnemyHUD);
		SetOrDeleteValue(ini, "NameContext", "sOther", custom.NameContext.kOther, defaults.NameContext.kOther);

		SetOrDeleteValue(ini, "Hotkeys", "sGenerateNames", custom.Hotkeys.generateAll, defaults.Hotkeys.generateAll);
		SetOrDeleteValue(ini, "Hotkeys", "sGenerateNameTarget", custom.Hotkeys.generateTarget, defaults.Hotkeys.generateTarget);
		SetOrDeleteValue(ini, "Hotkeys", "sToggleObscurity", custom.Hotkeys.toggleObscurity, defaults.Hotkeys.toggleObscurity);
		SetOrDeleteValue(ini, "Hotkeys", "sToggleNames", custom.Hotkeys.toggleNames, defaults.Hotkeys.toggleNames);
		SetOrDeleteValue(ini, "Hotkeys", "sReloadSettings", custom.Hotkeys.reloadSettings, defaults.Hotkeys.reloadSettings);
		SetOrDeleteValue(ini, "Hotkeys", "sFixStuckName", custom.Hotkeys.fixStuckName, defaults.Hotkeys.fixStuckName);
		SetOrDeleteValue(ini, "Hotkeys", "sUnsafeFixStuckName", custom.Hotkeys.unsafeFixStuckName, defaults.Hotkeys.unsafeFixStuckName);

		ini.SaveFile(options.string().c_str());
	}

}
