#pragma once
#include "LookupNameDefinitions.h"
#include "NameDefinition.h"

namespace NND {
	namespace Distribution
	{

		enum NameScope
		{

			kName,
			kTitle,
			kObscurity
		};

		/// Priority
		enum Priority
		{

			/// Definition with `kRace` priority contains base names specific for races or otherwise a large group that shares an innate trait that cannot be changed.
			/// This is the default one and may be omitted.
			kRace = 0,

			/// Definition with `kClass` priority contains names for a more narrow group that shares an innate trait, which also cannot be changed.
			kClass,

			/// Definition with `kFaction` priority contains names for a medium sized group that are united by some common idea or belief, that are scattered across the world.
			kFaction,

			/// Definition with `kClan` priority contains names for a small group that are united are united by some common idea or belief, but typically located in a single area.
			kClan,

			/// Definition with `kIndividual` priority contains names for distinct individuals that are usually hand-picked.
			kIndividual,

			/// Default priority is `kRace`.
			kDefault = kRace,

			/// The highest possible priority.
			kForced = kIndividual
		};

		// TODO: Don't save the actual names, instead save indices of picked name parts.
		// Pros:
		//		- Much less memory footprint, all names could be stored in NameDefinitions, other places would reference them with string_view
		//		- Much simpler SKSE cosave structure: just a bunch of numbers
		//
		// Cons:
		//		- The name would reset if Name Definition would get deleted, or the names would shuffle.
		//		- Need to handle proper loading with index validation.
		struct DistributedName
		{
			Name name{};
			Name title{};
			Name obscurity{};

			std::string sourceNameDefinition{};

			NameRef GetNameInScope(NameScope scope);
		};

		inline std::unordered_map<RE::FormID, DistributedName> names;

		inline NameRef GetName(const RE::TESNPC* npc) {
			if (names.contains(npc->formID)) {
				return names.at(npc->formID).GetNameInScope(kName);
			}
			logger::info("Generating name for {}", npc->GetName());
			const auto& definition = definitions.at("NNDNord");

			if (const auto name = definition.GetRandomName(npc->GetSex()).Assemble(); !name->empty()) {
				logger::info("Caching name for {}. New name: {}", std::string(npc->GetName()), *name);
				names[npc->formID].name = *name;
				return names.at(npc->formID).name;
			}

			return empty;
		}

		inline NameRef GetName(const RE::Actor* actor) {
			return GetName(actor->GetActorBase());
		}
	}
}
