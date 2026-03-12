#include "Hooks.h"
#include "Distributor.h"
#include "Options.h"
#include "Persistency.h"
#include "RE/T/TESObjectREFR.h"
#include "Hooking.h"

namespace NND
{
	using namespace Distribution;

	namespace Cache
	{
		struct Character_Load3D
		{
			using Target = RE::Character;

			static inline constexpr std::size_t index{ 0x6A };


			static RE::NiAVObject* thunk(RE::Character* a_this, bool a_backgroundLoading) {
				// Avoid processing any names before the game is loaded.
				// In this case we're avoiding creating redundant NNDData objects during loading process.
				if (!Persistency::Manager::GetSingleton()->IsLoadingGame(); a_this && !a_this->IsPlayerRef()) {
					Manager::GetSingleton()->CreateData(a_this);
				}
				return func(a_this, a_backgroundLoading);
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void post_hook() {
				logger::info("🪝Installed Character hook");
			}
		};
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
				static inline constexpr REL::ID relocation = RELOCATION_ID(19354, 19781);
				static inline constexpr size_t  offset = OFFSET(0x247, 0x23D);

				static void pre_hook() {
					// Swaps 2nd argument in ExtraTextDisplayData::GetDisplayName_140145820(extraTextData, object->data.objectReference, temperFactor) to pass TESObjectREFR* obj instead of obj->GetBaseObject()
					// mov rdx, [r15+40h] (49 8B 57 40)
					//                     v  v  v   +
					// mov rdx, r15       (4C 89 FA) + 90
					REL::safe_fill(relocation.address() + OFFSET(0x240, 0x236), 0x4C, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x241, 0x237), 0x89, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x242, 0x238), 0xFA, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x243, 0x239), 0x90, 1);
				}

				static const char* thunk(RE::ExtraTextDisplayData* a_this, RE::TESObjectREFR* obj, float temperFactor) {
					const auto originalName = a_this->GetDisplayName(obj->GetBaseObject(), temperFactor);
					return GetName(Options::NameContext::kOther, obj, originalName);
				}

				static inline REL::Relocation<decltype(thunk)> func;

				static void post_hook() {
					logger::info("🪝Installed Default GetDisplayFullName hook");
				}
			};

			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed in all other cases, like notifications.
			struct GetDisplayFullName_GetFormName
			{
				static inline constexpr REL::ID relocation = RELOCATION_ID(19354, 19781);
				static inline constexpr size_t  offset = OFFSET(0x236, 0x22C);

				static void pre_hook() {
					// Swaps the argument in TESForm::GetFormName_1401A38F0(object->data.objectReference) to pass TESObjectREFR* obj instead of obj->GetBaseObject()
					// mov rcx, [r15+40h] (49 8B 4F 40)
					//                     v  v  v  +
					// mov rcx, r15       (4C 89 F9) + 90
					REL::safe_fill(relocation.address() + OFFSET(0x232, 0x228), 0x4C, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x233, 0x229), 0x89, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x234, 0x22A), 0xF9, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x235, 0x22B), 0x90, 1);
				}

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

				static void post_hook() {
					logger::info("🪝Installed Default GetDisplayFullName hook");
				}
			};

			namespace PickpocketNotification
			{
				/// Vanilla: Full.
				///	    NND: Full.
				/// Name displayed in pickpocket notification ("%Name% has already caught you")
				struct Activate_GetBaseObject
				{
					static inline constexpr REL::ID relocation = RELOCATION_ID(24211, 24715);
					static inline constexpr size_t  offset = OFFSET(0x6AB, 0x6CA);

					static void pre_hook() {
						// Erase the whole thing with TESFullName. Instead Actor::GetBaseObject_1406313C0 will return a name.
						// lea	rcx, [rax+0D8h]
						// mov  rax, [rcx]
						// call qword ptr[rax + 28h]
						REL::safe_fill(relocation.address() + OFFSET(0x6B0, 0x6CF), 0x90, 13);
					}

					static const char* thunk(RE::Actor* a_this) {
						const auto originalName = Naming::Default::GetDisplayFullName(a_this);
						return GetName(Options::NameContext::kOther, a_this, originalName);
					}

					static inline REL::Relocation<decltype(thunk)> func;

					static void post_hook() {
						logger::info("🪝Installed Pickpocket Notification hook");
					}
				};
			}
		}

		namespace Subtitles
		{
			/// Vanilla: Short.
			///	    NND: Short.
			///	Name displayed in subtitles.
			struct DisplayNextSubtitle_GetDisplayFullName
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(51761, 52637);
				static inline constexpr std::size_t offset = OFFSET(0x117, 0x110);

				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(Options::NameContext::kSubtitles, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;

				static void post_hook() {
					logger::info("🪝Installed DisplayNextSubtitle hook");
				}
			};

			/// Short circuit the condition to be always true, to force DisplayNextSubtitle to always call GetDisplayFullName.
			struct DisplayNextSubtitle_ExtraDataList
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(51761, 52637);
				static inline constexpr std::size_t offset = OFFSET(0xEF, 0xE8);

				static std::uintptr_t thunk(RE::ExtraDataList*) {
					return 1;
				}
				static inline REL::Relocation<decltype(thunk)> func;

				static void post_hook() {
					logger::info("🪝Installed DisplayNextSubtitle aux hook");
				}
			};
		}

		namespace EnemyHUD
		{
			/// Vanilla: Full.
			///	    NND: Full.
			///	Name displayed in the HUD near Enemy health.
			template<typename Call>
			struct EnemyHealthUpdate_GetDisplayFullName_Base
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = Call::func(a_this);
					return GetName(Options::NameContext::kEnemyHUD, a_this, originalName);
				}
			};

			struct EnemyHealthUpdate_GetDisplayFullName_NameLength
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(50776, 51671);
				static inline constexpr std::size_t offset = OFFSET(0x21B, 0x20E);

				using Proxy = EnemyHealthUpdate_GetDisplayFullName_Base<EnemyHealthUpdate_GetDisplayFullName_NameLength>;

				static inline REL::Relocation<decltype(Proxy::thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed EnemyHUD length hook");
				}
			};

			struct EnemyHealthUpdate_GetDisplayFullName_Name
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(50776, 51671);
				static inline constexpr std::size_t offset = OFFSET(0x261, 0x254);

				using Proxy = EnemyHealthUpdate_GetDisplayFullName_Base<EnemyHealthUpdate_GetDisplayFullName_Name>;

				static inline REL::Relocation<decltype(Proxy::thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed EnemyHUD name hook");
				}
			};
		}

		namespace Crosshair
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// Name displayed on player's minions.
			template<typename Call>
			struct ActivateText_GetDisplayFullName_Minion_Base
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = Call::func(a_this);
					return GetName(Options::NameContext::kCrosshairMinion, a_this, originalName);
				}
			};

			struct ActivateText_GetDisplayFullName_Minion_Call1
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(24212, 24716);
				static inline constexpr std::size_t offset = OFFSET(0xB3, 0xB5);

				using Proxy = ActivateText_GetDisplayFullName_Minion_Base<ActivateText_GetDisplayFullName_Minion_Call1>;

				static inline REL::Relocation<decltype(Proxy::thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed Crosshair Minion1 hook");
				}
			};
			
			struct ActivateText_GetDisplayFullName_Minion_Call2
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(24212, 24716);
				static inline constexpr std::size_t offset = OFFSET(0xDA, 0xDC);

				using Proxy = ActivateText_GetDisplayFullName_Minion_Base<ActivateText_GetDisplayFullName_Minion_Call2>;

				static inline REL::Relocation<decltype(Proxy::thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed Crosshair Minion2 hook");
				}
			};

			template <typename Call>
			struct ActivateText_GetDisplayFullName_Base
			{
				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = Call::func(a_this);
					return GetName(Options::NameContext::kCrosshair, a_this, originalName);
				}
			};

			/// Vanilla: Full.
			///	    NND: Full.
			/// Default activation name (e.g. Talk, Steal, Pickpocket, etc.)
			struct ActivateText_GetDisplayFullName_Default
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(24212, 24716);
				static inline constexpr std::size_t offset = OFFSET(0x33A, 0x33A);

				using Proxy = ActivateText_GetDisplayFullName_Base<ActivateText_GetDisplayFullName_Default>;

				static inline REL::Relocation<decltype(Proxy::thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed Crosshair Default1 hook");
				}
			};

			/// Vanilla: Full.
			///	    NND: Full.
			/// Activation name for Search action.
			struct ActivateText_GetDisplayFullName_Search : ActivateText_GetDisplayFullName_Base<ActivateText_GetDisplayFullName_Search>
			{
				static inline constexpr REL::ID relocation = RELOCATION_ID(24212, 24716);
				static inline constexpr std::size_t offset = OFFSET(0x23D, 0x23D);

				static inline REL::Relocation<decltype(thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed Crosshair Default2 hook");
				}
			};
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
				static inline constexpr REL::ID     relocation = RELOCATION_ID(24212, 24716);
				static inline constexpr std::size_t offset = OFFSET(0x1F2, 0x1F2);

				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(Options::NameContext::kDialogue, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed ActivateText hook.");
				}
			};

			// Vanilla: Full.
			///	   NND: Full.
			/// Name in Dialogue menu.
			struct MenuTopicManager_GetDisplayFullName
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(34455, 35282);
				static inline constexpr std::size_t offset = OFFSET(0x4AE, 0x587);

				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(Options::NameContext::kDialogue, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed MenuTopicManager hook.");
				}
			};
		}

		namespace Inventory
		{
			/// Vanilla: Full.
			///	    NND: Full.
			/// Name in the Inventory menu when bartering.
			struct BarterMenu_GetDisplayFullName
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(50012, 50956);
				static inline constexpr std::size_t offset = OFFSET(0x4B, 0x94);

				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(Options::NameContext::kInventory, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed BarterMenu hook.");
				}
			};

			/// Vanilla: Full.
			///     NND: Full.
			/// Name in the Inventory menu when Pickpocketing or trading with followers and all other cases of inventory menu, except barter.
			struct ContainerMenu_GetDisplayFullName
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(50213, 51142);
				static inline constexpr std::size_t offset = OFFSET(0x50, 0x99);

				static const char* thunk(RE::TESObjectREFR* a_this) {
					const auto originalName = func(a_this);
					return GetName(Options::NameContext::kInventory, a_this, originalName);
				}
				static inline REL::Relocation<decltype(thunk)> func;

				static inline void post_hook() {
					logger::info("🪝Installed ContainerMenu hook.");
				}
			};
		}

		namespace Barter
		{
			/// Vanilla: Short.
			///	    NND: Short.
			/// Name at the bottom bar near Gold amount when bartering.
			struct BarterMenu_GetShortName
			{
				static inline constexpr REL::ID     relocation = RELOCATION_ID(50013, 50957);
				static inline constexpr std::size_t offset = OFFSET(0x20C, 0x20C);

				static void pre_hook() {
					// Swaps the argument in TESNPC::GetShortName(character->data.objectReference) to pass TESObjectREFR* character instead of character->GetBaseObject()
					// mov rcx, [rbx+40h] (48 8B 4B 40)
					//                           v  v
					// mov rcx, [rbp+30h] (48 8B 4D 30)
					REL::safe_fill(relocation.address() + OFFSET(0x20A, 0x20A), 0x4D, 1);
					REL::safe_fill(relocation.address() + OFFSET(0x20B, 0x20B), 0x30, 1);
				}

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

				static inline void post_hook() {
					logger::info("🪝Installed BarterMenu Gold hook.");
				}
			};
		}
	}

	namespace Obscurity
	{
		struct Character_SetDialogueWithPlayer
		{
			using Target = RE::Character;
			static inline constexpr std::size_t index{ 0x41 };

			static bool thunk(RE::Character* a_this, bool inDialogue, bool forceGreet, RE::TESTopicInfo* a_topic) {
				if (a_this && inDialogue && (Options::Obscurity::greetings || !forceGreet)) {
					if (Manager::GetSingleton()->RevealName(a_this))
						NND::UpdateCrosshairs();
				}
				return func(a_this, inDialogue, forceGreet, a_topic);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline void post_hook() {
				logger::info("🪝Installed SetDialogueWithPlayer hook");
			}
		};

		struct TESNPC_Activate_Dead_OpenInventory
		{
			static inline constexpr REL::ID     relocation = RELOCATION_ID(24211, 24715);
			static inline constexpr std::size_t offset = OFFSET(0x3D8, 0x3E7);

			static const char* thunk(RE::Actor* a_this, RE::ContainerMenu::ContainerMode mode) {
				if (a_this && Options::Obscurity::obituary) {
					if (Manager::GetSingleton()->RevealName(a_this))
						NND::UpdateCrosshairs();
				}
				return func(a_this, mode);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline void post_hook() {
				logger::info("🪝Installed Looting hook");
			}
		};

		struct TESNPC_Activate_Pickpocket_OpenInventory
		{
			static inline constexpr REL::ID     relocation = RELOCATION_ID(24211, 24715);
			static inline constexpr std::size_t offset = OFFSET(0x69E, 0x6BB);

			static const char* thunk(RE::Actor* a_this, RE::ContainerMenu::ContainerMode mode) {
				if (a_this && Options::Obscurity::stealing) {
					if (Manager::GetSingleton()->RevealName(a_this))
						NND::UpdateCrosshairs();
				}
				return func(a_this, mode);
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline void post_hook() {
				logger::info("🪝Installed Pickpocket hook");
			}
		};
	}

	void Install() {
		logger::info("{:*^30}", "HOOKS");

		constexpr size_t totalBytes = 14 * 18;
		
		stl::on_trampoline(totalBytes, []() {
			stl::install_hook<Cache::Character_Load3D>();

			stl::install_hook<Naming::Default::GetDisplayFullName_GetFormName>();
			stl::install_hook<Naming::Default::GetDisplayFullName_GetDisplayName>();
			stl::install_hook<Naming::Default::PickpocketNotification::Activate_GetBaseObject>();

			stl::install_hook<Naming::Subtitles::DisplayNextSubtitle_GetDisplayFullName>();
			stl::install_hook<Naming::Subtitles::DisplayNextSubtitle_ExtraDataList>();

			stl::install_hook<Naming::EnemyHUD::EnemyHealthUpdate_GetDisplayFullName_NameLength>();
			stl::install_hook<Naming::EnemyHUD::EnemyHealthUpdate_GetDisplayFullName_Name>();

			stl::install_hook<Naming::Crosshair::ActivateText_GetDisplayFullName_Minion_Call1>();
			stl::install_hook<Naming::Crosshair::ActivateText_GetDisplayFullName_Minion_Call2>();
			stl::install_hook<Naming::Crosshair::ActivateText_GetDisplayFullName_Default>();
			stl::install_hook<Naming::Crosshair::ActivateText_GetDisplayFullName_Search>();

			stl::install_hook<Naming::Dialogue::ActivateText_Dialogue_GetDisplayFullName>();
			stl::install_hook<Naming::Dialogue::MenuTopicManager_GetDisplayFullName>();

			stl::install_hook<Naming::Inventory::BarterMenu_GetDisplayFullName>();
			stl::install_hook<Naming::Inventory::ContainerMenu_GetDisplayFullName>();

			stl::install_hook<Naming::Barter::BarterMenu_GetShortName>();

			stl::install_hook<Obscurity::Character_SetDialogueWithPlayer>();
			stl::install_hook<Obscurity::TESNPC_Activate_Dead_OpenInventory>();
			stl::install_hook<Obscurity::TESNPC_Activate_Pickpocket_OpenInventory>();
		});
	}
}
