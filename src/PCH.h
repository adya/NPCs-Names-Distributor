#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <CLIBUtil/simpleINI.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
using namespace std::literals;

namespace NND
{
	inline void UpdateCrosshairs() {
		SKSE::GetTaskInterface()->AddUITask([]() {
			RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
		});
	}
}

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif

#include "Version.h"
