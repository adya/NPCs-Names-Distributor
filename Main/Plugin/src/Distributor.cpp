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

		NameRef NNDData::GetName(NameFormat format) const {
			if (format == kShortName) {
				return shortDisplayName != empty ? shortDisplayName : name;
			}
			return format == kName ? name : displayName;
		}
	}

	// FormDelete
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
				break;
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

			std::optional<NameComponents> MakeNameComponents(Scope scope, const RE::TESNPC* npc) {
				if (!loadedDefinitions.contains(scope))
					return std::nullopt;

				const auto& scopedLoadedDefinitions = loadedDefinitions.at(scope);
				if (scopedLoadedDefinitions.empty())
					return std::nullopt;

				std::vector<std::reference_wrapper<const NameDefinition>> definitions{};
				// Get a list of matching definitions.
				npc->ForEachKeyword([&](const RE::BGSKeyword& kwd) {
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
				logger::info("Generating {} for {} from: [{}]", nameType, npc->GetName(), clib_util::string::join(defNames, ", "));
#endif
				// Assemble a name.
				NameComponents comps;
				const auto     sex = npc->GetSex();

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

			// TODO: Get a reference to this keyword in the plugin and use it to compare directly.
			// It could be slightly more efficient.
			bool IsObscure(const RE::TESNPC* npc) {
				return npc->HasKeywordString("NNDObscure") || !npc->HasKeywordString("NNDKnown");
			}

			void CreateName(Scope scope, Name* name, Name* shortened, const RE::TESNPC* npc) {
				const auto components = MakeNameComponents(scope, npc);
				if (components.has_value()) {
					const auto fullName = components->Assemble();
					if (fullName.has_value() && !fullName->empty()) {
#ifndef NDEBUG
						std::string nameType = rawScopeName(scope);
						logger::info("Caching new {} '{}' for '{}' [0x{:X}]", nameType, *fullName, std::string(npc->GetName()), npc->formID);
#endif
						*name = *fullName;

						if (shortened) {
							const auto shortName = components->AssembleShort();
							if (shortName.has_value() && !shortName->empty() && *shortName != *fullName) {
#ifndef NDEBUG
								logger::info("Caching new short {} '{}' for '{}' [0x{:X}]", nameType, *shortName, std::string(npc->GetName()), npc->formID);
#endif
								*shortened = *shortName;
							}
						}
					}
				}
			}
		}

		NameRef Manager::GetName(NameFormat format, const RE::TESNPC* npc, const char* originalName) {
			{  // Lock for reading access to cached names.
				ReadLocker lock(_lock);
				if (names.contains(npc->formID)) {
					const auto& data = names.at(npc->formID);
					return !data.isUnique ? data.GetName(format) : empty;
				}
			}

			const auto startTime = std::chrono::steady_clock::now();

			NNDData data{};

			data.formId = npc->formID;

			data.isUnique = npc->HasKeyword(unique);
			data.isTitleless = npc->HasKeyword(titleless);
			data.isKnown = npc->HasKeyword(known);

			// Ignore marked as unique NPCs.
			if (data.isUnique) {
				SetName(data);
				return originalName;
			}

			details::CreateName(Scope::kName, &data.name, &data.shortDisplayName, npc);
			details::CreateName(Scope::kTitle, &data.title, nullptr, npc);

			// Use original name as title if no custom one provided.
			if (data.title == empty && !data.isTitleless) {
				data.title = originalName;
			}

			if (details::IsObscure(npc)) {
				details::CreateName(Scope::kObscurity, &data.obscurity, nullptr, npc);
			}

			data.UpdateDisplayName();
			const auto endTime = std::chrono::steady_clock::now();
			const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			logger::info("Generated name in {}μs / {}ms", duration, duration / 1000.0f);

			return SetName(data).GetName(format);
		}

		NNDData& Manager::SetName(const NNDData& data) {
			WriteLocker lock(_lock);
			names[data.formId] = data;
			return names.at(data.formId);
		}

		void Manager::DeleteName(RE::FormID formId) {
			WriteLocker lock(_lock);
			names.erase(formId);
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
