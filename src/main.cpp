#include "Distributor.h"
#include "Hooks.h"
#include "Hotkeys.h"
#include "LookupNameDefinitions.h"
#include "ModAPI.h"
#include "NNDKeywords.h"
#include "Options.h"
#include "Persistency.h"

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		// Disregard result of the LoadNameDefinitions. If nothing is loaded we still can use Obscurity.
		NND::LoadNameDefinitions();
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		NND::Options::Load();
		NND::Install();
		NND::Distribution::Manager::Register();
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		NND::Hotkeys::Manager::Register();
		NND::Persistency::Manager::Register();
		NND::CacheKeywords();
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		NND::Persistency::Manager::GetSingleton()->StartLoadingGame();
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
		NND::Options::Load();
		NND::Persistency::Manager::GetSingleton()->FinishLoadingGame();
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("NPCsNamesDistributor");
	v.AuthorName("sasnikol");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info) {
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "NPCsNamesDistributor";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

std::string current_date_string() {
	auto        now = std::chrono::system_clock::now();
	std::time_t time_now = std::chrono::system_clock::to_time_t(now);
	std::tm     tm_now;
#ifdef _WIN32
	localtime_s(&tm_now, &time_now);
#else
	localtime_r(&time_now, &tm_now);
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm_now, "%Y-%m-%d");
	return oss.str();
}

void InitializeLog() {
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);

#ifndef NDEBUG
	bool truncate = false;
#else
	bool truncate = true;
#endif

	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), truncate);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
	log->set_pattern("[%H:%M:%S] %v"s);

	spdlog::set_default_logger(std::move(log));

#ifndef NDEBUG
	logger::info(FMT_STRING("{:*^30}"), current_date_string());
#endif
	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse, false);

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}

extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI(const NND_API::InterfaceVersion a_interfaceVersion) {
	const auto api = Messaging::NNDInterface::GetSingleton(a_interfaceVersion);

	logger::info("NND::RequestPluginAPI called, InterfaceVersion {}", static_cast<std::underlying_type<NND_API::InterfaceVersion>::type>(a_interfaceVersion));

	switch (a_interfaceVersion) {
	case NND_API::InterfaceVersion::kV1:
	case NND_API::InterfaceVersion::kV2:
		logger::info("NND::RequestPluginAPI returned the API singleton");
		return api;
	}

	logger::info("NND::RequestPluginAPI requested the wrong interface version");
	return nullptr;
}
