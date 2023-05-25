#include "Distributor.h"
#include "LookupNameDefinitions.h"
#include "NNDKeywords.h"

namespace NND
{
	using Scope = NameDefinition::Scope;

	// NNDData
	namespace Distribution
	{
		// Here we'll handle all styles and whatnot.
		void NNDData::UpdateDisplayName(const RE::Actor* actor) {
			const Name effectiveTitle(GetTitle(actor));
			if (name != empty && effectiveTitle != empty) {
				Name formattedDisplayName{ Options::DisplayName::format };
				if (formattedDisplayName != empty) {
					clib_util::string::replace_first_instance(formattedDisplayName, "[name]", name);
					clib_util::string::replace_first_instance(formattedDisplayName, "[title]", effectiveTitle);
					clib_util::string::replace_first_instance(formattedDisplayName, "[break]", "\n");
					displayName = formattedDisplayName;
				} else {
					displayName = name + " (" + effectiveTitle + ")";
				}
			} else if (name != empty) {
				displayName = name;
			} else if (this->title != empty && !isUnique) {
				// If we have a custom title and actor is not unique
				// then we can use this custom title as a standalone name.
				displayName = this->title;
			} else {
				displayName = empty;  // fall back to original name.
			}
		}

		NameRef NNDData::GetTitle(const RE::Actor* actor) const {
			if (title != empty)
				return title;
			if (allowDefaultTitle)
				return actor->GetActorBase()->GetFullName();
			return empty;
		}

		NameRef NNDData::GetObscurity(const RE::Actor* actor) const {
			if (obscurity != empty)
				return obscurity;
			if (isObscuringTitle && title != empty)
				return title;
			if (allowDefaultObscurity)
				return actor->GetActorBase()->GetFullName();
			
			return Options::Obscurity::defaultName;
		}

