#include "MCPMenu.h"
#include "Distributor.h"
#include "Hotkeys.h"
#include "NameFixer.h"
#include "NameRegenerator.h"
#include "Options.h"

#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NND
{
	namespace Menu
	{
		void RenderGeneralSection() {
			Text("General Settings");
			Separator();
			Spacing();

			if (CollapsingHeader("Names Distribution", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();
				
				if (Checkbox("Enable Names Distribution", &Options::General::enabled)) {
					Options::Save();
					NND::UpdateCrosshairs();
					logger::info("Names distribution {}", Options::General::enabled ? "enabled" : "disabled");
				}
				TextWrapped("Enable or disable the entire NPCs Names Distributor functionality");
				
				Unindent();
				Spacing();
			}

			if (CollapsingHeader("Display Name Format", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();

				static int selectedPreset = -1;

				if (IsWindowAppearing()) {
					selectedPreset = -1;
					for (size_t i = 0; i < Options::DisplayName::defaultFormats.size(); ++i) {
						if (Options::DisplayName::format == Options::DisplayName::defaultFormats[i]) {
							selectedPreset = static_cast<int>(i);
							break;
						}
					}
				}

				Text("Preset Formats");
				const char* formatNames[] = {
					"[name]",
					"[name][break][title]",
					"[name] ([title])",
					"[name] [[title]]",
					"[name], [title]",
					"[name]; [title]",
					"[name]. [title]",
					"[name] [title]",
					"Custom"
				};

				if (Combo("##PresetFormats", &selectedPreset, formatNames, 9)) {
					if (selectedPreset >= 0 && selectedPreset < static_cast<int>(Options::DisplayName::defaultFormats.size())) {
						Options::DisplayName::format = std::string(Options::DisplayName::defaultFormats[selectedPreset]);
						Options::Save();
						logger::info("Display format changed to preset: {}", Options::DisplayName::format);
					}
				}
				Spacing();

				static char formatBuffer[512];
				if (IsWindowAppearing()) {
					strcpy_s(formatBuffer, Options::DisplayName::format.c_str());
				}

				Text("Custom Format");
				if (InputText("##CustomFormat", formatBuffer, sizeof(formatBuffer))) {
					if (strlen(formatBuffer) > 0) {
						Options::DisplayName::format = formatBuffer;
						selectedPreset = static_cast<int>(Options::DisplayName::defaultFormats.size());
						Options::Save();
						logger::info("Display format changed to custom: {}", formatBuffer);
					}
				}
				TextWrapped("Supports: [name], [title], [break] (newline)");

				Unindent();
				Spacing();
			}

			if (CollapsingHeader("Name Contexts", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();

				const char* styleNames[] = { "Display Name", "Full Name", "Short Name", "Title" };

				auto RenderStyleCombo = [&](const char* label, NameStyle& setting) {
					int current = static_cast<int>(setting);
					Text("%s", label);
					SameLine(200);
					PushItemWidth(-1);
					if (Combo(("##" + std::string(label)).c_str(), &current, styleNames, 4)) {
						setting = static_cast<NameStyle>(current);
						Options::Save();
						NND::UpdateCrosshairs();
					}
					PopItemWidth();
				};

				RenderStyleCombo("Crosshair", Options::NameContext::kCrosshair);
				RenderStyleCombo("Crosshair Minion", Options::NameContext::kCrosshairMinion);
				RenderStyleCombo("Subtitles", Options::NameContext::kSubtitles);
				RenderStyleCombo("Dialogue", Options::NameContext::kDialogue);
				RenderStyleCombo("Dialogue History", Options::NameContext::kDialogueHistory);
				RenderStyleCombo("Inventory", Options::NameContext::kInventory);
				RenderStyleCombo("Barter", Options::NameContext::kBarter);
				RenderStyleCombo("Enemy HUD", Options::NameContext::kEnemyHUD);
				RenderStyleCombo("Other", Options::NameContext::kOther);

				Unindent();
				Spacing();
			}
		}

		void RenderObscuritySection() {
			Text("Obscurity Settings");
			Separator();
			Spacing();

			if (Checkbox("Enable Obscurity", &Options::Obscurity::enabled)) {
				Options::Save();
				NND::UpdateCrosshairs();
				logger::info("Obscurity {}", Options::Obscurity::enabled ? "enabled" : "disabled");
			}
			TextWrapped("Hide NPC names until they are revealed through gameplay");
			Spacing();

			if (Checkbox("Reveal in Greetings", &Options::Obscurity::greetings)) {
				Options::Save();
			}
			TextWrapped("Reveal names during NPC or Player greetings");
			Spacing();

			if (Checkbox("Reveal when Looting", &Options::Obscurity::obituary)) {
				Options::Save();
			}
			TextWrapped("Reveal names when looting dead NPCs");
			Spacing();

			if (Checkbox("Reveal when Pickpocketing", &Options::Obscurity::stealing)) {
				Options::Save();
			}
			TextWrapped("Reveal names during pickpocket attempts");
			Spacing();

			Separator();
			Spacing();

			static char defaultNameBuffer[256];
			if (IsWindowAppearing()) {
				strcpy_s(defaultNameBuffer, Options::Obscurity::defaultName.data());
			}

			Text("Default Name Format");
			if (InputText("##DefaultName", defaultNameBuffer, sizeof(defaultNameBuffer))) {
				if (strlen(defaultNameBuffer) > 0) {
					Options::Obscurity::defaultName = defaultNameBuffer;
					Options::Save();
				}
			}
			TextWrapped("Format for obscured names. Supports: [sex], [race]");
		}

		void RenderHotkeysSection() {
			Text("Hotkey Settings");
			Separator();
			Spacing();
			TextWrapped("Format: 'Key' or 'Modifier+Key' (e.g., 'RCtrl+G', 'RCtrl+RShift+G')");
			Spacing();

			auto* manager = Hotkeys::Manager::GetSingleton();

			static char toggleNamesBuffer[64];
			static char toggleObscurityBuffer[64];
			static char generateAllBuffer[64];
			static char generateTargetBuffer[64];
			static char reloadSettingsBuffer[64];
			static char fixStuckNameBuffer[64];
			static char unsafeFixStuckNameBuffer[64];

			if (IsWindowAppearing()) {
				strcpy_s(toggleNamesBuffer, Options::Hotkeys::toggleNames.c_str());
				strcpy_s(toggleObscurityBuffer, Options::Hotkeys::toggleObscurity.c_str());
				strcpy_s(generateAllBuffer, Options::Hotkeys::generateAll.c_str());
				strcpy_s(generateTargetBuffer, Options::Hotkeys::generateTarget.c_str());
				strcpy_s(reloadSettingsBuffer, Options::Hotkeys::reloadSettings.c_str());
				strcpy_s(fixStuckNameBuffer, Options::Hotkeys::fixStuckName.c_str());
				strcpy_s(unsafeFixStuckNameBuffer, Options::Hotkeys::unsafeFixStuckName.c_str());
			}

			auto RenderHotkeyInput = [](const char* label, char* buffer, std::string& pattern, Hotkeys::KeyCombination& hotkey) {
				Text("%s", label);
				if (InputText(("##" + std::string(label)).c_str(), buffer, 64)) {
					if (hotkey.SetPattern(buffer)) {
						pattern = buffer;
						Options::Save();
						RE::DebugNotification("Hotkey updated successfully");
						logger::info("Updated '{}' hotkey to '{}'", label, buffer);
					} else {
						RE::DebugNotification("Invalid hotkey pattern");
						logger::error("Failed to set '{}' as hotkey pattern for '{}'", buffer, label);
					}
				}
			};

			RenderHotkeyInput("Toggle Names", toggleNamesBuffer, Options::Hotkeys::toggleNames, manager->toggleNames);
			RenderHotkeyInput("Toggle Obscurity", toggleObscurityBuffer, Options::Hotkeys::toggleObscurity, manager->toggleObscurity);
			RenderHotkeyInput("Generate All Names", generateAllBuffer, Options::Hotkeys::generateAll, manager->generateAll);
			RenderHotkeyInput("Generate Target Name", generateTargetBuffer, Options::Hotkeys::generateTarget, manager->generateTarget);
			RenderHotkeyInput("Reload Settings", reloadSettingsBuffer, Options::Hotkeys::reloadSettings, manager->reloadSettings);
			RenderHotkeyInput("Fix Stuck Name", fixStuckNameBuffer, Options::Hotkeys::fixStuckName, manager->fixStuckName);
			RenderHotkeyInput("Unsafe Fix Stuck Name", unsafeFixStuckNameBuffer, Options::Hotkeys::unsafeFixStuckName, manager->unsafeFixStuckName);
		}

		void RenderActionsSection() {
			Text("Quick Actions");
			Separator();
			Spacing();

			if (Button("Generate All Names", ImVec2(250, 0))) {
				Regenerator::RegenerateAll();
			}
			TextWrapped("Regenerate names for all NPCs (shows confirmation dialog)");
			Spacing();

			if (Button("Generate Target Name", ImVec2(250, 0))) {
				if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
					if (const auto actor = actorRef->As<RE::Actor>()) {
						Regenerator::RegenerateTarget(actor);
						RE::DebugNotification("Target NPC name regenerated");
					} else {
						RE::DebugNotification("Target is not an NPC");
					}
				} else {
					RE::DebugNotification("No target selected");
				}
			}
			TextWrapped("Regenerate name for the NPC you're currently looking at");
			Spacing();

			if (Button("Reload Settings", ImVec2(250, 0))) {
				Options::Load();

				auto manager = Distribution::Manager::GetSingleton();
				manager->UpdateNames([&manager](auto& names) {
					for (auto& pair : names) {
						if (const auto actor = RE::TESForm::LookupByID(pair.first);
						    actor && actor->formType == RE::FormType::ActorCharacter) {
#ifndef NDEBUG
							manager->UpdateData(pair.second, actor->As<RE::Actor>(), false, true);
#else
							manager->UpdateData(pair.second, actor->As<RE::Actor>(), false);
#endif
						}
					}
				});

				NND::UpdateCrosshairs();
				RE::DebugNotification("Settings reloaded from INI");
				logger::info("Settings reloaded via menu");
			}
			TextWrapped("Reload all settings from the INI file");
			Spacing();

			Separator();
			Spacing();
			TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Legacy Functions (for pre-NND 2.0 saves)");
			Spacing();

			if (Button("Fix Stuck Name (Target)", ImVec2(250, 0))) {
				if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
					if (const auto actor = actorRef->As<RE::Actor>()) {
						NameFixer::FixName(actor);
						RE::DebugNotification("Attempted to fix stuck name");
						logger::info("Fixed stuck name for target NPC");
					} else {
						RE::DebugNotification("Target is not an NPC");
					}
				} else {
					RE::DebugNotification("No target selected");
				}
			}
			TextWrapped("Fix stuck name for targeted NPC (safe method)");
			Spacing();

			if (Button("Unsafe Fix Stuck Name (Target)", ImVec2(250, 0))) {
				if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
					if (const auto actor = actorRef->As<RE::Actor>()) {
						NameFixer::FixNameUnsafe(actor);
						RE::DebugNotification("Applied unsafe fix to stuck name");
						logger::warn("Unsafe fix applied to stuck name for target NPC");
					} else {
						RE::DebugNotification("Target is not an NPC");
					}
				} else {
					RE::DebugNotification("No target selected");
				}
			}
			TextWrapped("UNSAFE: Force fix stuck name for targeted NPC");
		}

		void Register() {
			if (!SKSEMenuFramework::IsInstalled()) {
				logger::warn("SKSEMenuFramework is not installed");
				return;
			}

			SKSEMenuFramework::SetSection("NPCs Names Distributor");
			
			SKSEMenuFramework::AddSectionItem("General", RenderGeneralSection);
			SKSEMenuFramework::AddSectionItem("Obscurity", RenderObscuritySection);
			SKSEMenuFramework::AddSectionItem("Hotkeys", RenderHotkeysSection);
			SKSEMenuFramework::AddSectionItem("Actions", RenderActionsSection);
			
			logger::info("Added section to SKSEMenuFramework");
		}
	}
}
