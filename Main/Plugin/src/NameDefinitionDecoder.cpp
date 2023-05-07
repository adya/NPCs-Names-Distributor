#include "NameDefinitionDecoder.h"
#include "json.hpp"
#include <fstream>

namespace NND
{
	using json = nlohmann::json;

	static constexpr auto kFirst = "First"sv;
	static constexpr auto kMiddle = "Middle"sv;
	static constexpr auto kLast = "Last"sv;
	static constexpr auto kConjunctions = "Conjunctions"sv;

	static constexpr auto kMale = "Male"sv;
	static constexpr auto kFemale = "Female"sv;
	static constexpr auto kAny = "Any"sv;

	static constexpr auto kNames = "Names"sv;
	static constexpr auto kChance = "Chance"sv;

	static constexpr auto kPrefix = "Prefix"sv;
	static constexpr auto kSuffix = "Suffix"sv;
	static constexpr auto kExclusive = "Exclusive"sv;

	static constexpr auto kBehavior = "Behavior"sv;
	static constexpr auto kInherit = "Inherit"sv;
	static constexpr auto kCircumfix = "Circumfix"sv;
	static constexpr auto kScopes = "Scopes"sv;

	static constexpr auto kScopeName = "Name"sv;
	static constexpr auto kScopeTitle = "Title"sv;
	static constexpr auto kScopeObscuring = "Obscuring"sv;

	using NameSegment = NameDefinition::NameSegment;
	using Conjunctions = NameDefinition::Conjunctions;
	using NamesVariant = NameDefinition::NamesVariant;

	namespace convert
	{		
		void from_json(const json& j, NameDefinition::BaseNamesContainer& p) {
			try {
				j.at(kNames).get_to(p.names);
			} catch (const json::out_of_range& e) {}

			try {
				j.at(kChance).get_to(p.chance);
			} catch (const json::out_of_range& e) {}
		}

		void from_json(const json& j, NameSegment::Behavior& p) {
			try {
				j.at(kInherit).get_to(p.shouldInherit);
			} catch (const json::out_of_range& e) {}

			try {
				j.at(kCircumfix).get_to(p.useCircumfix);
			} catch (const json::out_of_range& e) {}
		}

		void from_json(const json& j, NameDefinition::Adfix& p) {
			from_json(j, static_cast<NameDefinition::BaseNamesContainer&>(p));
			try {
				j.at(kExclusive).get_to(p.exclusive);
			} catch (const json::out_of_range& e) {}
		}

	    void from_json(const json& j, NamesVariant& p) {
			from_json(j, static_cast<NameDefinition::BaseNamesContainer&>(p));
			try {
				from_json(j.at(kPrefix), p.prefix);
			} catch (const json::out_of_range& e) {}

			try {
				from_json(j.at(kSuffix), p.suffix);
			} catch (const json::out_of_range& e) {}
	    }

		void from_json(const json& j, NameDefinition::Behavior& p) {
			try {
				if (const auto scopes = j.at(kScopes).get<std::set<std::string_view>>(); scopes.empty()) {
					p.useForNames = scopes.contains(kScopeName);
					p.useForTitles = scopes.contains(kScopeTitle);
					p.useForObscuring = scopes.contains(kScopeObscuring);
				}
		    }
		    catch (const json::out_of_range& e) {}

			try {
				j.at(kChance).get_to(p.chance);
			} catch (const json::out_of_range& e) {}
		}

	    void from_json(const json& j, Conjunctions& p) {
			try {
				j.at(kMale).get_to(p.male);
			} catch (const json::out_of_range& e) {}

			try {
				j.at(kFemale).get_to(p.female);
			} catch (const json::out_of_range& e) {}

			try {
				j.at(kAny).get_to(p.any);
			} catch (const json::out_of_range& e) {}
		}

		void from_json(const json& j, NameSegment& p) {
			try {
				from_json(j.at(kMale), p.male);
			} catch (const json::out_of_range& e) {}

			try {
				from_json(j.at(kFemale), p.female);
			} catch (const json::out_of_range& e) {}

			try {
				from_json(j.at(kAny), p.any);
			} catch (const json::out_of_range& e) {}

			try {
			    from_json(j.at(kBehavior), p.behavior);
			} catch (const json::out_of_range& e) {}
		}

		void from_json(const json& j, NameDefinition& p) {
			auto hasNames = false;
			try {
			    from_json(j.at(kFirst), p.firstName);
				hasNames = true;
			}
		    catch (const json::out_of_range& e) {}
			try {
				from_json(j.at(kMiddle), p.middleName);
				hasNames = true;
			} catch (const json::out_of_range& e) {}

			try {
				from_json(j.at(kLast), p.lastName);
				hasNames = true;
			} catch (const json::out_of_range& e) {}
			
			if (!hasNames) {
				logger::warn("\t\tNo name sections were found. Name Definition will be skipped.");
				return;
			}
			try {
				from_json(j.at(kConjunctions), p.conjunction);
			} catch (const json::out_of_range& e) {}

			try {
		        from_json(j.at(kBehavior), p.behavior);
			} catch (const json::out_of_range& e) {}
		}
	}

	/// May throw json::parse_error
	json modernize(const std::filesystem::path& a_path) {
		std::ifstream ifile(a_path);
		json          data = nlohmann::json::parse(ifile);
		ifile.close();

		const auto flat = data.flatten();
		json modernized{};
		bool wasModernized = false;
		for (auto& it : flat.items()) {
			auto key = it.key();
			// truncate obsolete NND_ prefix
			wasModernized |= clib_util::string::replace_all(key, "NND_", "");
			// Replace Given/Family with more universal terms for name parts.
			wasModernized |= clib_util::string::replace_first_instance(key, "Given", "First");
			wasModernized |= clib_util::string::replace_first_instance(key, "Family", "Last");
			// And Combine was renamed to Inherit.
			wasModernized |= clib_util::string::replace_first_instance(key, "Combine", "Inherit");
			modernized[key] = it.value();
		}
		if (wasModernized) {
			logger::info("Updating to use latest format");
			modernized = modernized.unflatten();
			std::ofstream ofile(a_path);
			ofile << std::setw(4) << modernized << std::endl;
			return modernized;
		}

		return data;
	}

	NameDefinition NameDefinitionDecoder::decode(const std::filesystem::path& a_path) {
		std::ifstream f(a_path);
		const json data = modernize(a_path);
		NameDefinition definition{};
		convert::from_json(data, definition);
		return definition;
	}
}
