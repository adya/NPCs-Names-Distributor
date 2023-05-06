#include "Hooks.h"

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
}
