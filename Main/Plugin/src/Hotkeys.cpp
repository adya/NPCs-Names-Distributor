#include "Hotkeys.h"

#include "Distributor.h"
#include "NameFixer.h"
#include "NameRegenerator.h"

namespace NND
{
	// Events
	namespace Hotkeys
	{
			void Manager::Register() {
			if (const auto scripts = RE::BSInputDeviceManager::GetSingleton()) {
				scripts->AddEventSink<RE::InputEvent*>(GetSingleton());
				logger::info("Registered for {}", typeid(RE::InputEvent).name());
			}
		}

		void Manager::GenerateAllTrigger(const KeyCombination* keys) {
			Regenerator::RegenerateAll();
		}

		void Manager::GenerateTargetTrigger(const KeyCombination*) {
			if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
				if (const auto actor = actorRef->As<RE::Actor>()) {
					Regenerator::RegenerateTarget(actor);
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

		void Manager::ToggleNamesTrigger(const KeyCombination*) {
			Options::General::enabled = !Options::General::enabled;
			// In case we're looking at someone when toggling names.
			RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
			logger::info("Toggled names {}", Options::General::enabled ? "ON"sv : "OFF"sv);
			Options::Save();
		}

		void Manager::FixStuckNameTrigger(const KeyCombination*) {
			if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
				if (const auto actor = actorRef->As<RE::Actor>()) {
					NameFixer::FixName(actor);
				}
			}
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
				toggleObscurity.Process(a_event) ||
				toggleNames.Process(a_event) ||
				fixStuckName.Process(a_event);

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
			if (!toggleNames.SetPattern(Options::Hotkeys::toggleNames))
				logger::error("Failed to set Key Combination '{}' for toggleNames", Options::Hotkeys::toggleNames);
			if (!fixStuckName.SetPattern(Options::Hotkeys::fixStuckName))
				logger::error("Failed to set Key Combination '{}' for fixStuckName", Options::Hotkeys::fixStuckName);
		}
	}

}
