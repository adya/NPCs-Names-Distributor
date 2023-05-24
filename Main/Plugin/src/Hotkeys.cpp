#include "Hotkeys.h"

#include "Distributor.h"

namespace NND
{
	// Events
	namespace Hotkeys
	{
		namespace Confirmation
		{
			inline constexpr auto message = R"(Regenerate All Names

This action will delete cached names for all NPCs. New names will be generated (and cached) for them on-demand.

Delete cache?

(You can reload last save to undo this action.))";

			class GenerateNamesConfirmCallback : public RE::IMessageBoxCallback
			{
			public:
				GenerateNamesConfirmCallback() = default;

				~GenerateNamesConfirmCallback() override = default;

				void Run(Message a_msg) override {
					const std::int32_t response = static_cast<std::int32_t>(a_msg) - 4;
					if (response == 0) {
						logger::info("Resetting all names..");
						Distribution::Manager::GetSingleton()->UpdateNames([](auto& names) {
							names.clear();
						});
						// In case we're looking at someone when reseting the cache..
						RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
					}
				}
			};

			RE::MessageBoxData* MakeMessageBox(const std::string& a_message) {
				const auto factoryManager = RE::MessageDataFactoryManager::GetSingleton();
				const auto uiStrHolder = RE::InterfaceStrings::GetSingleton();

				if (factoryManager && uiStrHolder) {
					auto factory = factoryManager->GetCreator<RE::MessageBoxData>(uiStrHolder->messageBoxData);
					auto messageBox = factory ? factory->Create() : nullptr;
					if (messageBox) {
						messageBox->unk4C = 4;
						messageBox->unk38 = 10;
						messageBox->bodyText = a_message;

						return messageBox;
					}
				}

				return nullptr;
			}
		}

		void Manager::Register() {
			if (const auto scripts = RE::BSInputDeviceManager::GetSingleton()) {
				scripts->AddEventSink<RE::InputEvent*>(GetSingleton());
				logger::info("Registered for {}", typeid(RE::InputEvent).name());
			}
		}

		void Manager::GenerateAllTrigger(const KeyCombination* keys) {
			if (const auto messageBox = Confirmation::MakeMessageBox(Confirmation::message)) {
				if (const auto gameSettings = RE::GameSettingCollection::GetSingleton()) {
					const auto sYesText = gameSettings->GetSetting("sYesText");
					const auto sNoText = gameSettings->GetSetting("sNoText");
					if (sYesText && sNoText) {
						messageBox->buttonText.push_back(sYesText->GetString());
						messageBox->buttonText.push_back(sNoText->GetString());

						messageBox->callback = RE::BSTSmartPointer<RE::IMessageBoxCallback>{ new Confirmation::GenerateNamesConfirmCallback() };
						messageBox->QueueMessage();
					}
				}
			}
		}

		void Manager::GenerateTargetTrigger(const KeyCombination*) {
			if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
				if (const auto actor = actorRef->As<RE::Actor>()) {
					logger::info("Resetting name for target..");
					Distribution::Manager::GetSingleton()->DeleteData(actor);
					// Immediately refresh the name for NPC we are looking at.
					RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
				}
			}
		}

		void Manager::ReloadSettingsTrigger(const KeyCombination* keys) {
			logger::info("Reloading settings..");
			Options::Load();
			// In case we're looking at someone when reloading options (like default names or formats).
			RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
		}

		void Manager::ToggleObscurityTrigger(const KeyCombination* keys) {
			Options::Obscurity::enabled = !Options::Obscurity::enabled;
			// In case we're looking at someone when toggling obscurity.
			RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
			logger::info("Toggled obscurity {}", Options::Obscurity::enabled ? "ON"sv : "OFF"sv);
			Options::Save();
		}

		RE::BSEventNotifyControl Manager::ProcessEvent(RE::InputEvent* const* a_event,
		                                               RE::BSTEventSource<RE::InputEvent*>*) {
			using EventType = RE::INPUT_EVENT_TYPE;
			using Result = RE::BSEventNotifyControl;

			if (!a_event)
				return Result::kContinue;

			const auto ui = RE::UI::GetSingleton();
			if (ui->GameIsPaused()) {
				return Result::kContinue;
			}

			generateAll.Process(a_event) ||
				generateTarget.Process(a_event) ||
				reloadSettings.Process(a_event) ||
				toggleObscurity.Process(a_event);

			return Result::kContinue;
		}

		Manager::Manager() {
			if (!generateAll.SetPattern(Options::Hotkeys::generateAll))
				logger::error("Failed to set Key Combination for generateAll", Options::Hotkeys::generateAll);
			if (!generateTarget.SetPattern(Options::Hotkeys::generateTarget))
				logger::error("Failed to set Key Combination for generateTarget", Options::Hotkeys::generateTarget);
			if (!reloadSettings.SetPattern(Options::Hotkeys::reloadSettings))
				logger::error("Failed to set Key Combination '{}' for reloadSettings", Options::Hotkeys::reloadSettings);
			if (!toggleObscurity.SetPattern(Options::Hotkeys::toggleObscurity))
				logger::error("Failed to set Key Combination '{}' for toggleObscurity", Options::Hotkeys::toggleObscurity);
		}
	}

}