		NameRef NNDData::GetName(NameStyle style, const RE::Actor* actor) const {
			if (Options::Obscurity::enabled && isObscured) {
				if (const auto obscurity = GetObscurity(actor); obscurity != empty)
					return obscurity;
				logger::warn("WARN: Obscuring name is empty. Make sure you don't have Obscurity:sDefaultName set to empty value in INI.");
				return empty;
			}

			if (!Options::General::enabled)
				return empty;

			if (isUnique) {
				return empty;
			}
			switch (style) {
			case kDisplayName:
				return displayName;
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

#ifndef NDEBUG
		std::string rawScopeName(const Scope scope) {
			switch (scope) {
			default:
			case Scope::kName:
				return "name";
			case Scope::kTitle:
				return "title";
			case Scope::kObscurity:
				return "obscure name";
			}
		}

		std::string allScopeNames(const Scope scope) {
			std::vector<std::string> names{};
			if (has(scope, Scope::kName)) {
				names.push_back("name");
			}
			if (has(scope, Scope::kTitle)) {
				names.push_back("title");
			}
			if (has(scope, Scope::kObscurity)) {
				names.push_back("obscuring");
			}
			return clib_util::string::join(names, ", ");
		}
#endif

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
				actor->GetActorBase()->ForEachKeyword([&](const RE::BGSKeyword& kwd) {
					std::string name = kwd.formEditorID.c_str();
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
#ifndef NDEBUG
				std::vector<std::string> defNames;

				std::ranges::transform(definitions.begin(), definitions.end(), std::back_inserter(defNames), [](const auto& d) { return d.get().name; });
				logger::info("\t\tFrom: [{}]", clib_util::string::join(defNames, ", "));
#endif
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

#ifndef NDEBUG
				std::string nameType = rawScopeName(scope);
				logger::info("\tCreating {}:", nameType);
#endif
				const auto components = MakeNameComponents(scope, actor, commonScopes);
				if (components.has_value()) {
					const auto fullName = components->Assemble();
					if (fullName.has_value() && fullName != empty) {
#ifndef NDEBUG
						logger::info("\t\tPicked: '{}'", *fullName);
#endif
						*name = *fullName;

						if (shortened) {
							const auto shortName = components->AssembleShort();
							if (shortName.has_value() && !shortName->empty() && *shortName != *fullName) {
#ifndef NDEBUG
								logger::info("\t\tShort: '{}'", *shortName);
#endif
								*shortened = *shortName;
							}
						}
#ifndef NDEBUG
					} else {
						logger::info("\t\tDefault will be used");
#endif
					}
#ifndef NDEBUG
				} else {
					logger::info("\t\tDefault will be used");
#endif
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
						RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
#ifndef NDEBUG
						logger::info("Revealing [0x{:X}] ('{}') who is now a minion", actor->formID, data.name != empty ? data.displayName : actor->GetActorBase()->GetFullName());
#endif
					}
						
					return data.GetName(style, actor);
				}
			}
			// This is the case when user hit "Regenerate" name.
			// Also this is called at the very beginning of the loading for some weird npcs.
#ifndef NDEBUG
			logger::info("Pre-cached name for [0x{:X}] ('{}') not found. Name will be created in-place.", actor->formID, actor->GetActorBase()->GetName());
#endif
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
			data.isObscured = data.isObscured && !actor->HasKeyword(known); // we don't want to turn obscurity back on when removing known keyword.
			return data;
		}

		NNDData& Manager::CreateData(RE::Actor* actor) {
			{
				WriteLocker lock(_lock);
				if (names.contains(actor->formID)) {
					auto& data = names.at(actor->formID);
#ifndef NDEBUG
					logger::info("An old actor touches the NND: [0x{:X}] ('{}'):", actor->formID, actor->GetActorBase()->GetName());
#endif
					UpdateDataFlags(data, actor);
					return data;
				}
			}
#ifndef NDEBUG
			logger::info("A new actor touches the NND: [0x{:X}] ('{}'):", actor->formID, actor->GetActorBase()->GetName());
#endif
			const auto startTime = std::chrono::steady_clock::now();

			NNDData data{};

			data.formId = actor->formID;

			// Enable obscurity by default if actor supports it. We do this only once during first data creation.
			data.isObscured = ActorSupportsObscurity(actor);
			UpdateDataFlags(data, actor);
#ifndef NDEBUG
			logger::info("\tIsUnique: {}", data.isUnique);
			logger::info("\tAllowsDefaultTitle: {}", data.allowDefaultTitle);
			logger::info("\tIsObscured: {}", data.isObscured);
			logger::info("\tAllowsDefaultObscurity: {}", data.allowDefaultObscurity);
			logger::info("\tCanBeObscured: {}", ActorSupportsObscurity(actor));
#endif
			MakeName(data, actor);
			MakeTitle(data, actor);
			MakeObscureName(data, actor);

			data.UpdateDisplayName(actor);

			// TODO: Comment this for releases.
			// (not NDEBUG, since I'd want to measure performance on Release from time to time)
			const auto endTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
#ifndef NDEBUG
			logger::info("\tDisplayName: '{}'", data.name != empty ? data.displayName : actor->GetActorBase()->GetName());
			logger::info("\tDuration: {} ms", duration);
#else
			if (data.name != empty)
				logger::info("Generated name '{}' for [0x{:X}] ('{}') in {} ms", data.displayName, actor->formID, actor->GetActorBase()->GetName(), duration);
#endif
			return SetData(data);
		}

		void Manager::DeleteData(RE::Actor* actor) {
			WriteLocker lock(_lock);
#ifndef NDEBUG
			if (names.contains(actor->formID)) {
				const NNDData data = names.at(actor->formID);
				if (names.erase(actor->formID))
					logger::info("Deleted cache for [0x{:X}] ('{}')", actor->formID, data.name != empty ? data.displayName : actor->GetActorBase()->GetFullName());
			}
#else
			names.erase(actor->formID);
#endif
		}

		bool Manager::RevealName(const RE::Actor* actor, bool forceGreet) {
			if (forceGreet && !Options::Obscurity::greetings)
				return false;
			WriteLocker lock(_lock);
			if (const auto& it = names.find(actor->formID); it != names.end() && it->second.isObscured) {
				it->second.isObscured = false;
#ifndef NDEBUG
				logger::info("Revealing [0x{:X}] ('{}')", actor->formID, it->second.name != empty ? it->second.name : actor->GetActorBase()->GetName());
#endif
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
#ifndef NDEBUG
				if (data.title != empty) {
					std::string nameType = allScopeNames(titleScopes);
					if (has(titleScopes, Scope::kTitle))
						logger::info("\t\tCan be used in obscurity");
				}
#endif
			}
		}

		void Manager::MakeObscureName(NNDData& data, const RE::Actor* actor) const {
			if (data.isObscured && !data.isObscuringTitle && data.obscurity == empty) {
				details::CreateName(Scope::kObscurity, &data.obscurity, nullptr, actor);
			}
		}

		void Manager::DeleteName(RE::FormID formId) {
			WriteLocker lock(_lock);
#ifndef NDEBUG
			if (names.contains(formId)) {
				const NNDData data = names.at(formId);
				if (names.erase(formId))
					logger::info("Deleted name for [0x{:X}] ('')", formId, data.displayName);				
			}
#else
			names.erase(formId);
#endif
		}

		NNDData& Manager::UpdateData(NNDData& data, RE::Actor* actor, bool definitionsChanged) const {
			UpdateDataFlags(data, actor);
#ifndef NDEBUG
			logger::info("\t\tIsUnique: {}", data.isUnique);
			logger::info("\t\tAllowsDefaultTitle: {}", data.allowDefaultTitle);
			logger::info("\t\tIsObscured: {}", data.isObscured);
			logger::info("\t\tAllowsDefaultObscurity: {}", data.allowDefaultObscurity);
#endif
			if (definitionsChanged) {
#ifndef NDEBUG
				logger::info("\t\tUpdating name..");
#endif
				MakeName(data, actor);
				MakeTitle(data, actor);
				MakeObscureName(data, actor);
			}

			data.UpdateDisplayName(actor);

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

		const Manager::NamesMap& Manager::GetAllNames() {
			ReadLocker lock(_lock);
			return names;
		}
	}
}
