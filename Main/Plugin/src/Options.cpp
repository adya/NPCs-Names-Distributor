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

	void ReadStyle(const CSimpleIniA& ini, const char* name, NameStyle& style) {
		auto rawName = ini.GetValue("NameContext", name);
		if (clib_util::string::iequals(rawName, "display"))
			style = kDisplayName;
		if (clib_util::string::iequals(rawName, "full"))
			style = kFullName;
		if (clib_util::string::iequals(rawName, "short"))
			style = kShortName;
		if (clib_util::string::iequals(rawName, "title"))
			style = kTitle;

		logger::warn("WARN: Unexpected value for NameContext:{}. Got '{}', but expected either: 'display', 'full', 'short', 'title'", name, rawName);
	}

	void Options::Load() {
		logger::info("{:*^30}", "OPTIONS");
		std::filesystem::path options = R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini)";
		CSimpleIniA           ini{};
		ini.SetUnicode();
		if (ini.LoadFile(options.string().c_str()) >= 0) {
			ReadStyle(ini, "Crosshair", NameContext::kCrosshair);
			ReadStyle(ini, "CrosshairMinion", NameContext::kCrosshairMinion);
			ReadStyle(ini, "Subtitles", NameContext::kSubtitles);
			ReadStyle(ini, "Dialogue", NameContext::kDialogue);
			ReadStyle(ini, "Inventory", NameContext::kInventory);
			ReadStyle(ini, "Barter", NameContext::kBarter);
			ReadStyle(ini, "EnemyHUD", NameContext::kEnemyHUD);
			ReadStyle(ini, "Other", NameContext::kOther);
		} else {
			logger::info(R"(Data\SKSE\Plugins\NPCsNamesDistributor.ini not found. Default options will be used.)");
		}

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
