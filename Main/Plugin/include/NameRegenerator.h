#pragma once
#include "Distributor.h"

namespace NND
{
	namespace Regenerator
	{
		namespace details
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

			inline RE::MessageBoxData* MakeMessageBox(const std::string& a_message) {
				const auto factoryManager = RE::MessageDataFactoryManager::GetSingleton();
				const auto uiStrHolder = RE::InterfaceStrings::GetSingleton();

				if (factoryManager && uiStrHolder) {
					if (const auto factory = factoryManager->GetCreator<RE::MessageBoxData>(uiStrHolder->messageBoxData)) {
						if (const auto messageBox = factory->Create()) {
							messageBox->unk4C = 4;
							messageBox->unk38 = 10;
							messageBox->bodyText = a_message;

							return messageBox;
						}
					}
				}

				return nullptr;
			}
		}
		inline void RegenerateAll() {
			if (const auto messageBox = details::MakeMessageBox(details::message)) {
				if (const auto gameSettings = RE::GameSettingCollection::GetSingleton()) {
					const auto sYesText = gameSettings->GetSetting("sYesText");
					const auto sNoText = gameSettings->GetSetting("sNoText");
					if (sYesText && sNoText) {
						messageBox->buttonText.push_back(sYesText->GetString());
						messageBox->buttonText.push_back(sNoText->GetString());

						messageBox->callback = RE::BSTSmartPointer<RE::IMessageBoxCallback>{ new details::GenerateNamesConfirmCallback() };
						messageBox->QueueMessage();
					}
				}
			}
		}

		inline void RegenerateTarget(const RE::Actor* actor)
		{
			logger::info("Resetting name for target..");
			Distribution::Manager::GetSingleton()->DeleteData(actor);
			// Immediately refresh the name for NPC we are looking at.
			RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
		}
	}
}
