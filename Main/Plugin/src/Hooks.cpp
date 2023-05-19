#include "Hooks.h"
#include "Distributor.h"

namespace NND
{
	using namespace Distribution;

	enum class NameContext
	{
		kDefault = 0,
		kCrosshair,
		kSubtitles,
		kDialogue,
		kInventory,
		kBarter,
		kEnemyHUD
	};

	// TODO: Read from settings.
	inline NameFormat GetFormatByContext(NameContext context) {
		switch (context) {
		default:
		case NameContext::kDefault: return kFullName;
		case NameContext::kCrosshair: return kDisplayName;
		case NameContext::kSubtitles: return kShortName;
		case NameContext::kDialogue: return kFullName;
		case NameContext::kInventory: return kFullName;
		case NameContext::kBarter: return kShortName;
		case NameContext::kEnemyHUD: return kFullName;
		}
	}

	namespace Naming
	{
		static const char* GetName(NameContext context, const RE::TESObjectREFR* ref, const char* originalName) {
			if (!ref || !ref->Is(RE::FormType::ActorCharacter)) {
				return originalName;
			}

			if (const auto actor = ref->As<RE::Actor>()) {
				if (const auto name = Distribution::Manager::GetSingleton()->GetName(GetFormatByContext(context), actor, originalName); name != empty) {
					return name.data();
				}
			}

			return originalName;
		}

		namespace Default
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed in all other cases, like notifications.
			struct GetDisplayFullName_GetDisplayName
			{
				static const char* thunk(RE::ExtraTextDisplayData* a_this, RE::TESObjectREFR* obj, float temperFactor) {
					const auto originalName = a_this->GetDisplayName(obj->GetBaseObject(), temperFactor);
					return GetName(NameContext::kDefault, obj, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed in all other cases, like notifications.
			struct GetDisplayFullName_GetFormName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = a_this->GetBaseObject()->GetName();
					return GetName(NameContext::kDefault, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			namespace PickpocketNotification
			{
				/// Vanilla: Full.
				///	    NND: Full.
				/// Name displayed in pickpocket notification ("%Name% has already caught you")
				struct Activate_GetBaseObject
				{
					static const char* thunk(RE::Actor* a_this) {
						const auto originalName = a_this->GetDisplayFullName();
						return GetName(NameContext::kDefault, a_this, originalName);
					}
					static inline REL::Relocation<decltype(thunk)> func;
				};

				inline void Install() {
					const REL::Relocation<std::uintptr_t> activate{ RELOCATION_ID(0, 24715) };

					// Erase the whole thing with TESFullName. Instead Actor::GetBaseObject_1406313C0 will return a name.
					// lea	rcx, [rax+0D8h]
					// mov  rax, [rcx]
					// call qword ptr[rax + 28h]
					REL::safe_fill(activate.address() + OFFSET(0, 0x6CF), 0x90, 13);
					stl::write_thunk_call<Activate_GetBaseObject>(activate.address() + OFFSET(0, 0x6CA));

					logger::info("Installed Pickpocket Notification hooks");
				}
			}

			inline void Install() {
				// Yeah, these will need to be re-hooked to somewhere where Actor is used.
				const REL::Relocation<std::uintptr_t> displayFullName{ RE::Offset::TESObjectREFR::GetDisplayFullName };

				// Swaps the argument to pass TESObjectREFR* obj instead of obj->GetBaseObject() 
				// mov rcx, [r15+40h] (49 8B 4F 40)
				//                     v  v  v  +
				// mov rcx, r15       (4C 89 F9) + 90
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x228), 0x4C, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x229), 0x89, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x22A), 0xF9, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x22B), 0x90, 1);
				stl::write_thunk_call<GetDisplayFullName_GetFormName>(displayFullName.address() + OFFSET(0, 0x22C));

