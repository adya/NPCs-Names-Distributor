#include "Hooks.h"
#include "Distributor.h"
#include "Options.h"
#include "Persistency.h"

namespace NND
{
	using namespace Distribution;

	namespace Cache
	{
		struct Character_Load3D
		{
			static RE::NiAVObject* thunk(RE::Character* a_this, bool a_backgroundLoading) {
				// Avoid processing any names before the game is loaded.
				// In this case we're avoiding creating redundant NNDData objects during loading process.
				if (!Persistency::Manager::GetSingleton()->IsLoadingGame(); a_this && !a_this->IsPlayerRef()) {
					Manager::GetSingleton()->CreateData(a_this);
				}
				return func(a_this, a_backgroundLoading);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline constexpr std::size_t index{ 0 };
			static inline constexpr std::size_t size{ 0x6A };
		};

		void Install() {
			stl::write_vfunc<RE::Character, Character_Load3D>();
			logger::info("Installed Character hooks");
		}
	}

	namespace Naming
	{
		inline const char* sMissingName() {
			auto settings = RE::GameSettingCollection::GetSingleton();
			if (auto setting = settings->GetSetting("sMissingName")) {
				return setting->data.s;
			}
			return nullptr;
		}

		static const char* GetName(NameStyle style, RE::TESObjectREFR* ref, const char* originalName) {
			if (!ref || !ref->Is(RE::FormType::ActorCharacter)) {
				return originalName;
			}

			if (const auto actor = ref->As<RE::Actor>(); actor != RE::PlayerCharacter::GetSingleton()) {
				if (const auto name = Manager::GetSingleton()->GetName(style, actor); name != empty && name != sMissingName()) {
					return name.data();
				}
			}
			return originalName != sMissingName() ? originalName : "";
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
					return GetName(Options::NameContext::kOther, obj, originalName);
				}

				static inline REL::Relocation<decltype(thunk)> func;
			};

			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed in all other cases, like notifications.
			struct GetDisplayFullName_GetFormName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					if (a_this->Is(RE::FormType::ActorCharacter)) {
						// For characters we want to display the name as their display full name. Probably this will never be executed since this branch in the game executed when it's already known to be other types of objects.
						const auto originalName = Naming::Default::GetDisplayFullName(a_this);
						return GetName(Options::NameContext::kOther, a_this, originalName);
					}

