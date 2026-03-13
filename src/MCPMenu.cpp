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
		namespace
		{
			constexpr ImVec4 PLACEHOLDER_COLOR = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

			SKSEMenuFramework::Model::InputEvent* g_inputEventHandler = nullptr;
			bool                                  g_showRegenerateConfirmation = false;

			void ShowTooltip(const char* text) {
				if (IsItemHovered()) {
					BeginTooltip();
					PushTextWrapPos(GetFontSize() * 35.0f);
					TextUnformatted(text);
					PopTextWrapPos();
					EndTooltip();
				}
			}

			std::string ReplaceFormatTokens(const std::string& format, const std::string& name, const std::string& title) {
				std::string result = format;
				size_t      pos = 0;

				while ((pos = result.find("[name]", pos)) != std::string::npos) {
					result.replace(pos, 6, name);
					pos += name.length();
				}

				pos = 0;
				while ((pos = result.find("[title]", pos)) != std::string::npos) {
					result.replace(pos, 7, title);
					pos += title.length();
				}

				pos = 0;
				while ((pos = result.find("[break]", pos)) != std::string::npos) {
					result.replace(pos, 7, "\n");
					pos += 1;
				}

				return result;
			}

			std::string ReplaceObscurityTokens(const std::string& format, const std::string& sex, const std::string& race) {
				std::string result = format;
				size_t      pos = 0;

				while ((pos = result.find("[sex]", pos)) != std::string::npos) {
					result.replace(pos, 5, sex);
					pos += sex.length();
				}

				pos = 0;
				while ((pos = result.find("[race]", pos)) != std::string::npos) {
					result.replace(pos, 6, race);
					pos += race.length();
				}

				return result;
			}

			struct HotkeyCapture
			{
				bool                     isCapturing = false;
				std::set<uint32_t>       capturedKeys;
				std::string*             targetPattern = nullptr;
				Hotkeys::KeyCombination* targetHotkey = nullptr;
				char*                    displayBuffer = nullptr;

				void StartCapture(std::string& pattern, Hotkeys::KeyCombination& hotkey, char* buffer) {
					isCapturing = true;
					capturedKeys.clear();
					targetPattern = &pattern;
					targetHotkey = &hotkey;
					displayBuffer = buffer;
				}

				void StopCapture(bool success = false) {
					if (success && targetPattern && targetHotkey && !capturedKeys.empty()) {
						Hotkeys::KeyCombination tempCombo([](const Hotkeys::KeyCombination*) {});
						tempCombo = Hotkeys::KeyCombination(capturedKeys, [](const Hotkeys::KeyCombination*) {});

						*targetPattern = std::string(tempCombo.GetPattern());
						if (displayBuffer) {
							strcpy_s(displayBuffer, 64, targetPattern->c_str());
						}
						targetHotkey->SetPattern(*targetPattern);
						Options::Save();
						logger::info("Captured hotkey: {}", *targetPattern);
					}

					isCapturing = false;
					capturedKeys.clear();
					targetPattern = nullptr;
					targetHotkey = nullptr;
					displayBuffer = nullptr;
				}

				bool ProcessInput(RE::InputEvent* event) {
					if (!isCapturing) {
						return false;
					}

					auto button = event->AsButtonEvent();
					if (!button || !button->HasIDCode()) {
						return false;
					}

					auto key = button->GetIDCode();
					switch (button->GetDevice()) {
					case RE::INPUT_DEVICE::kMouse:
						key += SKSE::InputMap::kMacro_MouseButtonOffset;
						break;
					case RE::INPUT_DEVICE::kGamepad:
						key = SKSE::InputMap::GamepadMaskToKeycode(key);
						break;
					default:
						break;
					}

					if (button->IsPressed()) {
						capturedKeys.insert(key);
					} else if (button->IsUp() && !capturedKeys.empty()) {
						StopCapture(true);
						return true;
					}

					return true;
				}
			};

			HotkeyCapture g_hotkeyCapture;

			bool InputEventCallback(RE::InputEvent* event) {
				return g_hotkeyCapture.ProcessInput(event);
			}

			std::string SortHotkeyModifiers(const std::string& hotkey) {
				std::vector<std::string> parts;
				size_t                   start = 0;
				size_t                   end = hotkey.find(" + ");

				while (end != std::string::npos) {
					parts.push_back(hotkey.substr(start, end - start));
					start = end + 3;
					end = hotkey.find(" + ", start);
				}
				parts.push_back(hotkey.substr(start));

				auto isModifier = [](const std::string& key) {
					return key.find("ctrl") != std::string::npos ||
					       key.find("shift") != std::string::npos ||
					       key.find("alt") != std::string::npos;
				};

				std::vector<std::string> modifiers;
				std::vector<std::string> keys;

				for (const auto& part : parts) {
					std::string lower = part;
					std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
					if (isModifier(lower)) {
						modifiers.push_back(part);
					} else {
						keys.push_back(part);
					}
				}

				std::string result;
				for (const auto& mod : modifiers) {
					if (!result.empty())
						result += " + ";
					result += mod;
				}
				for (const auto& key : keys) {
					if (!result.empty())
						result += " + ";
					result += key;
				}

				return result;
			}

			void RegenerateAllNamesFromMenu() {
				Regenerator::details::GenerateNamesConfirmCallback().Run(static_cast<RE::IMessageBoxCallback::Message>(4));
			}
		}

		void RenderGeneralSection() {
			if (Checkbox("Enable Names Distribution", &Options::General::enabled)) {
				Options::Save();
				NND::UpdateCrosshairs();
				logger::info("Names distribution {}", Options::General::enabled ? "enabled" : "disabled");
			}
			ShowTooltip("Enables or disables Names and Titles. Note: Names are still being generated, just not displayed.");
			Spacing();
			Separator();
			Spacing();

			if (CollapsingHeader("Display Name Format", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();

				if (!Options::General::enabled) {
					BeginDisabled();
				}

				static int  selectedPreset = -1;
				static char formatBuffer[512] = "";
				static bool bufferInitialized = false;
				static bool showCustomInput = false;

				if (IsWindowAppearing() || !bufferInitialized) {
					selectedPreset = -1;
					for (size_t i = 0; i < Options::DisplayName::defaultFormats.size(); ++i) {
						if (Options::DisplayName::format == Options::DisplayName::defaultFormats[i]) {
							selectedPreset = static_cast<int>(i);
							break;
						}
					}

					if (selectedPreset == -1) {
						selectedPreset = static_cast<int>(Options::DisplayName::defaultFormats.size());
						showCustomInput = true;
					} else {
						showCustomInput = false;
					}

					strcpy_s(formatBuffer, Options::DisplayName::format.c_str());
					bufferInitialized = true;
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
						strcpy_s(formatBuffer, Options::DisplayName::format.c_str());
						showCustomInput = false;
						Options::Save();
						logger::info("Display format changed to preset: {}", Options::DisplayName::format);
					} else {
						showCustomInput = true;
					}
				}
				ShowTooltip("Select from predefined name format templates");

				SameLine();
				if (Button("Reset##PresetFormats", ImVec2(60, 0))) {
					Options::DisplayName::format = "[name] ([title])";
					selectedPreset = 2;
					strcpy_s(formatBuffer, "[name] ([title])");
					showCustomInput = false;
					Options::Save();
					logger::info("Display format reset to default");
				}
				ShowTooltip("Reset to default format");
				Spacing();

				if (showCustomInput) {
					Text("Custom Format");

					PushStyleColor(ImGuiCol_TextDisabled, PLACEHOLDER_COLOR);
					if (InputTextWithHint("##CustomFormat", "[name] ([title])", formatBuffer, sizeof(formatBuffer))) {
						if (strlen(formatBuffer) > 0) {
							Options::DisplayName::format = formatBuffer;
							Options::Save();
							logger::info("Display format changed to custom: {}", formatBuffer);
						} else {
							Options::DisplayName::format = "[name] ([title])";
							Options::Save();
							logger::info("Display format reset to default due to empty input");
						}
					}
					PopStyleColor();
					ShowTooltip("Supports: [name], [title], [break] (newline). Leave empty for default format.");
					Spacing();
				}

				Text("Preview:");
				SameLine();
				std::string previewFormat = (strlen(formatBuffer) > 0) ? formatBuffer : "[name] ([title])";
				std::string preview = ReplaceFormatTokens(previewFormat, "Torvar Stormcloak", "Whiterun Guard");
				TextWrapped("%s", preview.c_str());

				if (!Options::General::enabled) {
					EndDisabled();
				}

				Unindent();
				Spacing();
				Separator();
			}

			if (CollapsingHeader("Name Contexts", ImGuiTreeNodeFlags_DefaultOpen)) {
				Indent();

				if (!Options::General::enabled) {
					BeginDisabled();
				}

				const char* styleNames[] = { "Display Name", "Full Name", "Short Name", "Title" };

				auto RenderStyleCombo = [&](const char* label, NameStyle& setting, NameStyle defaultValue, const char* tooltip) {
					int current = static_cast<int>(setting);
					Text("%s", label);
					SameLine(200);
					PushItemWidth(180);
					if (Combo(("##" + std::string(label)).c_str(), &current, styleNames, 4)) {
						setting = static_cast<NameStyle>(current);
						Options::Save();
						NND::UpdateCrosshairs();
					}
					ShowTooltip(tooltip);
					PopItemWidth();

					SameLine();
					if (Button(("Reset##" + std::string(label)).c_str(), ImVec2(60, 0))) {
						setting = defaultValue;
						Options::Save();
						NND::UpdateCrosshairs();
					}
					ShowTooltip("Reset to default");
				};

				RenderStyleCombo("Crosshair", Options::NameContext::kCrosshair, kDisplayName, "Name shown under crosshair when looking at NPC");
				RenderStyleCombo("Crosshair Minion", Options::NameContext::kCrosshairMinion, kTitle, "Name shown under crosshair when looking at your summons/raised zombies.");
				RenderStyleCombo("Subtitles", Options::NameContext::kSubtitles, kShortName, "Name shown for speaker in dialogue subtitles");
				RenderStyleCombo("Dialogue", Options::NameContext::kDialogue, kFullName, "Name of the NPC that you're in dialogue with");
				RenderStyleCombo("Dialogue History", Options::NameContext::kDialogueHistory, kFullName, "Name shown for speaker in dialogue history");
				RenderStyleCombo("Inventory", Options::NameContext::kInventory, kFullName, "Name shown when interacting with NPC's inventory");
				RenderStyleCombo("Barter", Options::NameContext::kBarter, kShortName, "Name shown during barter (next to amount of gold)");
				RenderStyleCombo("Enemy HUD", Options::NameContext::kEnemyHUD, kFullName, "Name shown in enemy health bars");
				RenderStyleCombo("Other", Options::NameContext::kOther, kFullName, "Name shown in other contexts, such as console, notifications, etc.");

				if (!Options::General::enabled) {
					EndDisabled();
				}

				Spacing();
				Separator();
				Spacing();

				if (CollapsingHeader("Name Styles")) {
					Indent();

					if (BeginTable("NameStyles", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
						TableSetupColumn("Style", ImGuiTableColumnFlags_WidthFixed, 150.0f);
						TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
						TableHeadersRow();

						TableNextRow();
						TableSetColumnIndex(0);
						Text("Display Name");
						TableSetColumnIndex(1);
						TextWrapped("Shows all available components of the name: Full Name and Title.");

						TableNextRow();
						TableSetColumnIndex(0);
						Text("Full Name");
						TableSetColumnIndex(1);
						TextWrapped("Shows the full name, consisting of First, Middle and Last names.");

						TableNextRow();
						TableSetColumnIndex(0);
						Text("Short Name");
						TableSetColumnIndex(1);
						TextWrapped("Shows a shortened version of a name. Short names might include only some parts of the name.");

						TableNextRow();
						TableSetColumnIndex(0);
						Text("Title");
						TableSetColumnIndex(1);
						TextWrapped("Shows only Title (e.g. Whiterun Guard)");

						EndTable();
					}

					Unindent();
				}

				Unindent();
				Spacing();
			}
		}

		void RenderObscuritySection() {
			if (Checkbox("Enable Obscurity", &Options::Obscurity::enabled)) {
				Options::Save();
				NND::UpdateCrosshairs();
				logger::info("Obscurity {}", Options::Obscurity::enabled ? "enabled" : "disabled");
			}
			ShowTooltip("Hide NPC names until they are revealed by interacting with them");
			Spacing();
			Separator();
			Spacing();

			if (!Options::Obscurity::enabled) {
				BeginDisabled();
			}

			static char defaultNameBuffer[256] = "";
			static bool bufferInitialized = false;

			if (IsWindowAppearing() || !bufferInitialized) {
				strcpy_s(defaultNameBuffer, Options::Obscurity::defaultName.data());
				bufferInitialized = true;
			}

			Text("Default Name Format");

			PushStyleColor(ImGuiCol_TextDisabled, PLACEHOLDER_COLOR);
			if (InputTextWithHint("##DefaultName", "[sex] [race]", defaultNameBuffer, sizeof(defaultNameBuffer))) {
				if (strlen(defaultNameBuffer) > 0) {
					Options::Obscurity::defaultName = defaultNameBuffer;
					Options::Save();
				} else {
					Options::Obscurity::defaultName = "[sex] [race]";
					Options::Save();
					logger::info("Obscurity format reset to default due to empty input");
				}
			}
			PopStyleColor();
			ShowTooltip("Format for obscured names. Supports: [sex], [race]. Leave empty for default format.");

			SameLine();
			if (Button("Reset##DefaultName", ImVec2(60, 0))) {
				strcpy_s(defaultNameBuffer, "[sex] [race]");
				Options::Obscurity::defaultName = "[sex] [race]";
				Options::Save();
				logger::info("Obscurity format reset to default");
			}
			ShowTooltip("Reset to default format");

			Spacing();
			Text("Preview:");
			SameLine();
			std::string previewFormat = (strlen(defaultNameBuffer) > 0) ? defaultNameBuffer : "[sex] [race]";
			std::string obscurityPreview = ReplaceObscurityTokens(previewFormat, "Male", "Nord");
			TextWrapped("%s", obscurityPreview.c_str());
			Spacing();

			Separator();
			Spacing();

			Text("Revealing Names");
			Spacing();

			if (Checkbox("Reveal in Greetings", &Options::Obscurity::greetings)) {
				Options::Save();
			}
			ShowTooltip("Reveal names when NPC greets you from a distance");
			Spacing();

			if (Checkbox("Reveal when Looting", &Options::Obscurity::obituary)) {
				Options::Save();
			}
			ShowTooltip("Reveal names when looting dead NPCs");
			Spacing();

			if (Checkbox("Reveal when Pickpocketing", &Options::Obscurity::stealing)) {
				Options::Save();
			}
			ShowTooltip("Reveal names when pickpocketing NPCs");

			if (!Options::Obscurity::enabled) {
				EndDisabled();
			}
		}

		void RenderHotkeysSection() {
			TextWrapped("Click 'Capture' button and press desired key combination, or manually type it");
			ShowTooltip("Format: 'Key' or 'Modifier+Key' (e.g., 'RCtrl+G', 'RCtrl+RShift+G')");
			Spacing();

			auto* manager = Hotkeys::Manager::GetSingleton();

			static char toggleNamesBuffer[64] = "";
			static char toggleObscurityBuffer[64] = "";
			static char generateAllBuffer[64] = "";
			static char generateTargetBuffer[64] = "";
			static char reloadSettingsBuffer[64] = "";
			static char fixStuckNameBuffer[64] = "";
			static char unsafeFixStuckNameBuffer[64] = "";
			static bool buffersInitialized = false;

			if (IsWindowAppearing() || !buffersInitialized) {
				strcpy_s(toggleNamesBuffer, manager->toggleNames.GetPattern().data());
				strcpy_s(toggleObscurityBuffer, manager->toggleObscurity.GetPattern().data());
				strcpy_s(generateAllBuffer, manager->generateAll.GetPattern().data());
				strcpy_s(generateTargetBuffer, manager->generateTarget.GetPattern().data());
				strcpy_s(reloadSettingsBuffer, manager->reloadSettings.GetPattern().data());
				strcpy_s(fixStuckNameBuffer, manager->fixStuckName.GetPattern().data());
				strcpy_s(unsafeFixStuckNameBuffer, manager->unsafeFixStuckName.GetPattern().data());
				buffersInitialized = true;
			}

			auto CapitalizeHotkey = [](const std::string& hotkey) -> std::string {
				std::string result = SortHotkeyModifiers(hotkey);
				size_t      pos = 0;

				while (pos < result.length()) {
					if (pos == 0 || result[pos - 1] == ' ' || result[pos - 1] == '+') {
						if (result[pos] >= 'a' && result[pos] <= 'z') {
							result[pos] = result[pos] - 32;
						}
					}
					pos++;
				}

				pos = 0;
				while ((pos = result.find("rshift", pos)) != std::string::npos) {
					result.replace(pos, 6, "RShift");
					pos += 6;
				}

				pos = 0;
				while ((pos = result.find("rctrl", pos)) != std::string::npos) {
					result.replace(pos, 5, "RCtrl");
					pos += 5;
				}

				pos = 0;
				while ((pos = result.find("ralt", pos)) != std::string::npos) {
					result.replace(pos, 4, "RAlt");
					pos += 4;
				}

				pos = 0;
				while ((pos = result.find("lshift", pos)) != std::string::npos) {
					result.replace(pos, 6, "LShift");
					pos += 6;
				}

				pos = 0;
				while ((pos = result.find("lctrl", pos)) != std::string::npos) {
					result.replace(pos, 5, "LCtrl");
					pos += 5;
				}

				pos = 0;
				while ((pos = result.find("lalt", pos)) != std::string::npos) {
					result.replace(pos, 4, "LAlt");
					pos += 4;
				}

				return result;
			};

			auto RenderHotkeyInput = [&](const char* label, char* buffer, std::string& pattern, Hotkeys::KeyCombination& hotkey, const char* defaultPattern, const char* tooltip) {
				Text("%s", label);
				SameLine(250);

				PushItemWidth(180);
				bool isCapturing = g_hotkeyCapture.isCapturing && g_hotkeyCapture.targetPattern == &pattern;

				if (isCapturing) {
					PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.5f, 0.3f, 1.0f));
					char captureText[32] = "Press keys...";
					InputText(("##" + std::string(label)).c_str(), captureText, sizeof(captureText), ImGuiInputTextFlags_ReadOnly);
					PopStyleColor();
				} else {
					std::string displayValue = CapitalizeHotkey(buffer);
					char        displayBuffer[64];
					strcpy_s(displayBuffer, displayValue.c_str());

					PushStyleColor(ImGuiCol_TextDisabled, PLACEHOLDER_COLOR);
					if (InputTextWithHint(("##" + std::string(label)).c_str(), CapitalizeHotkey(defaultPattern).c_str(), displayBuffer, sizeof(displayBuffer))) {
						if (strlen(displayBuffer) > 0) {
							if (hotkey.SetPattern(displayBuffer)) {
								pattern = displayBuffer;
								strcpy_s(buffer, 64, std::string(hotkey.GetPattern()).c_str());
								Options::Save();
								logger::info("Updated '{}' hotkey to '{}'", label, buffer);
							} else {
								logger::error("Failed to set '{}' as hotkey pattern for '{}'", displayBuffer, label);
								strcpy_s(displayBuffer, std::string(hotkey.GetPattern()).c_str());
							}
						} else {
							pattern = defaultPattern;
							strcpy_s(buffer, 64, defaultPattern);
							hotkey.SetPattern(pattern);
							Options::Save();
							logger::info("Reset '{}' hotkey to default due to empty input: '{}'", label, defaultPattern);
						}
					}
					PopStyleColor();
				}
				PopItemWidth();
				ShowTooltip(tooltip);

				SameLine();
				if (Button(("Capture##" + std::string(label)).c_str(), ImVec2(80, 0))) {
					if (isCapturing) {
						g_hotkeyCapture.StopCapture(false);
					} else {
						g_hotkeyCapture.StartCapture(pattern, hotkey, buffer);
					}
				}
				ShowTooltip(isCapturing ? "Click to cancel capture" : "Click and press keys to capture hotkey");

				SameLine();
				if (Button(("Reset##" + std::string(label)).c_str(), ImVec2(60, 0))) {
					pattern = defaultPattern;
					strcpy_s(buffer, 64, defaultPattern);
					hotkey.SetPattern(pattern);
					Options::Save();
					logger::info("Reset '{}' hotkey to default: '{}'", label, defaultPattern);
				}
				ShowTooltip("Reset to default hotkey");
			};

			RenderHotkeyInput("Toggle Names", toggleNamesBuffer, Options::Hotkeys::toggleNames, manager->toggleNames, "RCtrl+N", "Toggle names distribution on/off");
			RenderHotkeyInput("Toggle Obscurity", toggleObscurityBuffer, Options::Hotkeys::toggleObscurity, manager->toggleObscurity, "RCtrl+O", "Toggle obscurity feature on/off");
			RenderHotkeyInput("Generate All Names", generateAllBuffer, Options::Hotkeys::generateAll, manager->generateAll, "RCtrl+RShift+G", "Regenerate names for all NPCs");
			RenderHotkeyInput("Generate Target Name", generateTargetBuffer, Options::Hotkeys::generateTarget, manager->generateTarget, "RCtrl+G", "Regenerate name for targeted NPC");
			RenderHotkeyInput("Reload Settings", reloadSettingsBuffer, Options::Hotkeys::reloadSettings, manager->reloadSettings, "RCtrl+L", "Reload settings from INI file");
			RenderHotkeyInput("Fix Stuck Name", fixStuckNameBuffer, Options::Hotkeys::fixStuckName, manager->fixStuckName, "RCtrl+Backspace", "Fix stuck name for targeted NPC (safe)");
			RenderHotkeyInput("Unsafe Fix Stuck Name", unsafeFixStuckNameBuffer, Options::Hotkeys::unsafeFixStuckName, manager->unsafeFixStuckName, "RCtrl+RShift+Backspace", "Force fix stuck name for targeted NPC (unsafe)");
		}

		void RenderActionsSection() {
			bool isPlayerLoaded = RE::PlayerCharacter::GetSingleton() != nullptr && RE::PlayerCharacter::GetSingleton()->Is3DLoaded();

			if (!isPlayerLoaded) {
				BeginDisabled();
			}

			if (Button("Generate All Names", ImVec2(250, 0))) {
				g_showRegenerateConfirmation = true;
			}
			ShowTooltip("Regenerate names for all tracked NPCs");
			Spacing();

			if (Button("Generate Target Name", ImVec2(250, 0))) {
				if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
					if (const auto actor = actorRef->As<RE::Actor>()) {
						Regenerator::RegenerateTarget(actor);
					}
				}
			}
			ShowTooltip("Regenerate name for the NPC you're currently looking at");
			Spacing();

			if (Button("Reload Settings", ImVec2(250, 0))) {
				Options::Load();

				auto manager = Distribution::Manager::GetSingleton();
				manager->UpdateNames([&manager](auto& names) {
					for (auto& pair : names) {
						if (const auto actor = RE::TESForm::LookupByID(pair.first);
						    actor && actor->formType == RE::FormType::ActorCharacter) {
#ifdef DEV
							manager->UpdateData(pair.second, actor->As<RE::Actor>(), false, true);
#else
							manager->UpdateData(pair.second, actor->As<RE::Actor>(), false);
#endif
						}
					}
				});

				NND::UpdateCrosshairs();
				logger::info("Settings reloaded via menu");
			}
			ShowTooltip("Reload all settings from the INI file");
			Spacing();

			Separator();
			Spacing();
			TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Legacy Functions (for pre-NND 2.0 saves)");
			Spacing();

			if (Button("Fix Stuck Name (Target)", ImVec2(250, 0))) {
				if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
					if (const auto actor = actorRef->As<RE::Actor>()) {
						NameFixer::FixName(actor);
						logger::info("Fixed stuck name for target NPC");
					}
				}
			}
			ShowTooltip("Fix stuck name for targeted NPC (safe method)");
			Spacing();

			if (Button("Unsafe Fix Stuck Name (Target)", ImVec2(250, 0))) {
				if (const auto actorRef = RE::CrosshairPickData::GetSingleton()->targetActor.get().get()) {
					if (const auto actor = actorRef->As<RE::Actor>()) {
						NameFixer::FixNameUnsafe(actor);
						logger::warn("Unsafe fix applied to stuck name for target NPC");
					}
				}
			}
			ShowTooltip("UNSAFE: Force fix stuck name for targeted NPC");

			if (!isPlayerLoaded) {
				EndDisabled();
				Spacing();
				TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Actions are disabled in main menu. Load a game to enable.");
			}

			if (g_showRegenerateConfirmation) {
				OpenPopup("Regenerate All Names?");
			}

			if (BeginPopupModal("Regenerate All Names?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				TextWrapped("This action will regenerate names for all tracked NPCs.");
				TextWrapped("You might experience some lag during the process.");
				Spacing();
				TextWrapped("Proceed?");
				Spacing();
				TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(You can reload your last save to undo this action.)");
				Spacing();
				Separator();
				Spacing();

				if (Button("Yes", ImVec2(120, 0))) {
					RegenerateAllNamesFromMenu();
					CloseCurrentPopup();
					g_showRegenerateConfirmation = false;
				}
				SetItemDefaultFocus();
				SameLine();
				if (Button("No", ImVec2(120, 0))) {
					CloseCurrentPopup();
					g_showRegenerateConfirmation = false;
				}

				EndPopup();
			}
		}

		void Register() {
			if (!SKSEMenuFramework::IsInstalled()) {
				logger::warn("SKSEMenuFramework is not installed");
				return;
			}

			SKSEMenuFramework::SetSection("NPCs Names Distributor");

			g_inputEventHandler = SKSEMenuFramework::AddInputEvent(InputEventCallback);

			SKSEMenuFramework::AddSectionItem("General", RenderGeneralSection);
			SKSEMenuFramework::AddSectionItem("Obscurity", RenderObscuritySection);
			SKSEMenuFramework::AddSectionItem("Hotkeys", RenderHotkeysSection);
			SKSEMenuFramework::AddSectionItem("Actions", RenderActionsSection);

			logger::info("Added section to SKSEMenuFramework");
		}
	}
}
