#include "Options.h"

namespace NND
{
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

	void Options::Load() {
		logger::info("{:*^30}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			Obscurity::enabled = ini.GetBoolValue("Obscurity", "bEnabled", Obscurity::enabled);
			Obscurity::greetings = ini.GetBoolValue("Obscurity", "bGreetings", Obscurity::greetings);
			Obscurity::defaultName = ini.GetValue("Obscurity", "sDefaultName", Obscurity::defaultName.data());
			
			if (const auto format = ini.GetValue("DisplayName", "sFormat")) {
				DisplayName::format = format;
			} else if (const auto formatIndex = ini.GetLongValue("DisplayName", "iFormat", -1); formatIndex >= 0 && formatIndex < DisplayName::defaultFormats.size()) {
				DisplayName::format = DisplayName::defaultFormats[formatIndex];
			}

			ReadStyle(ini, "sCrosshair", NameContext::kCrosshair);
			ReadStyle(ini, "sCrosshairMinion", NameContext::kCrosshairMinion);
			ReadStyle(ini, "sSubtitles", NameContext::kSubtitles);
			ReadStyle(ini, "sDialogue", NameContext::kDialogue);
			ReadStyle(ini, "sInventory", NameContext::kInventory);
			ReadStyle(ini, "sBarter", NameContext::kBarter);
			ReadStyle(ini, "sEnemyHUD", NameContext::kEnemyHUD);
			ReadStyle(ini, "sOther", NameContext::kOther);
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini not found. Default options will be used.)");
			logger::info("");
		}

		logger::info("Obscurity:");
		logger::info("\t{}", Obscurity::enabled ? "Enabled" : "Disabled");
		logger::info("\tCan be revealed {}", Obscurity::greetings ? "by both Player and NPC" : "only by Player");
		logger::info("\tDefault Name: {}", Obscurity::defaultName);
		logger::info("");

		logger::info("Display Name:");
		logger::info("\tFormat: {}", DisplayName::format);
		logger::info("");

		logger::info("Name Contexts:");
		logger::info("\tCrosshair: {}", name(NameContext::kCrosshair));
		logger::info("\tCrosshairMinion: {}", name(NameContext::kCrosshairMinion));
		logger::info("\tSubtitles: {}", name(NameContext::kSubtitles));
		logger::info("\tDialogue: {}", name(NameContext::kDialogue));
		logger::info("\tInventory: {}", name(NameContext::kInventory));
		logger::info("\tBarter: {}", name(NameContext::kBarter));
		logger::info("\tEnemyHUD: {}", name(NameContext::kEnemyHUD));
		logger::info("\tOther: {}", name(NameContext::kOther));
	}
}
