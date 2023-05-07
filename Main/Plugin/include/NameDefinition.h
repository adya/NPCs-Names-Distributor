#pragma once
namespace NND
{
	using Name = std::string;
	using NameRef = std::string_view;
	using NamesList = std::vector<Name>;

	constexpr inline NameRef empty = ""sv;

	struct NameComponents
	{
		NameRef firstPrefix = empty;
		NameRef firstName = empty;
		NameRef firstSuffix = empty;

		NameRef middlePrefix = empty;
		NameRef middleName = empty;
		NameRef middleSuffix = empty;

		NameRef lastPrefix = empty;
		NameRef lastName = empty;
		NameRef lastSuffix = empty;

		NameRef conjunction = empty;

		[[nodiscard]] bool IsValid() const {
			return firstName != empty ||
			       middleName != empty ||
			       lastName != empty;
		}
	};

	struct NameDefinition
	{
		struct Behavior
		{
			bool         useForNames = true;
			bool         useForTitles = false;
			bool         useForObscuring = false;
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

		struct Adfix : BaseNamesContainer
		{
			/// Flag indicating whether given Adfix can only be applied exclusively.
			///	When one of the exclusive adfixes is picked the other one will be skipped.
			bool exclusive = false;
		};

		struct NamesVariant : BaseNamesContainer
		{
			Adfix prefix;
			Adfix suffix;
		};

		struct NameSegment
		{
			struct Behavior
			{
				/// Flag indicating that NameSegment using this behavior should inherit the same NameSegment
				///	from the next Name Definition if it fails to pick a name from the current one.
				bool shouldInherit = false;

				/// Flag indicating that name part using this behavior should pair prefix and suffix.
				///	This behavior makes Name Definition pick one index that will be used for both prefix and suffix.
				///	If either of prefix or suffix has a non 100% chance then it will determine whether both of them will be used or none.
				///	If both of them has a less than 100% chance, than the prefix's chance will be used.
				bool useCircumfix = false;
			};

			NamesVariant male;
			NamesVariant female;
			NamesVariant any;

			Behavior behavior;

			/// Checks whether the NameSegment will always produce a name.
			///	NameSegment is considered static when it has at least one not empty NamesVariant and all present NamesVariants are static.
			[[nodiscard]] bool IsStatic() const {
				return male.IsStatic() && female.IsStatic() && any.IsStatic();
			}

			/// NameSegment is considered empty when all its NamesVariants are empty.
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

		NameSegment firstName;
		NameSegment middleName;
		NameSegment lastName;

		Conjunctions conjunction;

		Behavior behavior;

		[[nodiscard]] bool HasDefaultScopes() const {
			return behavior.HasDefaultScopes();
		}
	};
}
