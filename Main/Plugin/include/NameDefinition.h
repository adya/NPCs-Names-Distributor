#pragma once
namespace NND
{
    struct NameDefinition
    {
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

		struct Behavior
		{
			bool useForNames = true;
		    bool useForTitles = false;
			bool useForObscuring = false;
			std::uint8_t chance = 100;
		};

		struct BaseNamesContainer
		{
			std::vector<std::string> names{};
			std::uint8_t             chance = 100;
		};

		struct Affix: BaseNamesContainer {};
		
		struct GenderNames: BaseNamesContainer
		{
			Affix prefix;
			Affix suffix;
			/// Combination of prefix and suffix.
			///	Written in form: "{prefix}>{name}<{suffix}"
			Affix circumfix;
		};

		struct NamePart
		{
			struct Behavior
			{
				// TODO: Allow combining prefixes
				// Also, define which conjunction is used when combining two definitions
				// I think the original conjunction should be used.
				bool shouldCombine = false;
			};

			GenderNames male;
			GenderNames female;
			GenderNames any;

			Behavior behavior;
		};

		NamePart firstName;
		NamePart lastName;
		NamePart conjunction;


		Behavior behavior;
    };
}
