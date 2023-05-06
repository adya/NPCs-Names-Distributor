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

		using NamesList = std::vector<std::string>;

		struct Behavior
		{
			bool useForNames = true;
		    bool useForTitles = false;
			bool useForObscuring = false;
			std::uint8_t chance = 100;

			[[nodiscard]] bool HasDefaultScopes() const {
				return useForNames && !useForTitles && !useForObscuring;
			}
		};

		struct BaseNamesContainer
		{
			NamesList    names{};
			std::uint8_t chance = 100;

			/// Checks whether given names container will always produce a name.
			///	Container is considered static when it has at least one name and it's chance is 100%.
            [[nodiscard]] bool IsStatic() const {
				return chance >= 100;
			}

            [[nodiscard]] bool IsEmpty() const {
				return names.empty();
            }
		};

		using Affix = BaseNamesContainer;
		
		struct GenderNames: BaseNamesContainer
		{
			Affix prefix;
			Affix suffix;
		};

		struct NamePart
		{
			struct Behavior
			{
				/// Flag indicating that NamePart using this behavior should inherit the same NamePart
				///	from the next Name Definition if it fails to pick a name from the current one.
				bool shouldInherit = false;

				/// Flag indicating that name part using this behavior should pair prefix and suffix.
				///	This behavior makes Name Definition pick one index that will be used for both prefix and suffix.
				///	If either of prefix or suffix has a non 100% chance then it will determine whether both of them will be used or none.
				///	If both of them has a less than 100% chance, than the prefix's chance will be used.
				bool useCircumfix = false;
			};

			GenderNames male;
			GenderNames female;
			GenderNames any;
			
			Behavior behavior;

			/// Checks whether the NamePart will always produce a name.
			///	NamePart is considered static when it has at least one not empty gender section and all present sections are static.
			[[nodiscard]] bool IsStatic() const {
				return male.IsStatic() && female.IsStatic() && any.IsStatic();
			}

			/// NamePart is considered empty when all its gender names sections are empty.
			[[nodiscard]] bool IsEmpty() const {
				return male.IsEmpty() && female.IsEmpty() && any.IsEmpty();
			}
		};

		struct Conjunctions
		{
			NamesList male;
			NamesList female;
			NamesList any = { " " };
		};

		NamePart firstName;
		NamePart lastName;

        Conjunctions conjunction;


		Behavior behavior;

		[[nodiscard]] bool HasDefaultScopes() const {
			return behavior.HasDefaultScopes();
		}
    };
}
