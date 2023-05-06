#include "LookupNameDefinitions.h"
#include "CLIBUtil/distribution.hpp"
#include "NameDefinitionDecoder.h"

namespace NND
{
	void LogGenderNames(std::string_view gender, const NameDefinition::GenderNames& genderNames, bool useCircumfix)
	{
		if (const auto size = genderNames.names.size(); size > 0) {
			logger::info("\t\t\t{}: {}", gender, size);
			const auto prefix = genderNames.prefix.names.size();
			const auto suffix = genderNames.suffix.names.size();
			if (useCircumfix) {
				const auto circumfix = min(prefix, suffix);
				if (circumfix > 0) {
					const auto trimmedPrefixes = prefix - circumfix;
					const auto trimmedSuffixes = suffix - circumfix;
					logger::info("\t\t\t\tCircumfix: {}", circumfix);
					if (trimmedPrefixes > 0) {
						logger::warn("\t\t\t\tWARN: {} last prefixes will be ignored", trimmedPrefixes);
					} else if (trimmedSuffixes > 0) {
						logger::warn("\t\t\t\tWARN: {} last suffixes will be ignored", trimmedSuffixes);
					}
				}
			} else {
				if (prefix > 0) {
					logger::info("\t\t\t\tPrefix: {}", prefix);
				}
				if (suffix > 0) {
					logger::info("\t\t\t\tSuffix: {}", suffix);
				}
			}
		}
	}

	void LogDefinitionPart(std::string_view name, const NameDefinition::NamePart& namePart)
	{
		const auto inherits = namePart.behavior.shouldInherit;

		if (!inherits && namePart.IsEmpty())
			return;

		logger::info("\t\t{} Names:", name);
		if (inherits) {
			logger::info("\t\t\tInherits from the next Name Definition", name);
			if (namePart.IsStatic() && !namePart.IsEmpty()) {
				logger::warn("\t\t\tWARN: Redundant inheritance, since {} Name will always be produced by this Name Definition", name);
			}
		}

		LogGenderNames("Male"sv, namePart.male, namePart.behavior.useCircumfix);
		LogGenderNames("Female"sv, namePart.female, namePart.behavior.useCircumfix);
		LogGenderNames("Any"sv, namePart.any, namePart.behavior.useCircumfix);
	}

	void LogDefinition(const NameDefinition& definition)
	{
		std::vector<std::string> scopes{};
		if (definition.behavior.useForNames) {
			scopes.emplace_back("Names");
		}
		if (definition.behavior.useForTitles) {
			scopes.emplace_back("Titles");
		}
		if (definition.behavior.useForObscuring) {
			scopes.emplace_back("Obscurity");
		}

		if (!definition.HasDefaultScopes()) {
			logger::info("\t\tUsed for {}", clib_util::string::join(scopes, " and "));
		}

		LogDefinitionPart("First"sv, definition.firstName);
		LogDefinitionPart("Last"sv, definition.lastName);
	}

	bool LoadNameDefinitions()
	{
		logger::info("{:*^30}", "NAME DEFINITIONS");
		const auto files = clib_util::distribution::get_configs_paths(R"(Data\SKSE\Plugins\NPCsNamesDistributor)", ".json"sv);

		if (files.empty()) {
			logger::info("No Name Definition files found. NPCsNamesDistributor will be disabled.");
			logger::info(R"(Make sure your Name Definition files are located at Data\SKSE\Plugins\NPCsNamesDistributor)");
			return false;
		}
		logger::info("{} Name Definition files found", files.size());
		int                   validFiles = 0;
		NameDefinitionDecoder decoder{};
		for (const auto& file : files) {
			const auto name = file.stem().string();
			logger::info("\tLoading \"{}\"", name);
			try {
				auto definition = decoder.decode(file);
				definitions[name] = definition;
				LogDefinition(definition);
				++validFiles;
			} catch (const std::exception& error) {
				logger::critical("\t\tFailed to decode Name Definition {} with error: {} ", name, error.what());
			}
		}
		return validFiles > 0;
	}
}
