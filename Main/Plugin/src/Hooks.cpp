#include "Hooks.h"

#include "NameDefinitionDecoder.h"

namespace NND
{
	static const char* GetName(const RE::TESBoundObject* obj, const char* originalName)
	{
		if (!obj || !obj->Is(RE::FormType::NPC)) {
			return nullptr;
		}

		std::string name(originalName);
		name = "Injected! " + name;
		const RE::BSFixedString str = name;
		return str.data();
	}

	struct GetDisplayName
	{
		static const char* thunk(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* obj, float temperFactor)
		{
			const auto        originalName = func(a_this, obj, temperFactor);
		    return GetName(obj, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct GetFormName
	{
		static const char* thunk(RE::TESBoundObject* a_this)
		{
			const auto originalName = func(a_this);
			return GetName(a_this, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		logger::info("{:*^30}", "HOOKS");

		const REL::Relocation<std::uintptr_t> target{ RE::Offset::TESObjectREFR::GetDisplayFullName };
		stl::write_thunk_call<GetDisplayName>(target.address() + OFFSET(0, 0x23D));
		stl::write_thunk_call<GetFormName>(target.address() + OFFSET(0, 0x22C));

		logger::info("Installed GetDisplayFullName hooks");
	}

	
	// to be moved to Clib util
	inline std::vector<std::filesystem::path> get_configs(const std::filesystem::path& a_folder, const std::string_view& a_extension)
	{
		std::vector<std::filesystem::path> configs{};
		const auto                         iterator = std::filesystem::directory_iterator(a_folder);
		for (const auto& entry : iterator) {
			if (entry.exists()) {
				if (const auto& path = entry.path(); !path.empty() && path.extension() == a_extension) {
					configs.push_back(path);
				}
			}
		}

		std::ranges::sort(configs);

		return configs;
	}

	bool LoadNameDefinitions()
	{
		logger::info("{:*^30}", "NAME DEFINITIONS");
		const auto files = get_configs(R"(Data\SKSE\Plugins\NPCsNamesDistributor)", ".json"sv);

		if (files.empty()) {
			logger::info("No Name Definition files found. NPCsNamesDistributor will be disabled.");
			logger::info(R"(Make sure your Name Definition files are located at Data\SKSE\Plugins\NPCsNamesDistributor)");
			return false;
		}
		logger::info("{} name definition files found", files.size());
		int                   validFiles = 0;
		NameDefinitionDecoder decoder{};
		for (const auto& file : files) {
			const auto name = file.filename().string();
			logger::info("\t\t Loading {}", name);
			try {
				decoder.decode(file);
				++validFiles;
			} catch (...) {
				logger::warn("\t\tFailed to decode Name Definition {} with error", name);
			}
		}
		return validFiles > 0;
	}
}
