#pragma once
#include "CLIBUtil/bitmasks.hpp"

namespace NND
{
	using Name = std::string;
	using NameRef = std::string_view;
	using NameIndex = size_t;
	using NamesList = std::vector<Name>;

	enum class NameSegmentType : uint8_t
	{
		kNone = 0b000,
		kFirst = 0b001,
		kMiddle = 0b010,
		kLast = 0b100,

		kAll = kFirst | kMiddle | kLast
	};

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

		NameSegmentType shortSegments = NameSegmentType::kNone;

		[[nodiscard]] bool IsValid() const {
			return firstName != empty ||
			       middleName != empty ||
			       lastName != empty;
		}

		[[nodiscard]] std::optional<Name> Assemble() const;
		[[nodiscard]] std::optional<Name> AssembleShort() const;
	};


	struct NameDefinition
	{
		/// Priority
		enum Priority : uint8_t
		{
			/// Definition with `kRace` priority contains base names specific for races or otherwise a large group that shares an innate trait that cannot be changed.
			/// This is the default one and may be omitted.
			kRace = 0,

			/// Default priority is `kRace`.
			kDefault = kRace,

			/// Definition with `kClass` priority contains names for a more narrow group that shares an innate trait, which also cannot be changed.
			kClass,

			/// Definition with `kFaction` priority contains names for a medium sized group that are united by some common idea or belief, that are scattered across the world.
			kFaction,

			/// Definition with `kClan` priority contains names for a small group that are united are united by some common idea or belief, but typically located in a single area.
			kClan,

			/// Definition with `kIndividual` priority contains names for distinct individuals that are usually hand-picked.
			kIndividual,

			kTotal
		};

		enum class Scope: uint8_t
		{
			kNone		= 0b000,

			kName		= 0b001,
			kTitle		= 0b010,
			kObscurity	= 0b100,

			kDefault = kName,
			kAll = kName | kTitle | kObscurity
		};

		struct BaseNamesContainer
		{
			NamesList names{};
			uint8_t   chance = 100;

			/// Checks whether given names container will always produce a name.
			///	Container is considered static when its chance is 100%.
			[[nodiscard]] bool IsStatic() const {
				return chance >= 100;
			}

			/// Checks whether given names container will never produce a name.
			[[nodiscard]] bool IsDisabled() const {
				return chance <= 0 || IsEmpty();
			}

			[[nodiscard]] bool IsEmpty() const {
				return names.empty();
			}

			[[nodiscard]] size_t GetSize() const {
				return names.size();
			}

			[[nodiscard]] std::pair<NameRef, NameIndex> GetRandom(NameIndex maxIndex) const;

			[[nodiscard]] std::pair<NameRef, size_t> GetRandom() const {
				return GetRandom(names.size()-1);
			}

			[[nodiscard]] NameRef GetRandomName(NameIndex maxIndex) const {
				return GetRandom(maxIndex).first;
			}

			[[nodiscard]] NameRef GetRandomName() const {
				return GetRandom().first;
			}

			[[nodiscard]] NameRef GetNameAt(NameIndex index) const {
				return names.at(index);
			}
		};

		struct Adfix : BaseNamesContainer
		{
			/// Flag indicating whether given Adfix can only be applied exclusively.
			///	When one of the exclusive Adfixes is picked the other one will be skipped.
			bool exclusive = false;
		};

		struct NamesVariant : BaseNamesContainer
		{
			Adfix prefix{};
			Adfix suffix{};
		};

		struct NameSegment
		{
			NamesVariant male{};
			NamesVariant female{};
			NamesVariant any{};

			/// Flag indicating that NameSegment using this behavior should inherit the same NameSegment
			///	from the next Name Definition if it fails to pick a name from the current one.
			bool shouldInherit = false;

			/// Flag indicating that name part using this behavior should pair prefix and suffix.
			///	This behavior makes Name Definition pick one index that will be used for both prefix and suffix.
			///	If either of prefix or suffix has a non 100% chance then it will determine whether both of them will be used or none.
			///	If both of them has a less than 100% chance, than the prefix's chance will be used.
			bool useCircumfix = false;

			/// Checks whether the NameSegment will always produce a name.
			///	NameSegment is considered static when it has at least one not empty NamesVariant and all present NamesVariants are static.
			[[nodiscard]] bool IsStatic() const {
				return male.IsStatic() && female.IsStatic() && any.IsStatic();
			}

			/// NameSegment is considered empty when all its NamesVariants are empty.
			[[nodiscard]] bool IsEmpty() const {
				return male.IsEmpty() && female.IsEmpty() && any.IsEmpty();
			}

			[[nodiscard]] const NamesVariant& GetVariant(RE::SEX sex) const;
		};

		struct Conjunctions
		{
			NamesList male{};
			NamesList female{};
			NamesList any = { " " };

			[[nodiscard]] NameRef          GetRandom(RE::SEX sex) const;
			[[nodiscard]] const NamesList& GetList(RE::SEX sex) const;
		};

		NameSegment firstName{};
		NameSegment middleName{};
		NameSegment lastName{};

		Conjunctions conjunction{};

		Priority priority = Priority::kDefault;

		Scope scope = Scope::kDefault;

		NameSegmentType shortened = NameSegmentType::kNone;

		/// Name of the definition.
		std::string name;

		uint32_t crc32 = 0;

		[[nodiscard]] bool HasDefaultScopes() const {
			return scope == Scope::kDefault;
		}

		bool GetRandomFullName(RE::SEX sex, NameComponents& components) const;
		bool GetRandomFirstName(RE::SEX sex, NameComponents& components) const;
		bool GetRandomMiddleName(RE::SEX sex, NameComponents& components) const;
		bool GetRandomLastName(RE::SEX sex, NameComponents& components) const;
		bool GetRandomConjunction(RE::SEX sex, NameComponents& components) const;
	};
}

template <>
struct enable_bitmask_operators<NND::NameDefinition::Scope>
{
	static constexpr bool enable = true;
};

template <>
struct enable_bitmask_operators<NND::NameSegmentType>
{
	static constexpr bool enable = true;
};
