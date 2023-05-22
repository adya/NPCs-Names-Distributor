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
		void NNDData::UpdateDisplayName() {
			if (name != empty && title != empty) {
				Name formattedDisplayName = Options::DisplayName::format;
				if (formattedDisplayName != empty) {
					clib_util::string::replace_first_instance(formattedDisplayName, "[name]", name);
					clib_util::string::replace_first_instance(formattedDisplayName, "[title]", title);
					clib_util::string::replace_first_instance(formattedDisplayName, "[break]", "\n");
					displayName = formattedDisplayName;
				} else {
					displayName = name + " (" + title + ")";
				}
			} else if (name != empty) {
				displayName = name;
			}
			else if (title != empty) {
				displayName = title;
			} else {
				displayName = empty;  // fall back to original name.
			}
		}

		NameRef NNDData::GetName(NameStyle style) const {
			if (Options::Obscurity::enabled && isObscured && obscurity != empty) {
				return obscurity;
			}

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
				return title != empty ? title : name;
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
				std::string              nameType = rawScopeName(scope);

				std::ranges::transform(definitions.begin(), definitions.end(), std::back_inserter(defNames), [](const auto& d) { return d.get().name; });
				logger::info("Generating {} for {} from: [{}]", nameType, actor->GetName(), clib_util::string::join(defNames, ", "));
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
				Scope      commonScopes = scope;
				const auto components = MakeNameComponents(scope, actor, commonScopes);
				if (components.has_value()) {
					const auto fullName = components->Assemble();
					if (fullName.has_value() && !fullName->empty()) {
#ifndef NDEBUG
						std::string nameType = rawScopeName(scope);
						logger::info("Caching new {} '{}' for '{}' [0x{:X}]", nameType, *fullName, std::string(actor->GetName()), actor->formID);
#endif
						*name = *fullName;

						if (shortened) {
							const auto shortName = components->AssembleShort();
							if (shortName.has_value() && !shortName->empty() && *shortName != *fullName) {
#ifndef NDEBUG
								logger::info("Caching new short {} '{}' for '{}' [0x{:X}]", nameType, *shortName, std::string(actor->GetName()), actor->formID);
#endif
								*shortened = *shortName;
							}
						}
					}
				}
				return commonScopes;
			}
		}

		NameRef Manager::GetName(NameStyle style, RE::Actor* actor) {
			{  // Limit scope of the lock to cached names.
				WriteLocker lock(_lock);
				if (names.contains(actor->formID)) {
					auto& data = names.at(actor->formID);

					if (data.updateMask != NNDData::UpdateMask::kNone) {
#ifndef NDEBUG
						logger::info("Refreshing [0x{:X}] {} ({})...", data.formId, data.name, data.title);
#endif
						Refresh(data, actor);
					}
					// For commanded actors always reveal their name, since Player... well.. commands them :)
					// These are reanimates people.
					if (actor->IsCommandedActor() && actor->GetCommandingActor().get() == RE::PlayerCharacter::GetSingleton()) {
						data.isObscured = false;
					}
					return data.GetName(style);
				}
			}

			const auto startTime = std::chrono::steady_clock::now();

			NNDData data{};

			data.formId = actor->formID;

			/// A flag indicating whether given actor can be introduced to the player.
			///	For that actor:
			/// - must be able to talk with the player in any capacity
			///	- had zero conversations with the player prior to that. (mid-game installation support)
			const auto canBeIntroduced = actor->CanTalkToPlayer() && !talkedToPC->IsTrue(actor, nullptr);

			// This is a little bit weird check, I'm 98% sure that I could just use actor, but... there is this 2% chance.. :)
			if (const auto npc = actor->GetActorBase()) {
				data.isUnique = npc->HasKeyword(unique);
				data.isTitleless = npc->HasKeyword(titleless);
				data.isObscured = canBeIntroduced && !actor->IsCommandedActor() && !npc->HasKeyword(known);
			} else {
				data.isUnique = actor->HasKeyword(unique);
				data.isTitleless = actor->HasKeyword(titleless);
				data.isObscured = canBeIntroduced && !actor->IsCommandedActor() && !actor->HasKeyword(known);
			}

			// Ignore marked as unique NPCs that are not obscured.
			if (data.isUnique && !data.isObscured) {
				SetName(data);
				return empty;
			}

			if (!data.isUnique) {
				MakeName(data, actor);
			}

			MakeTitle(data, actor);
			MakeObscureName(data, actor);

			data.UpdateDisplayName();

			// TODO: Comment this for releases.
			// (not NDEBUG, since I'd want to measure performance on Release from time to time)
			const auto endTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			logger::info("Generated name {} in {} ms", data.displayName, duration);

			return SetName(data).GetName(style);
		}

		NNDData& Manager::SetName(const NNDData& data) {
			WriteLocker lock(_lock);
			names[data.formId] = data;
			return names.at(data.formId);
		}

		void Manager::RevealName(RE::FormID formId) {
			WriteLocker lock(_lock);
			if (const auto& it = names.find(formId); it != names.end()) {
				it->second.isObscured = false;
			}
		}

		void Manager::MakeName(NNDData& data, const RE::Actor* actor) const {
			if (!data.isUnique && data.name == empty) {
				details::CreateName(Scope::kName, &data.name, &data.shortDisplayName, actor);
			}
		}

		void Manager::MakeTitle(NNDData& data, const RE::Actor* actor) const {
			if (data.hasDefaultTitle || data.title == empty) {
				const Scope titleScopes = details::CreateName(Scope::kTitle, &data.title, nullptr, actor);
				data.hasDefaultTitle = false;
				// Use original full name as title if no custom one provided.
				if (data.title == empty && !data.isTitleless) {
					if (const auto npc = actor->GetActorBase()) {
						data.title = npc->GetFullName();
						data.hasDefaultTitle = data.title != empty;
					}
				}
				data.isObscuringTitle = !data.hasDefaultTitle && data.title != empty && has(titleScopes, Scope::kObscurity);
#ifndef NDEBUG
				std::string nameType = allScopeNames(titleScopes);
				logger::info("Title {} for [0x{:X}] can be used in {} scopes", data.title, actor->formID, nameType);
#endif
			}
		}

		void Manager::MakeObscureName(NNDData& data, const RE::Actor* actor) const {
			if (data.isObscured && (data.hasDefaultObscurity || data.obscurity == empty)) {
				// If custom title is provided and can be used in Obscurity scope - use it
				if (data.isObscuringTitle) {
					data.obscurity = data.title;
				} else {
					// If no custom title usable in obscurity, then try to pick obscuring name in
					details::CreateName(Scope::kObscurity, &data.obscurity, nullptr, actor);
					// If name wasn't picked check whether original name can be used as title. In all other cases fallback to default obscuring name.
					if (data.obscurity == empty) {
						auto originalName = empty;
						if (const auto npc = actor->GetActorBase()) {
							originalName = npc->GetFullName();
						}
						data.hasDefaultObscurity = data.isTitleless;
						data.obscurity = data.hasDefaultObscurity || originalName == empty ? Options::Obscurity::defaultName : originalName;
					}
				}
			}
		}

		void Manager::DeleteName(RE::FormID formId) {
			WriteLocker lock(_lock);
			names.erase(formId);
		}
		
		void Manager::Refresh(NNDData& data, const RE::Actor* actor) {
			if (has(data.updateMask, NNDData::UpdateMask::kDefinitions)) {
				MakeName(data, actor);
				MakeTitle(data, actor);
				MakeObscureName(data, actor);
				data.UpdateDisplayName();
			} else {
				if (has(data.updateMask, NNDData::UpdateMask::kObscureName)) {
					// Only check obscurity if definitions weren't changed. Otherwise it's already handled.
					if (data.isObscured && data.hasDefaultObscurity) {
						data.obscurity = Options::Obscurity::defaultName;
					}
				}

				if (has(data.updateMask, NNDData::UpdateMask::kDisplayName)) {
					data.UpdateDisplayName();
				}
			}
			
			data.updateMask = NNDData::UpdateMask::kNone;
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
