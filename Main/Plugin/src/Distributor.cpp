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
			if (name != empty) {
				displayName = name;
				if (title != empty)
					displayName += " (" + title + ")";
			} else if (title != empty) {
				displayName = title;
			} else {
				displayName = empty;  // fall back to original name.
			}
		}

		NameRef NNDData::GetName(NameFormat format, const RE::Actor* actor) const {
			if (isObscured && obscurity != empty) {
				return obscurity;
			}

			if (isUnique) {
				return empty;
			}

			if (format == kShortName) {
				return shortDisplayName != empty ? shortDisplayName : name;
			}
			return format == kFullName ? name : displayName;
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
		std::string rawScopeName(Scope scope) {
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

			std::optional<NameComponents> MakeNameComponents(Scope scope, const RE::Actor* actor) {
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

				for (const auto& definitionRef : definitions) {
					auto&      definition = definitionRef.get();
					const auto emptyFirst = comps.firstName == empty;
					const auto emptyMiddle = comps.middleName == empty;
					const auto emptyLast = comps.lastName == empty;
					const auto allNamesEmpty = emptyFirst && emptyMiddle && emptyLast;
					if (emptyFirst && !resolvedFirstName) {
						definition.GetRandomFirstName(sex, comps);
					}

					if (emptyMiddle && !resolvedMiddleName) {
						definition.GetRandomMiddleName(sex, comps);
					}

					if (emptyLast && !resolvedLastName) {
						definition.GetRandomLastName(sex, comps);
					}

					// At the moment we use first conjunction that will be picked with at least one name segment.
					// So if Name Definition only provided conjunction, it will be skipped.
					if (comps.conjunction == empty || allNamesEmpty) {
						definition.GetRandomConjunction(sex, comps);
					}

					// We use first found shortening setting in either name definition.
					if (comps.shortSegments == NameSegmentType::kNone && definition.shortened != NameSegmentType::kNone) {
						comps.shortSegments = definition.shortened;
					}

					resolvedFirstName = resolvedFirstName || !definition.firstName.shouldInherit;
					resolvedMiddleName = resolvedMiddleName || !definition.middleName.shouldInherit;
					resolvedLastName = resolvedLastName || !definition.lastName.shouldInherit;

					// If all segments are resolved, then we're ready :)
					if (resolvedFirstName && resolvedMiddleName && resolvedLastName)
						break;
				}
				return comps;
			}

			void CreateName(Scope scope, Name* name, Name* shortened, const RE::Actor* actor) {
				const auto components = MakeNameComponents(scope, actor);
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
			}
		}

		NameRef Manager::GetName(NameFormat format, RE::Actor* actor, const char* originalName) {
			{  // Limit scope of lock for reading access to cached names.
				ReadLocker lock(_lock);
				if (names.contains(actor->formID)) {
					auto& data = names.at(actor->formID);
					// For commanded actors always reveal their name, since Player... well.. commands them :)
					// These are reanimates people.
					if (actor->IsCommandedActor()) {
						data.isObscured = false;
					}
					return data.GetName(format, actor);
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
				return originalName;
			}

			if (!data.isUnique) {
				details::CreateName(Scope::kName, &data.name, &data.shortDisplayName, actor);
			}

			details::CreateName(Scope::kTitle, &data.title, nullptr, actor);

			/* Algorithm for obscurity:
			 * 1. If definition for obscuring names present - use it
			 * 2. If not present (or name wasn't picked) check title
			 * 3. If custom title is provided - use it
			 * 4. If no custom title and isTitleless = false - use preferred obscuring name (originalName or ???. Original name is default option when not titleless)
			*/
			if (data.isObscured) {
				details::CreateName(Scope::kObscurity, &data.obscurity, nullptr, actor);
				if (data.obscurity == empty) {
					data.obscurity = data.title != empty ? data.title : defaultObscure;
				}
			}

			// Set default title after obscuring, so that obscurity never uses originalName.
			// Use original name as title if no custom one provided.
			if (data.title == empty && !data.isTitleless) {
				data.title = originalName;
			}

			data.UpdateDisplayName();
			const auto endTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			logger::info("Generated name in {}μs / {}ms", duration, duration / 1000.0f);

			return SetName(data).GetName(format, actor);
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

		void Manager::DeleteName(RE::FormID formId) {
			WriteLocker lock(_lock);
			names.erase(formId);
		}

		Manager::Manager(): talkedToPC(std::make_unique<RE::TESCondition>()) {
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