					// For other references just do what the game does.
					if (const auto base = a_this->GetBaseObject(); base) {
						return base->GetName();
					} else {
						return "";
					}
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
						const auto originalName = Naming::Default::GetDisplayFullName(a_this);
						return GetName(Options::NameContext::kOther, a_this, originalName);
					}
					static inline REL::Relocation<decltype(thunk)> func;
				};

				inline void Install() {
					const REL::Relocation<std::uintptr_t> activate{ RELOCATION_ID(24211, 24715) };

					// Erase the whole thing with TESFullName. Instead Actor::GetBaseObject_1406313C0 will return a name.
					// lea	rcx, [rax+0D8h]
					// mov  rax, [rcx]
					// call qword ptr[rax + 28h]
					REL::safe_fill(activate.address() + OFFSET(0x6B0, 0x6CF), 0x90, 13);
					stl::write_thunk_call<Activate_GetBaseObject>(activate.address() + OFFSET(0x6AB, 0x6CA));

					logger::info("Installed Pickpocket Notification hooks");
				}
			}

			inline void Install() {
				const REL::Relocation<std::uintptr_t> displayFullName{ RE::Offset::TESObjectREFR::GetDisplayFullName };

				// Swaps the argument in TESForm::GetFormName_1401A38F0(object->data.objectReference) to pass TESObjectREFR* obj instead of obj->GetBaseObject()
				// mov rcx, [r15+40h] (49 8B 4F 40)
				//                     v  v  v  +
				// mov rcx, r15       (4C 89 F9) + 90
				REL::safe_fill(displayFullName.address() + OFFSET(0x232, 0x228), 0x4C, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0x233, 0x229), 0x89, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0x234, 0x22A), 0xF9, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0x235, 0x22B), 0x90, 1);
				stl::write_thunk_call<GetDisplayFullName_GetFormName>(displayFullName.address() + OFFSET(0x236, 0x22C));

				// Swaps 2nd argument in ExtraTextDisplayData::GetDisplayName_140145820(extraTextData, object->data.objectReference, temperFactor) to pass TESObjectREFR* obj instead of obj->GetBaseObject()
				// mov rdx, [r15+40h] (49 8B 57 40)
				//                     v  v  v   +
				// mov rdx, r15       (4C 89 FA) + 90
				REL::safe_fill(displayFullName.address() + OFFSET(0x240, 0x236), 0x4C, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0x241, 0x237), 0x89, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0x242, 0x238), 0xFA, 1);
				REL::safe_fill(displayFullName.address() + OFFSET(0x243, 0x239), 0x90, 1);
				stl::write_thunk_call<GetDisplayFullName_GetDisplayName>(displayFullName.address() + OFFSET(0x247, 0x23D));

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
					return GetName(Options::NameContext::kSubtitles, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			/// Short circuit the condition to be always true, to force DisplayNextSubtitle to always call GetDisplayFullName.
			struct DisplayNextSubtitle_ExtraDataList
			{
				static std::uintptr_t thunk(RE::ExtraDataList*) {
					return 1;
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> displayNextSubtitle{ RELOCATION_ID(51761, 52637) };
				stl::write_thunk_call<DisplayNextSubtitle_ExtraDataList>(displayNextSubtitle.address() + OFFSET(0xEF, 0xE8));
				stl::write_thunk_call<DisplayNextSubtitle_GetDisplayFullName>(displayNextSubtitle.address() + OFFSET(0x117, 0x110));
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
					return GetName(Options::NameContext::kEnemyHUD, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> enemyHealthUpdate{ RELOCATION_ID(50776, 51671) };
				stl::write_thunk_call<EnemyHealthUpdate_GetDisplayFullName>(enemyHealthUpdate.address() + OFFSET(0x21B, 0x20E));  // For Name length
				stl::write_thunk_call<EnemyHealthUpdate_GetDisplayFullName>(enemyHealthUpdate.address() + OFFSET(0x261, 0x254));  // For actual name that will be displayed
				logger::info("Installed EnemyHUD hooks");
			}
		}

		namespace Crosshair
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed on player's minions.
			struct ActivateText_Minion_GetDisplayFullName
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(Options::NameContext::kCrosshairMinion, a_this, originalName);
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
					return GetName(Options::NameContext::kCrosshair, a_this, originalName);
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
					return GetName(Options::NameContext::kCrosshair, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> activateText{ RELOCATION_ID(24212, 24716) };
				stl::write_thunk_call<ActivateText_Minion_GetDisplayFullName>(activateText.address() + OFFSET(0xB3, 0xB5));
				stl::write_thunk_call<ActivateText_Minion_GetDisplayFullName>(activateText.address() + OFFSET(0xDA, 0xDC));

				stl::write_thunk_call<ActivateText_Search_GetDisplayFullName>(activateText.address() + OFFSET(0x23D, 0x23D));
				stl::write_thunk_call<ActivateText_Default_GetDisplayFullName>(activateText.address() + OFFSET(0x33A, 0x33A));

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
					return GetName(Options::NameContext::kDialogue, a_this, originalName);
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
					return GetName(Options::NameContext::kDialogue, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> dialogueMenu{ RELOCATION_ID(34455, 35282) };
				stl::write_thunk_call<MenuTopicManager_GetDisplayFullName>(dialogueMenu.address() + OFFSET(0x4AE, 0x587));

				const REL::Relocation<std::uintptr_t> activateText{ RELOCATION_ID(24212, 24716) };
				stl::write_thunk_call<ActivateText_Dialogue_GetDisplayFullName>(activateText.address() + OFFSET(0x1F2, 0x1F2));
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
					return GetName(Options::NameContext::kInventory, a_this, originalName);
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
					return GetName(Options::NameContext::kInventory, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> barterMenuInventory{ RELOCATION_ID(50012, 50956) };
				stl::write_thunk_call<BarterMenu_GetDisplayFullName>(barterMenuInventory.address() + OFFSET(0x4B, 0x94));

				const REL::Relocation<std::uintptr_t> containerMenu{ RELOCATION_ID(50213, 51142) };
				stl::write_thunk_call<ContainerMenu_GetDisplayFullName>(containerMenu.address() + OFFSET(0x50, 0x99));
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
					// If there is a short name, pass it as original, otherwise fallback to default full name.
					if (const auto actor = a_this->As<RE::Actor>(); actor) {
						if (const auto npc = actor->GetActorBase(); npc && !npc->shortName.empty()) {
							return GetName(Options::NameContext::kBarter, a_this, npc->shortName.c_str());
						}
					}
					return GetName(Options::NameContext::kBarter, a_this, Default::GetDisplayFullName(a_this));
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install() {
				const REL::Relocation<std::uintptr_t> barterMenuGold{ RELOCATION_ID(50013, 50957) };
				// Swaps the argument in TESNPC::GetShortName(character->data.objectReference) to pass TESObjectREFR* character instead of character->GetBaseObject()
				// mov rcx, [rbx+40h] (48 8B 4B 40)
				//                           v  v
				// mov rcx, [rbp+30h] (48 8B 4D 30)
				REL::safe_fill(barterMenuGold.address() + OFFSET(0x20A, 0x20A), 0x4D, 1);
				REL::safe_fill(barterMenuGold.address() + OFFSET(0x20B, 0x20B), 0x30, 1);
				stl::write_thunk_call<BarterMenu_GetShortName>(barterMenuGold.address() + OFFSET(0x20C, 0x20C));
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
			static bool thunk(RE::Character* a_this, bool inDialogue, bool forceGreet, RE::TESTopicInfo* a_topic) {
				if (a_this && inDialogue && (Options::Obscurity::greetings || !forceGreet)) {
					if (Manager::GetSingleton()->RevealName(a_this))
						RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
				}
				return func(a_this, inDialogue, forceGreet, a_topic);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline constexpr std::size_t index{ 0 };
			static inline constexpr std::size_t size{ 0x41 };
		};

		struct TESNPC_Activate_Dead_OpenInventory
		{
			static const char* thunk(RE::Actor* a_this, RE::ContainerMenu::ContainerMode mode) {
				if (a_this && Options::Obscurity::obituary) {
					if (Manager::GetSingleton()->RevealName(a_this))
						RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
				}
				return func(a_this, mode);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct TESNPC_Activate_Pickpocket_OpenInventory
		{
			static const char* thunk(RE::Actor* a_this, RE::ContainerMenu::ContainerMode mode) {
				if (a_this && Options::Obscurity::stealing) {
					if (Manager::GetSingleton()->RevealName(a_this))
						RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
				}
				return func(a_this, mode);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install() {
			stl::write_vfunc<RE::Character, Character_SetDialogueWithPlayer>();
			logger::info("Installed SetDialogueWithPlayer hook");

			const REL::Relocation<std::uintptr_t> activate{ RELOCATION_ID(24211, 24715) };

			stl::write_thunk_call<TESNPC_Activate_Dead_OpenInventory>(activate.address() + OFFSET(0x3D8, 0x3E7));
			stl::write_thunk_call<TESNPC_Activate_Pickpocket_OpenInventory>(activate.address() + OFFSET(0x69E, 0x6BB));
			logger::info("Installed OpenInventory hooks");
		}
	}

	void Install() {
		logger::info("{:*^30}", "HOOKS");
		Naming::Install();
		Obscurity::Install();
		Cache::Install();
	}
}
