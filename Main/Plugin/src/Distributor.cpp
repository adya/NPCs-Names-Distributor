#include "Distributor.h"
#include "LookupNameDefinitions.h"

namespace NND
{
	namespace Distribution
	{
		NameRef NNDData::GetNameInScope(NameDefinition::Scope scope) {
			switch (scope) {
			default:
			case NameDefinition::Scope::kName:
				return name;
			case NameDefinition::Scope::kTitle:
				return title;
			case NameDefinition::Scope::kObscurity:
				return obscurity;
			}
		}

		// Here we'll handle all styles and whatnot.
		void NNDData::UpdateDisplayName() {
			displayName = name;
			if (title != empty)
				displayName += " (" + title + ")";
		}

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

		std::optional<Name> CreateName(NameDefinition::Scope scope, const RE::TESNPC* npc) {

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
			std::ranges::transform(definitions.begin(), definitions.end(), std::back_inserter(defNames), [](const auto& d) { return d.get().name; });
			logger::info("Generating name for {} from: [{}]", npc->GetName(), clib_util::string::join(defNames, ", "));
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
				auto& definition = definitionRef.get();
				const auto emptyFirst = comps.firstName == empty;
				const auto emptyMiddle = comps.middleName == empty;
				const auto emptyLast = comps.lastName == empty;
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
				if (comps.conjunction == empty || (emptyFirst && emptyMiddle && emptyLast)) {
					definition.GetRandomConjunction(sex, comps);
				}

				resolvedFirstName = resolvedFirstName || !definition.firstName.behavior.shouldInherit;
				resolvedMiddleName = resolvedMiddleName || !definition.middleName.behavior.shouldInherit;
				resolvedLastName = resolvedLastName || !definition.lastName.behavior.shouldInherit;

				// If all segments are resolved, then we're ready :)
				if (resolvedFirstName && resolvedMiddleName && resolvedLastName)
					break;
			}
			return comps.Assemble();
		}

		// TODO: Get a reference to this keyword in the plugin and use it to compare directly.
		// It could be slightly more efficient.
		bool IsObscure(const RE::TESNPC* npc) {
			return npc->HasKeywordString("NNDObscure") || !npc->HasKeywordString("NNDKnown");
		}

		void SetName(Name* name, NameDefinition::Scope scope, const RE::TESNPC* npc) {
			if (const auto createdName = CreateName(scope, npc)) {
				// Apparently I can't have this condition in the same if, because it will always be executed regardless of whether name had value or not..
				if (!createdName->empty()) {
#ifndef NDEBUG
					logger::info("Caching new name '{}' for '{}'", *createdName, std::string(npc->GetName()));
#endif
					*name = *createdName;
				}
			}
		}

		NameRef GetName(const RE::TESNPC* npc) {
			if (names.contains(npc->formID)) {
				auto& data = names.at(npc->formID);
				return data.displayName;
			}
			NNDData data{};

			SetName(&data.name, NameDefinition::Scope::kName, npc);
			SetName(&data.title, NameDefinition::Scope::kTitle, npc);

			if (IsObscure(npc)) {
				SetName(&data.obscurity, NameDefinition::Scope::kObscurity, npc);
			}

			data.UpdateDisplayName();
			names[npc->formID] = data;
			
			return data.displayName;
		}
	}
}
