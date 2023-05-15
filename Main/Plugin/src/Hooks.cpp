#include "Hooks.h"
#include "Distributor.h"

namespace NND
{
	using namespace Distribution;

	static const char* GetName(NameFormat format, const RE::TESBoundObject* obj, const char* originalName) {
		if (!obj || !obj->Is(RE::FormType::NPC)) {
			return originalName;
		}

		if (const auto npc = obj->As<RE::TESNPC>()) {
			if (const auto name = Distribution::Manager::GetSingleton()->GetName(format, npc, originalName); name != empty) {
				return name.data();
			}
		}

		return originalName;
	}

	// Always Full
	struct GetDisplayFullName_GetDisplayName
	{
		static const char* thunk(RE::ExtraTextDisplayData* a_this, RE::TESBoundObject* obj, float temperFactor) {
			const auto originalName = func(a_this, obj, temperFactor);
			return GetName(kDisplayName, obj, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// Always Full
	struct GetDisplayFullName_GetFormName
	{
		static const char* thunk(RE::TESBoundObject* a_this) {
			const auto originalName = func(a_this);
			return GetName(kDisplayName, a_this, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// Short when possible
	struct DisplayNextSubtitle_GetDisplayFullName
	{
		static const char* thunk(RE::TESObjectREFR* a_this) {
			const auto originalName = func(a_this);
			return GetName(kShortName, a_this->GetObjectReference(), originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// Short when possible
	struct DisplayNextSubtitle_GetShortName
	{
		static const char* thunk(RE::TESNPC* a_this) {
			const auto originalName = func(a_this);
			return GetName(kShortName, a_this, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// Always Full
	struct BarterMenu_GetShortName
	{
		static const char* thunk(RE::TESNPC* a_this) {
			const auto originalName = func(a_this);
			return GetName(kFullName, a_this, originalName);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install() {
		logger::info("{:*^30}", "HOOKS");

		const REL::Relocation<std::uintptr_t> displayFullName{ RE::Offset::TESObjectREFR::GetDisplayFullName };
		stl::write_thunk_call<GetDisplayFullName_GetDisplayName>(displayFullName.address() + OFFSET(0, 0x23D));
		stl::write_thunk_call<GetDisplayFullName_GetFormName>(displayFullName.address() + OFFSET(0, 0x22C));
		logger::info("Installed GetDisplayFullName hooks");

		const REL::Relocation<std::uintptr_t> displayNextSubtitle{ RELOCATION_ID(0, 52637) };
		stl::write_thunk_call<DisplayNextSubtitle_GetDisplayFullName>(displayNextSubtitle.address() + OFFSET(0, 0x110));
		stl::write_thunk_call<DisplayNextSubtitle_GetShortName>(displayNextSubtitle.address() + OFFSET(0, 0x106));
		logger::info("Installed DisplayNextSubtitle hooks");

		const REL::Relocation<std::uintptr_t> barterMenu{ RELOCATION_ID(0, 50957) };
		stl::write_thunk_call<BarterMenu_GetShortName>(barterMenu.address() + OFFSET(0, 0x20C));
		logger::info("Installed BarterMenu hooks");

		/*REL::Relocation<std::uintptr_t> vtbl{ RE::Offset::Character::Vtbl };
		_SetDialogueWithPlayer = vtbl.write_vfunc(0x41, SetDialogueWithPlayer);*/
	}
}
