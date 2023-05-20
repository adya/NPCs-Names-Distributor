#include "NameDefinition.h"
#include <ClibUtil/rng.hpp>

namespace NND
{
	inline clib_util::RNG staticRNG{};

	inline bool AssignRandomNameVariant(const NameDefinition::NamesVariant& variant, bool useCircumfix, NameRef* nameComp, NameRef* prefixComp, NameRef* suffixComp) {
		*nameComp = variant.GetRandom().first;
		if (*nameComp == empty)
			return false;
		// Reset adfixes if components already had one (from previous definition)
		*prefixComp = empty;
		*suffixComp = empty;
		// Oh, yeah. This doesn't look too good :)
		if (useCircumfix) {
			if (const auto circumfixSize = std::min(variant.prefix.GetSize(), variant.suffix.GetSize()); circumfixSize > 0) {
				if (const auto [prefix, index] = variant.prefix.GetRandom(circumfixSize); prefix != empty) {
					if (const auto suffix = variant.suffix.GetNameAt(index); suffix != empty) {
						*prefixComp = prefix;
						*suffixComp = suffix;
					}
				}
			}
		} else {
			*prefixComp = variant.prefix.GetRandomName();
			*suffixComp = variant.suffix.GetRandomName();
		}

		return *nameComp != empty;
	}

	bool NameDefinition::GetRandomFullName(const RE::SEX sex, NameComponents& components) const {
		return GetRandomFirstName(sex, components) ||
		       GetRandomMiddleName(sex, components) ||
		       GetRandomLastName(sex, components) ||
		       GetRandomConjunction(sex, components);
	}

	bool NameDefinition::GetRandomFirstName(RE::SEX sex, NameComponents& components) const {
		return AssignRandomNameVariant(firstName.GetVariant(sex),
		                               firstName.useCircumfix,
		                               &components.firstName,
		                               &components.firstPrefix,
		                               &components.firstSuffix);
	}

	bool NameDefinition::GetRandomMiddleName(RE::SEX sex, NameComponents& components) const {
		return AssignRandomNameVariant(middleName.GetVariant(sex),
		                               middleName.useCircumfix,
		                               &components.middleName,
		                               &components.middlePrefix,
		                               &components.middleSuffix);
	}

	bool NameDefinition::GetRandomLastName(RE::SEX sex, NameComponents& components) const {
		return AssignRandomNameVariant(lastName.GetVariant(sex),
		                               lastName.useCircumfix,
		                               &components.lastName,
		                               &components.lastPrefix,
		                               &components.lastSuffix);
	}

	bool NameDefinition::GetRandomConjunction(RE::SEX sex, NameComponents& components) const {
		components.conjunction = conjunction.GetRandom(sex);
		return components.conjunction != empty;
	}

	/// Gets NamesVariant that matches given `sex`.
	const NameDefinition::NamesVariant& NameDefinition::NameSegment::GetVariant(const RE::SEX sex) const {
		if (sex == RE::SEX::kMale) {
			return male.IsEmpty() ? any : male;
		}
		if (sex == RE::SEX::kFemale) {
			return female.IsEmpty() ? any : female;
		}
		return any;
	}

	std::optional<Name> NameComponents::Assemble() const {
		if (!IsValid()) 
			return std::nullopt;

		NamesList names{};
		if (firstName != empty) {
			names.push_back(std::string(firstPrefix) + std::string(firstName) + std::string(firstSuffix));
		}
		if (middleName != empty) {
			names.push_back(std::string(middlePrefix) + std::string(middleName) + std::string(middleSuffix));
		}
		if (lastName != empty) {
			names.push_back(std::string(lastPrefix) + std::string(lastName) + std::string(lastSuffix));
		}
		return clib_util::string::join(names, conjunction);
	}

	std::optional<Name> NameComponents::AssembleShort() const {
		if (!IsValid())
			return std::nullopt;

		NamesList names{};
		if (has(shortSegments, NameSegmentType::kFirst) && firstName != empty) {
			names.push_back(std::string(firstPrefix) + std::string(firstName) + std::string(firstSuffix));
		}
		if (has(shortSegments, NameSegmentType::kMiddle) && middleName != empty) {
			names.push_back(std::string(middlePrefix) + std::string(middleName) + std::string(middleSuffix));
		}
		if (has(shortSegments, NameSegmentType::kLast) && lastName != empty) {
			names.push_back(std::string(lastPrefix) + std::string(lastName) + std::string(lastSuffix));
		}
		return clib_util::string::join(names, conjunction);
	}

	std::pair<NameRef, NameIndex> NameDefinition::BaseNamesContainer::GetRandom(NameIndex maxIndex) const {
		if (!IsDisabled() && (IsStatic() || chance > staticRNG.Generate<uint32_t>(0, 100))) {
			auto index = staticRNG.Generate<NameIndex>(0, std::min(maxIndex, GetSize()-1));
			auto& newName = names.at(index);
			return { newName, index };
		}
		return { empty, 0 };
	}

	const NamesList& NameDefinition::Conjunctions::GetList(const RE::SEX sex) const {
		if (sex == RE::SEX::kMale) {
			return male.empty() ? any : male;
		}
		if (sex == RE::SEX::kFemale) {
			return female.empty() ? any : female;
		}
		return any;
	}

	NameRef NameDefinition::Conjunctions::GetRandom(const RE::SEX sex) const {
		if (auto& list = GetList(sex); !list.empty()) {
			auto& newName = list.at(staticRNG.Generate<NameIndex>(0, list.size()-1));
			return newName;
		}
		return empty;
	}
}
