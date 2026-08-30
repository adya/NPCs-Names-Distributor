#include "Distributor.h"
#include "Hooks.h"
#include "Hotkeys.h"
#include "LookupNameDefinitions.h"
#include "MCPMenu.h"
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
		NND::Menu::Register();
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

void InitializeLog() {
	auto path = logger::log_directory();
	if (!path) {
		SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);

	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), NND::Options::Log::truncate);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	auto logLevel = spdlog::level::from_str(NND::Options::Log::level);
	if (logLevel == spdlog::level::off) {
		logLevel = spdlog::level::info;
	}

	log->set_level(logLevel);
	log->flush_on(logLevel);
	log->set_pattern("[%H:%M:%S] %v"s);

	spdlog::set_default_logger(std::move(log));

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
	logger::info("Log level: {}", spdlog::level::to_string_view(logLevel));
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("NPCsNamesDistributor");
	v.AuthorName("sasnikol");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo) {
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {

	NND::Options::Load(true);

	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse, false);

	// Pre-allocate trampoline space for hook installation.
	// Each write_call<5> hook needs 14 bytes; NND installs 18 call hooks.
	SKSE::AllocTrampoline(14 * 18);

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}

extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI(const NND_API::InterfaceVersion a_interfaceVersion) {
	const auto api = Messaging::NNDInterface::GetSingleton(a_interfaceVersion);

	logger::info("NND::RequestPluginAPI called, InterfaceVersion {}", static_cast<std::underlying_type<NND_API::InterfaceVersion>::type>(a_interfaceVersion));

	logger::info("NND::RequestPluginAPI returned the API singleton");
	return api;
}
