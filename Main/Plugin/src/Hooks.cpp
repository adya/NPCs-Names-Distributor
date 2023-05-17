#include "Hooks.h"
#include "Distributor.h"

namespace NND
{
	using namespace Distribution;

	namespace Naming
	{
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

		inline void Install() {
			// Yeah, these will need to be re-hooked to somewhere where Actor is used.
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
		}
	}

	namespace Obscurity
	{
		struct Character_SetDialogueWithPlayer
		{
			static bool thunk(RE::Character* a_this, bool inDialgoue, bool forceGreet, RE::TESTopicInfo* a_topic) {
				if (a_this && inDialgoue) {
					// TODO: Handle force greet with custom options.
					Manager::GetSingleton()->RevealName(a_this->formID);
				}
				return func(a_this, inDialgoue, forceGreet, a_topic);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline constexpr std::size_t index{ 0 };
			static inline constexpr std::size_t size{ 0x41 };
		};

		inline void Install()
		{
			stl::write_vfunc<RE::Character, Character_SetDialogueWithPlayer>();
			logger::info("Installed SetDialogueWithPlayer hook");
		}
	}

	void Install() {
		logger::info("{:*^30}", "HOOKS");
		Naming::Install();
		Obscurity::Install();
	}
}