				// Swaps 2nd argument to pass TESObjectREFR* obj instead of obj->GetBaseObject() 
				// mov rdx, [r15+40h] (49 8B 57 40)
				//                     v  v  v   +
				// mov rdx, r15       (4C 89 FA) + 90
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x236), 0x4C, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x237), 0x89, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x238), 0xFA, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0, 0x239), 0x90, 1);
				stl::write_thunk_call<GetDisplayFullName_GetDisplayName>(displayFullName.address() + OFFSET(0, 0x23D));

				logger::info("Installed Default GetDisplayFullName hooks");

				PickpocketNotification::Install();
			}
		}

		namespace Subtitles
		{
			/// Vanilla: Short.
			///	    NND: Short.
			///	Name displayed in subtitles.
			struct DisplayNextSubtitle_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kSubtitles, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			/// Short circuit the condition to be always true, to force DisplayNextSubtitle to always call GetDisplayFullName.
			struct DisplayNextSubtitle_ExtraDataList
			{
				static std::uintptr_t thunk(RE::ExtraDataList* a_this) {
					return 1;
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> displayNextSubtitle{ RELOCATION_ID(0, 52637) };
				stl::write_thunk_call<DisplayNextSubtitle_ExtraDataList>(displayNextSubtitle.address() + OFFSET(0, 0xE8));
				stl::write_thunk_call<DisplayNextSubtitle_GetDisplayFullName>(displayNextSubtitle.address() + OFFSET(0, 0x110));
				logger::info("Installed Subtitles hooks");
			}
		}

		namespace EnemyHUD
		{
			/// Vanilla: Full.
			///	    NND: Full.
			///	Name displayed in the HUD near Enemy health.
			struct EnemyHealthUpdate_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kEnemyHUD, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> enemyHealthUpdate{ RELOCATION_ID(0, 51671) };
				stl::write_thunk_call<EnemyHealthUpdate_GetDisplayFullName>(enemyHealthUpdate.address() + OFFSET(0, 0x20E));  // For Name length
				stl::write_thunk_call<EnemyHealthUpdate_GetDisplayFullName>(enemyHealthUpdate.address() + OFFSET(0, 0x254));  // For actual name that will be displayed
				logger::info("Installed EnemyHUD hooks");
			}
		}

		namespace Crosshair
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed on someone's remains.
			struct ActivateText_Bones_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kCrosshair, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};
			
			/// Vanilla: Full.
			///	    NND: Full.
			/// Default activation name (e.g. Talk, Steal, Pickpocket, etc.)
			struct ActivateText_Default_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kCrosshair, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			/// Vanilla: Full.
			///	    NND: Full.
			/// Activation name for Search action.
			struct ActivateText_Search_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kCrosshair, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> activateText{ RELOCATION_ID(0, 24716) };
				stl::write_thunk_call<ActivateText_Bones_GetDisplayFullName>(activateText.address() + OFFSET(0, 0xB5));
				stl::write_thunk_call<ActivateText_Bones_GetDisplayFullName>(activateText.address() + OFFSET(0, 0xDC));

				stl::write_thunk_call<ActivateText_Search_GetDisplayFullName>(activateText.address() + OFFSET(0, 0x23D));
				stl::write_thunk_call<ActivateText_Default_GetDisplayFullName>(activateText.address() + OFFSET(0, 0x33A));

				logger::info("Installed Crosshair hooks");
			}
		}

		namespace Dialogue
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// This is a name displayed while UI transitions into Dialogue menu.
			///	The order is this:
			///	1) ActivateText_Default_GetDisplayFullName
			///	2) ActivateText_Dialogue_GetDisplayFullName <- you are here
			///	3) MenuTopicManager_GetDisplayFullName
			struct ActivateText_Dialogue_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kDialogue, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			// Vanilla: Full.
			///	   NND: Full.
			/// Name in Dialogue menu.
			struct MenuTopicManager_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kDialogue, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install()
			{
				const REL::Relocation<std::uintptr_t> dialogueMenu{ RELOCATION_ID(0, 35282) };
				stl::write_thunk_call<MenuTopicManager_GetDisplayFullName>(dialogueMenu.address() + OFFSET(0, 0x587));

				const REL::Relocation<std::uintptr_t> activateText{ RELOCATION_ID(0, 24716) };
				stl::write_thunk_call<ActivateText_Dialogue_GetDisplayFullName>(activateText.address() + OFFSET(0, 0x1F2));
				logger::info("Installed ActivateText hooks");
			}
		}

		namespace Inventory
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// Name in the Inventory menu when bartering.
			struct BarterMenu_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kInventory, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			/// Vanilla: Full.
			///     NND: Full.
			/// Name in the Inventory menu when Pickpocketing or trading with followers and all other cases of inventory menu, except barter.
			struct ContainerMenu_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(NameContext::kInventory, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> barterMenuInventory{ RELOCATION_ID(0, 50956) };
				stl::write_thunk_call<BarterMenu_GetDisplayFullName>(barterMenuInventory.address() + OFFSET(0, 0x94));

				const REL::Relocation<std::uintptr_t> containerMenu{ RELOCATION_ID(0, 51142) };
				stl::write_thunk_call<ContainerMenu_GetDisplayFullName>(containerMenu.address() + OFFSET(0, 0x99));
				logger::info("Installed Inventory hooks");
			}
		}

		namespace Barter
		{
			/// Vanilla: Short.
			///	    NND: Short.
			/// Name at the bottom bar near Gold amount when bartering.
			struct BarterMenu_GetShortName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					return GetName(NameContext::kBarter, a_this, a_this->GetDisplayFullName());
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> barterMenuGold{ RELOCATION_ID(0, 50957) };
				// Swaps the argument to pass TESObjectREFR* obj instead of obj->GetBaseObject() 
				// mov rcx, [rbx+40h] (48 8B 4B 40)
				//                           v  v
				// mov rcx, [rbp+30h] (48 8B 4D 30)
				REL::safe_fill(barterMenuGold.address() + OFFSET(0, 0x20A), 0x4D, 1);
				REL::safe_fill(barterMenuGold.address() + OFFSET(0, 0x20B), 0x30, 1);
				stl::write_thunk_call<BarterMenu_GetShortName>(barterMenuGold.address() + OFFSET(0, 0x20C));
			}
		}

		inline void Install() {
			Default::Install();
			Subtitles::Install();
			EnemyHUD::Install();
			Crosshair::Install();
			Dialogue::Install();
			Inventory::Install();
			Barter::Install();
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
					RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
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
