#include "NameDefinitionDecoder.h"
#include "CLIBUtil/distribution.hpp"
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

	static constexpr auto kInherit = "Inherit"sv;
	static constexpr auto kCircumfix = "Circumfix"sv;

	static constexpr auto kScopes = "Scopes"sv;
	static constexpr auto kPriority = "Priority"sv;
	static constexpr auto kShorten = "Shortened"sv;

	static constexpr auto kScopeName = "Name"sv;
	static constexpr auto kScopeTitle = "Title"sv;
	static constexpr auto kScopeObscuring = "Obscuring"sv;

	using NameSegment = NameDefinition::NameSegment;
	using Conjunctions = NameDefinition::Conjunctions;
	using NamesVariant = NameDefinition::NamesVariant;
	using Scope = NameDefinition::Scope;
	using Segment = NameSegmentType;
	using Priority = NameDefinition::Priority;

	namespace convert
	{
		/// An ordered array of strings representing all supported priorities.
		/// This array must be ordered accordingly to NameDefinition::Priority enum, so that rawPriorities[Priority::kXXX] will always yield correct mapping.
		inline constexpr std::array rawPriorities{ "Race"sv, "Class"sv, "Faction"sv, "Clan"sv, "Individual"sv };

		constexpr Priority operator++(Priority& type) {
			return type = static_cast<Priority>(static_cast<uint8_t>(type) + 1);
		}

		static constexpr std::string_view toRawPriority(Priority priority) {
			return rawPriorities[priority];
		}

		static constexpr NameDefinition::Priority fromRawPriority(const std::string_view& raw) {
			for (Priority priority = Priority::kDefault; priority < Priority::kTotal; ++priority) {
				if (rawPriorities[priority] == raw) {
					return priority;
				}
			}
			return Priority::kDefault;
		}

		void from_json(const json& j, NameDefinition::BaseNamesContainer& p) {
			try {
				j.at(kNames).get_to(p.names);
			} catch (const json::out_of_range&) {}

			try {
				j.at(kChance).get_to(p.chance);
			} catch (const json::out_of_range&) {}
		}

		void from_json(const json& j, NameDefinition::Adfix& p) {
			from_json(j, static_cast<NameDefinition::BaseNamesContainer&>(p));
			try {
				j.at(kExclusive).get_to(p.exclusive);
			} catch (const json::out_of_range&) {}
		}

		void from_json(const json& j, NamesVariant& p) {
			from_json(j, static_cast<NameDefinition::BaseNamesContainer&>(p));
			try {
				from_json(j.at(kPrefix), p.prefix);
			} catch (const json::out_of_range&) {}

			try {
				from_json(j.at(kSuffix), p.suffix);
			} catch (const json::out_of_range&) {}
		}

		void from_json(const json& j, Conjunctions& p) {
			try {
				j.at(kMale).get_to(p.male);
			} catch (const json::out_of_range&) {}

			try {
				j.at(kFemale).get_to(p.female);
			} catch (const json::out_of_range&) {}

			try {
				j.at(kAny).get_to(p.any);
			} catch (const json::out_of_range&) {}
		}

		void from_json(const json& j, NameSegment& p) {
			try {
				from_json(j.at(kMale), p.male);
			} catch (const json::out_of_range&) {}

			try {
				from_json(j.at(kFemale), p.female);
			} catch (const json::out_of_range&) {}

			try {
				from_json(j.at(kAny), p.any);
			} catch (const json::out_of_range&) {}

			try {
				j.at(kInherit).get_to(p.shouldInherit);
			} catch (const json::out_of_range&) {}

			try {
				j.at(kCircumfix).get_to(p.useCircumfix);
			} catch (const json::out_of_range&) {}
		}

		void from_json(const json& j, NameDefinition& p) {
			auto hasNames = false;
			try {
				from_json(j.at(kFirst), p.firstName);
				hasNames = true;
			} catch (const json::out_of_range&) {}
			try {
				from_json(j.at(kMiddle), p.middleName);
				hasNames = true;
			} catch (const json::out_of_range&) {}

			try {
				from_json(j.at(kLast), p.lastName);
				hasNames = true;
			} catch (const json::out_of_range&) {}

			if (!hasNames) {
				logger::warn("\t\tNo name sections were found. Name Definition will be skipped.");
				return;
			}
			try {
				from_json(j.at(kConjunctions), p.conjunction);
			} catch (const json::out_of_range&) {}

			try {
				if (const auto shorten = j.at(kShorten).get<std::set<std::string_view>>(); !shorten.empty()) {
					if (shorten.contains(kFirst))
						enable(p.shortened, Segment::kFirst);
					if (shorten.contains(kMiddle))
						enable(p.shortened, Segment::kMiddle);
					if (shorten.contains(kLast))
						enable(p.shortened, Segment::kLast);
				}
			} catch (const json::out_of_range&) {}

			try {
				if (const auto scopes = j.at(kScopes).get<std::set<std::string_view>>(); !scopes.empty()) {
					p.scope = Scope::kNone;  // if we found kScopes property, then we reset default scope.
					if (scopes.contains(kScopeName))
						enable(p.scope, Scope::kName);
					if (scopes.contains(kScopeTitle))
						enable(p.scope, Scope::kTitle);
					if (scopes.contains(kScopeObscuring))
						enable(p.scope, Scope::kObscurity);
				}
			} catch (const json::out_of_range&) {}

			try {
				p.priority = fromRawPriority(j.at(kPriority).get<std::string_view>());
			} catch (const json::out_of_range&) {}
		}
	}

	/// May throw json::parse_error
	json modernize(const std::filesystem::path& a_path) {
		std::ifstream ifile(a_path);
		json          data = nlohmann::json::parse(ifile);
		ifile.close();

		const auto flat = data.flatten();
		json       modernized{};
		bool       wasModernized = false;

		// Replace legacy keys.
		for (auto& it : flat.items()) {
			std::string key = it.key();
			// truncate obsolete NND_ prefix
			wasModernized |= clib_util::string::replace_all(key, "NND_", "");
			// Replace Given/Family with more universal terms for name parts.
			wasModernized |= clib_util::string::replace_first_instance(key, "Given", "First");
			wasModernized |= clib_util::string::replace_first_instance(key, "Family", "Last");
			// And Combine was renamed to Inherit.
			wasModernized |= clib_util::string::replace_first_instance(key, "Combine", "Inherit");
			// Finally, replace Behavior/ path, since behaviors had been flattened
			wasModernized |= clib_util::string::replace_first_instance(key, "Behavior/", "");
			modernized[key] = it.value();
		}

		// Remove keyword priorities and write them to the Name Definition instead
		const auto distrs = clib_util::distribution::get_configs_paths("Data", "_DISTR"sv, ".ini"sv);
		for (const auto& distr : distrs) {
			std::ifstream ifile(distr);
			if (ifile.is_open()) {
				const std::string content((std::istreambuf_iterator<char>(ifile)),
				                          (std::istreambuf_iterator<char>()));
				ifile.close();
				std::string       name = a_path.stem().string();
				const std::string pattern = name + "_(Race|Class|Faction|Forced)";
				const std::regex  re(pattern);
				std::smatch       match;
				std::regex_search(content, match, re);
				if (match.size() > 1) {
					const std::string new_content = std::regex_replace(content, re, name);  // Replace all occurrences

					auto priority = match[1].str();
					if (priority == "Forced")  // Rename Forced priority
						priority = convert::toRawPriority(Priority::kIndividual);
					modernized["/Priority"] = priority;
					wasModernized = true;
					std::ofstream ofile(distr);  // Open the file for output
					if (ofile.is_open()) {
						ofile << new_content;  // Write the new string to the file
						ofile.close();         // Close the output file
						logger::info("Removed keyword priorities in \"{}\"", distr.filename().string());
					}
				}
			}
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

	NameDefinition NameDefinitionDecoder::decode(const std::filesystem::path& a_path) const {
		const json     data = modernize(a_path);
		NameDefinition definition{};
		convert::from_json(data, definition);
		return definition;
	}
}
