#include "Distributor.h"
#include "LookupNameDefinitions.h"
#include "NNDKeywords.h"
#include "Persistency.h"

namespace NND
{
	using Scope = NameDefinition::Scope;

	// NNDData
	namespace Distribution
	{
		// Here we'll handle all styles and whatnot.
		void NNDData::UpdateDisplayName(RE::Actor* actor) {
			const Name effectiveTitle(GetTitle(actor));
			if (name != empty && effectiveTitle != empty) {
				const NameRef effectiveShortName = shortDisplayName != empty ? shortDisplayName : name;
				Name          formattedDisplayName{ Options::DisplayName::format };
				if (formattedDisplayName != empty) {
					clib_util::string::replace_first_instance(formattedDisplayName, "[name]", name);
					clib_util::string::replace_first_instance(formattedDisplayName, "[short]", effectiveShortName);
					clib_util::string::replace_first_instance(formattedDisplayName, "[title]", effectiveTitle);
					clib_util::string::replace_first_instance(formattedDisplayName, "[break]", "\n");
					displayName = formattedDisplayName;
				} else {
					displayName = name + " (" + effectiveTitle + ")";
				}
			} else if (name != empty) {
				displayName = name;
			} else if (this->title != empty) {
				if (isUnique) {
					// If we have a custom title and actor is unique
					// then we can attach that title to actor's original name.

					const Name originalName = Naming::Default::GetDisplayFullName(actor);

					const auto npc = actor->GetActorBase();
					const Name originalShortName = npc && !npc->shortName.empty() ? npc->shortName.c_str() : originalName;

					Name formattedDisplayName{ Options::DisplayName::format };
					if (formattedDisplayName != empty) {
						clib_util::string::replace_first_instance(formattedDisplayName, "[name]", originalName);
						clib_util::string::replace_first_instance(formattedDisplayName, "[short]", originalShortName);
						clib_util::string::replace_first_instance(formattedDisplayName, "[title]", this->title);
						clib_util::string::replace_first_instance(formattedDisplayName, "[break]", "\n");
						displayName = formattedDisplayName;
					} else {
						displayName = originalName + " (" + this->title + ")";
					}
				} else {
					// If we have a custom title and actor is not unique
					// then we can use this custom title as a standalone name.
					displayName = this->title;
				}
			} else {
				displayName = empty;  // fall back to original name.
			}
		}

		NameRef NNDData::GetDisplayName(RE::Actor* actor) {
			UpdateDisplayName(actor);
			return displayName;
		}

		void NNDData::UpdateDefaultObscurityName(const RE::Actor* actor) {
			if (Name formattedObscurityName{ Options::Obscurity::defaultName }; formattedObscurityName != empty) {
				const Name race = actor->GetRace()->GetFullName();
				Name       sex;
				switch (actor->GetActorBase()->GetSex()) {
				case RE::SEX::kMale:
					sex = "Male";
					break;
				case RE::SEX::kFemale:
					sex = "Female";
					break;
				default:
					break;
				}

				const auto hadRace = clib_util::string::replace_first_instance(formattedObscurityName, "[race]", race);
				const auto hadSex = clib_util::string::replace_first_instance(formattedObscurityName, "[sex]", sex);

				if (hadRace || hadSex) {
					clib_util::string::trim(formattedObscurityName);
					defaultObscurity = formattedObscurityName;
					return;
				}
			}
			defaultObscurity = empty;
		}

		NameRef NNDData::GetTitle(RE::Actor* actor) const {
			if (title != empty)
				return title;
			if (allowDefaultTitle)
				return Naming::Default::GetDisplayFullName(actor);
			return empty;
		}

		NameRef NNDData::GetObscurity(RE::Actor* actor) const {
			if (obscurity != empty)
				return obscurity;
			if (isObscuringTitle && title != empty)
				return title;
			if (allowDefaultObscurity && Naming::Default::GetDisplayFullName(actor) != empty)
				return Naming::Default::GetDisplayFullName(actor);
			if (defaultObscurity != empty)
				return defaultObscurity;

			return Options::Obscurity::defaultName;
		}

