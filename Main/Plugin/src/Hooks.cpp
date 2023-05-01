#include "Hooks.h"

namespace NND
{
	struct GetDisplayName
	{
		static const char* thunk(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* obj, float temperFactor)
		{
			auto originalName = func(a_this, obj, temperFactor);
			logger::info("[GetDisplayName] Here we will determine new name for {}", originalName);
			// TODO: Generate a name
			std::string name(originalName);
			name = "Injected! " + name;
			const RE::BSFixedString str = name;
			return str.data();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct GetFormName
	{
		static const char* thunk(RE::TESBoundObject* a_this)
		{
			auto originalName = func(a_this);
			logger::info("[GetFormName] Here we will determine new name for {}", originalName);
			// TODO: Generate a name
			std::string name(originalName);
			name = "Injected! " + name;
            const RE::BSFixedString str = name;
			return str.data();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		logger::info("{:*^30}", "HOOKS");

		const REL::Relocation<std::uintptr_t> target{ RE::Offset::TESObjectREFR::GetDisplayFullName };
		stl::write_thunk_call<GetDisplayName>(target.address() + OFFSET(0, 0x23D));
		stl::write_thunk_call<GetFormName>(target.address() + OFFSET(0, 0x22C));

		logger::info("Installed GetFullDisplayName hooks");

		logger::info("{:*^30}", "GAME");
	}
}
