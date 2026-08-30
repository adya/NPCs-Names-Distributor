#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

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

#include "Version.h"