		NameRef NNDData::GetName(NameStyle style, RE::Actor* actor) {
			if (actor->IsPlayerRef()) {
				return empty;
			}

			if (Options::Obscurity::enabled && isObscured) {
				return GetObscurity(actor);
			}

			if (!Options::General::enabled) {
				return empty;
			}

			// Check if unique actor has a custom title. In Display Name style we can combine those.
			if (isUnique) {
				return title != empty && style == kDisplayName ? GetDisplayName(actor) : empty;
			}

			switch (style) {
			case kDisplayName:
				return GetDisplayName(actor);
			default:
			case kFullName:
				return name;
			case kShortName:
				return shortDisplayName != empty ? shortDisplayName : name;
			case kTitle:
				if (const auto title = GetTitle(actor); title != empty)
					return title;
				return name;
			}
		}
	}

	// Events
	namespace Distribution
	{
		void Manager::Register() {
			if (const auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
				scripts->AddEventSink<RE::TESFormDeleteEvent>(GetSingleton());
				logger::info("Registered for {}", typeid(RE::TESFormDeleteEvent).name());
			}
		}

		RE::BSEventNotifyControl Manager::ProcessEvent(const RE::TESFormDeleteEvent* a_event,
		                                               RE::BSTEventSource<RE::TESFormDeleteEvent>*) {
			if (a_event && a_event->formID != 0) {
				DeleteName(a_event->formID);
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	}

	// Names Manager
	namespace Distribution
	{
		namespace details
		{
			/// Sorts NameDefinitions by their priorities.
			///	If priorities are the same, then alphabetical order is used.
			struct definitions_priority_greater
			{
				bool operator()(const NameDefinition& lhs, const NameDefinition& rhs) const {
					if (lhs.priority > rhs.priority)
						return true;
					if (lhs.priority == rhs.priority)
						return lhs.name < rhs.name;
					return false;
				}
			};

			/**
			 * \brief Creates a NameComponents object that contains resolved name segments from all loaded name definitions that are associated with given actor.
			 * \param scope Target scope in which the name components are being picked.
			 * \param actor An actor for whom the name components are being picked. Used to determine appropriate name variant.
			 * \param commonScopes All other scopes that used Name Definitions have in common.
			 * \return Created NameComponents containing a resolved name segments.
			 */
			std::optional<NameComponents> MakeNameComponents(Scope scope, const RE::Actor* actor, Scope& commonScopes) {
				if (!actor || !actor->GetActorBase())
					return std::nullopt;

				if (!loadedDefinitions.contains(scope))
					return std::nullopt;

				const auto& scopedLoadedDefinitions = loadedDefinitions.at(scope);
				if (scopedLoadedDefinitions.empty())
					return std::nullopt;

				std::vector<std::reference_wrapper<const NameDefinition>> definitions{};
				// Get a list of matching definitions.
				actor->GetActorBase()->ForEachKeyword([&](const RE::BGSKeyword* kwd) {
					std::string name = kwd->formEditorID.c_str();
					if (scopedLoadedDefinitions.contains(name)) {
						const auto& definition = scopedLoadedDefinitions.at(name);
						definitions.emplace_back(definition);
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});

				if (definitions.empty()) {
					return std::nullopt;
				}

				// Sort by priorities
				std::ranges::sort(definitions, definitions_priority_greater());

				std::vector<std::string> defNames;
				std::ranges::transform(definitions.begin(), definitions.end(), std::back_inserter(defNames), [](const auto& d) { return d.get().name; });
				logger::debug("\t\tFrom: [{}]", clib_util::string::join(defNames, ", "));

				// Assemble a name.
				NameComponents comps;
				const auto     sex = actor->GetActorBase()->GetSex();

				// Flags that determine whether a name segment was resolved and components contain final result.
				// These flags are used to handle name inheritance.
				auto resolvedFirstName = false;
				auto resolvedMiddleName = false;
				auto resolvedLastName = false;

				commonScopes = Scope::kAll;

				for (const auto& definitionRef : definitions) {
					const auto& definition = definitionRef.get();

					auto pickedFirstName = false;
					auto pickedMiddleName = false;
					auto pickedLastName = false;

					if (!resolvedFirstName) {
						pickedFirstName = definition.GetRandomFirstName(sex, comps);
						resolvedFirstName = pickedFirstName || !definition.firstName.shouldInherit;
					}

					if (!resolvedMiddleName) {
						pickedMiddleName = definition.GetRandomMiddleName(sex, comps);
						resolvedMiddleName = pickedMiddleName || !definition.middleName.shouldInherit;
					}

					if (!resolvedLastName) {
						pickedLastName = definition.GetRandomLastName(sex, comps);
						resolvedLastName = pickedLastName || !definition.lastName.shouldInherit;
					}

					const auto pickedAnyName = pickedFirstName || pickedMiddleName || pickedLastName;

					if (pickedAnyName) {
						commonScopes &= definition.scope;
					}

					// At the moment we use first conjunction that will be picked with at least one name segment.
					// So if Name Definition only provided conjunction, it will be skipped.
					if (pickedAnyName && comps.conjunction == empty) {
						definition.GetRandomConjunction(sex, comps);
					}

					// We use first found shortening setting in either name definition that provided at least one name.
					if (pickedAnyName && comps.shortSegments == NameSegmentType::kNone && definition.shortened != NameSegmentType::kNone) {
						comps.shortSegments = definition.shortened;
					}

					// If all segments are resolved, then we're ready :)
					if (resolvedFirstName && resolvedMiddleName && resolvedLastName)
						break;
				}
				return comps;
			}

			/**
			 * \brief Creates a name for given scope and fills provided name properties.
			 * \param scope Scope of the Name Definitions that should be used in name creation.
			 * \param name Pointer to one of the NNDData's members that will store full name picked for specified scope.
			 * \param shortened Optional pointer to one of the NNDData's members that will store a short version of the name picked for specified scope.
			 * \param actor An actor for whom a name is being created. Used to determine appropriate name variant.
			 * \return  All scopes in which created name can be used.
			 *			These scopes are picked from the Name Definition which provided the name.
			 *			If name components were picked from multiple Name Definitions then only the common scopes are used.
			 */
			Scope CreateName(Scope scope, Name* name, Name* shortened, const RE::Actor* actor) {
				Scope commonScopes = scope;

				logger::debug("\tCreating {}:", scope);

				const auto components = MakeNameComponents(scope, actor, commonScopes);
				if (components.has_value()) {
					const auto fullName = components->Assemble();
					if (fullName.has_value() && fullName != empty) {
						logger::debug("\t\tPicked: '{}'", *fullName);
						*name = *fullName;

						if (shortened) {
							const auto shortName = components->AssembleShort();
							if (shortName.has_value() && !shortName->empty() && *shortName != *fullName) {
								logger::debug("\t\tShort: '{}'", *shortName);
								*shortened = *shortName;
							}
						}
					} else {
						logger::debug("\t\tDefault will be used");
					}
				} else {
					logger::debug("\t\tDefault will be used");
				}
				return commonScopes;
			}
		}

		NameRef Manager::GetName(NameStyle style, RE::Actor* actor) {
			{  // Limit scope of the lock to cached names.
				WriteLocker lock(_lock);
				if (names.contains(actor->formID)) {
					auto& data = names.at(actor->formID);

					// For commanded actors always reveal their name, since Player... well.. commands them :)
					// These are reanimates people.
					if (data.isObscured && actor->IsCommandedActor() && actor->GetCommandingActor().get() == RE::PlayerCharacter::GetSingleton()) {
						data.isObscured = false;
						NND::UpdateCrosshairs();
						logger::debug("Revealing [0x{:X}] ('{}') who is now a minion", actor->formID, data.name != empty ? data.displayName : actor->GetActorBase()->GetName());
					}

					return data.GetName(style, actor);
				}
			}

			if (Persistency::Manager::GetSingleton()->IsLoadingGame()) {
				logger::debug("Pre-cached name for [0x{:X}] ('{}') not found. Deferred: game is loading.", actor->formID, actor->GetActorBase()->GetName());
				return empty;
			}

			return CreateData(actor).GetName(style, actor);
		}

		NNDData& Manager::SetData(const NNDData& data) {
			WriteLocker lock(_lock);
			names[data.formId] = data;
			return names.at(data.formId);
		}

		NNDData& Manager::UpdateDataFlags(NNDData& data, RE::Actor* actor) const {
			data.isUnique = actor->HasKeyword(unique);
			data.allowDefaultTitle = !actor->HasKeyword(disableDefaultTitle);
			data.allowDefaultObscurity = !actor->HasKeyword(disableDefaultObscurity);
			data.isObscured = data.isObscured && !actor->HasKeyword(known);  // we don't want to turn obscurity back on when removing known keyword.
			return data;
		}

		NNDData& Manager::CreateData(RE::Actor* actor, bool shouldOverwrite) {
			// If overwrite is not allowed, then check that data does not exist first.
			// Otherwise, proceed to generate new data.
			if (!shouldOverwrite) {
				WriteLocker lock(_lock);
				if (names.contains(actor->formID)) {
					auto& data = names.at(actor->formID);
					logger::debug("An old actor touches the NND: [0x{:X}] ('{}'):", actor->formID, actor->GetActorBase()->GetName());
					UpdateDataFlags(data, actor);
					data.UpdateDisplayName(actor);
					data.UpdateDefaultObscurityName(actor);
					logger::debug("\tIsUnique: {}", data.isUnique);
					logger::debug("\tAllowsDefaultTitle: {}", data.allowDefaultTitle);
					logger::debug("\tIsObscured: {}", data.isObscured);
					logger::debug("\tAllowsDefaultObscurity: {}", data.allowDefaultObscurity);
					logger::debug("\tCanBeObscured: {}", ActorSupportsObscurity(actor));
					logger::debug("\tName: '{}'", data.name);
					logger::debug("\tTitle: '{}'", data.title);
					logger::debug("\tObscuringName: '{}'", data.obscurity);
					logger::debug("\tShortName: '{}'", data.shortDisplayName);
					logger::debug("\tDisplayName: '{}'", data.displayName);
					return data;
				}
			}
			logger::debug("A new actor touches the NND: [0x{:X}] ('{}'):", actor->formID, actor->GetActorBase()->GetName());
			const auto startTime = std::chrono::steady_clock::now();

			NNDData data{};

			data.formId = actor->formID;

			// Enable obscurity by default if actor supports it. We do this only once during first data creation.
			data.isObscured = ActorSupportsObscurity(actor);
			UpdateDataFlags(data, actor);
			logger::debug("\tIsUnique: {}", data.isUnique);
			logger::debug("\tAllowsDefaultTitle: {}", data.allowDefaultTitle);
			logger::debug("\tIsObscured: {}", data.isObscured);
			logger::debug("\tAllowsDefaultObscurity: {}", data.allowDefaultObscurity);
			logger::debug("\tCanBeObscured: {}", ActorSupportsObscurity(actor));

			MakeName(data, actor);
			MakeTitle(data, actor);
			MakeObscureName(data, actor);

			data.UpdateDisplayName(actor);
			data.UpdateDefaultObscurityName(actor);

			const auto endTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			logger::debug("\tName: '{}'", data.name);
			logger::debug("\tTitle: '{}'", data.title);
			logger::debug("\tObscuringName: '{}'", data.obscurity);
			logger::debug("\tShortName: '{}'", data.shortDisplayName);
			logger::debug("\tDisplayName: '{}'", data.displayName);
			logger::debug("\tDuration: {} ms", duration);
			if (data.name != empty)
				logger::info("Generated name '{}, {} data' for [0x{:X}] ('{}') in {} ms", data.name, data.title, actor->formID, actor->GetActorBase()->GetName(), duration);
			return SetData(data);
		}

		void Manager::DeleteData(const RE::Actor* actor) {
			WriteLocker lock(_lock);
			if (names.contains(actor->formID)) {
				const NNDData data = names.at(actor->formID);
				if (names.erase(actor->formID))
					logger::debug("Deleted cache for [0x{:X}] ('{}')", actor->formID, data.name != empty ? data.displayName : actor->GetActorBase()->GetFullName());
			}
		}

		bool Manager::RevealName(const RE::Actor* actor) {
			WriteLocker lock(_lock);
			if (const auto& it = names.find(actor->formID); it != names.end() && it->second.isObscured) {
				it->second.isObscured = false;
				logger::debug("Revealing [0x{:X}] ('{}')", actor->formID, it->second.name != empty ? it->second.name : actor->GetActorBase()->GetName());
				return true;
			}

			return false;
		}

		void Manager::MakeName(NNDData& data, const RE::Actor* actor) const {
			if (!data.isUnique && data.name == empty) {
				details::CreateName(Scope::kName, &data.name, &data.shortDisplayName, actor);
			}
		}

		void Manager::MakeTitle(NNDData& data, const RE::Actor* actor) const {
			if (data.title == empty) {
				const Scope titleScopes = details::CreateName(Scope::kTitle, &data.title, nullptr, actor);
				data.isObscuringTitle = data.title != empty && has(titleScopes, Scope::kObscurity);
				if (data.title != empty) {
					logger::debug("\t\tTitle scopes: [{}]", titleScopes);
					if (has(titleScopes, Scope::kTitle))
						logger::debug("\t\tCan be used in obscurity");
				}
			}
		}

		void Manager::MakeObscureName(NNDData& data, const RE::Actor* actor) const {
			if (data.isObscured && !data.isObscuringTitle && data.obscurity == empty) {
				details::CreateName(Scope::kObscurity, &data.obscurity, nullptr, actor);
			}
		}

		void Manager::DeleteName(RE::FormID formId) {
			WriteLocker lock(_lock);
			if (names.contains(formId)) {
				const NNDData data = names.at(formId);
				if (names.erase(formId))
					logger::debug("Deleted name for [0x{:X}] ('')", formId, data.displayName);
			}
		}

		NNDData& Manager::UpdateData(NNDData& data, RE::Actor* actor, bool definitionsChanged, bool silenceLog) const {
			UpdateDataFlags(data, actor);

			if (definitionsChanged) {
				logger::debug("\t\tUpdating name..");
				MakeName(data, actor);
				MakeTitle(data, actor);
				MakeObscureName(data, actor);
			}

			data.UpdateDisplayName(actor);
			data.UpdateDefaultObscurityName(actor);
			if (!silenceLog) {
				logger::debug("\t\tIsUnique: {}", data.isUnique);
				logger::debug("\t\tAllowsDefaultTitle: {}", data.allowDefaultTitle);
				logger::debug("\t\tIsObscured: {}", data.isObscured);
				logger::debug("\t\tAllowsDefaultObscurity: {}", data.allowDefaultObscurity);
				logger::debug("\t\tName: '{}'", data.name);
				logger::debug("\t\tTitle: '{}'", data.title);
				logger::debug("\t\tObscuringName: '{}'", data.obscurity);
				logger::debug("\t\tShortName: '{}'", data.shortDisplayName);
				logger::debug("\t\tDisplayName: '{}'", data.displayName);
			}
			return data;
		}

		bool Manager::ActorSupportsObscurity(RE::Actor* actor) const {
			// For commanded actors always reveal their name, since Player... well.. commands them :)
			// These are reanimates people.
			if (actor->IsCommandedActor() && actor->GetCommandingActor().get() == RE::PlayerCharacter::GetSingleton()) {
				return false;
			}

			/// A flag indicating whether given actor can be introduced to the player.
			///	For that actor:
			/// - must be able to talk with the player in any capacity
			///	- had zero conversations with the player prior to that. (mid-game installation support)
			const auto canBeIntroduced = actor->CanTalkToPlayer() && !talkedToPC->IsTrue(actor, nullptr);

			return canBeIntroduced;
		}

		Manager::Manager() :
			talkedToPC(std::make_unique<RE::TESCondition>()) {
			RE::CONDITION_ITEM_DATA condData{};
			condData.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetTalkedToPC;
			condData.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
			condData.comparisonValue.f = 1;

			const auto newNode = new RE::TESConditionItem;
			newNode->data = condData;
			newNode->next = nullptr;

			talkedToPC->head = newNode;
		}

		void Manager::UpdateNames(const std::function<void(NamesMap&)> update) {
			WriteLocker lock(_lock);
			update(names);
		}

		const Manager::NamesMap& Manager::GetAllNames() const {
			ReadLocker lock(_lock);
			return names;
		}
	}
}
