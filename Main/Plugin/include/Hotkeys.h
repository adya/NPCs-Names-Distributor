#pragma once
#include "CLIBUtil/hotkeys.hpp"
#include "Options.h"

namespace NND
{
	namespace Hotkeys
	{
		using namespace clib_util::hotkeys;
		class Manager : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			static Manager* GetSingleton() {
				static Manager singleton;
				return &singleton;
			}

			static void Register();

			static void GenerateAllTrigger(const KeyCombination*);
			static void GenerateTargetTrigger(const KeyCombination*);
			static void ReloadSettingsTrigger(const KeyCombination*);
			static void ToggleObscurityTrigger(const KeyCombination*);
			static void ToggleNamesTrigger(const KeyCombination*);

			static void FixStuckNameTrigger(const KeyCombination*);

			KeyCombination generateAll{ GenerateAllTrigger };
			KeyCombination generateTarget{ GenerateTargetTrigger };
			KeyCombination reloadSettings{ ReloadSettingsTrigger };
			KeyCombination toggleObscurity{ ToggleObscurityTrigger };
			KeyCombination toggleNames{ ToggleNamesTrigger };

			KeyCombination fixStuckName{ FixStuckNameTrigger };

		protected:
			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

		private:
			// Singleton stuff :)
			Manager();
			Manager(const Manager&) = delete;
			Manager(Manager&&) = delete;

			~Manager() override = default;

			Manager& operator=(const Manager&) = delete;
			Manager& operator=(Manager&&) = delete;
		};
	}
}
