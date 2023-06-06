#include "LookupNameDefinitions.h"
#include "CLIBUtil/distribution.hpp"
#include "NameDefinitionDecoder.h"
#include "crc32.h"

namespace NND
{
	void LogNamesVariant(std::string_view name, const NameDefinition::NamesVariant& variant, bool useCircumfix) {
		if (const auto size = variant.names.size(); size > 0) {
			logger::info("\t\t{}: {}", name, size);
			const auto prefix = variant.prefix.names.size();
			const auto suffix = variant.suffix.names.size();
			if (useCircumfix) {
				const auto circumfix = min(prefix, suffix);
				if (circumfix > 0) {
					const auto trimmedPrefixes = prefix - circumfix;
					const auto trimmedSuffixes = suffix - circumfix;
					logger::info("\t\t\tCircumfix: {}", circumfix);
					if (trimmedPrefixes > 0) {
						logger::warn("\t\t\tWARN: {} last prefixes will be ignored", trimmedPrefixes);
					} else if (trimmedSuffixes > 0) {
						logger::warn("\t\t\tWARN: {} last suffixes will be ignored", trimmedSuffixes);
					}
				}
			} else {
				if (prefix > 0) {
					logger::info("\t\t\tPrefix: {}", prefix);
				}
				if (suffix > 0) {
					logger::info("\t\t\tSuffix: {}", suffix);
				}
			}
		}
	}

	void LogNameSegment(std::string_view name, const NameDefinition::NameSegment& namePart) {
		const auto inherits = namePart.shouldInherit;

		if (!inherits && namePart.IsEmpty())
			return;

		logger::info("\t{} Names:", name);
		if (inherits) {
			logger::info("\t\tInherits from the next Name Definition", name);
			if (namePart.IsStatic() && !namePart.IsEmpty()) {
				logger::warn("\t\tWARN: Redundant inheritance, since {} Name will always be produced by this Name Definition", name);
			}
		}

		LogNamesVariant("Male"sv, namePart.male, namePart.useCircumfix);
		LogNamesVariant("Female"sv, namePart.female, namePart.useCircumfix);
		LogNamesVariant("Any"sv, namePart.any, namePart.useCircumfix);
	}

	void LogDefinition(const NameDefinition& definition) {
		std::vector<std::string> scopes{};
		if (has(definition.scope, NameDefinition::Scope::kName)) {
			scopes.emplace_back("Names");
		}
		if (has(definition.scope, NameDefinition::Scope::kTitle)) {
			scopes.emplace_back("Titles");
		}
		if (has(definition.scope, NameDefinition::Scope::kObscurity)) {
			scopes.emplace_back("Obscurity");
		}

		if (!definition.HasDefaultScopes()) {
			logger::info("\tUsed for {}", clib_util::string::join(scopes, " and "));
		}

		LogNameSegment("First"sv, definition.firstName);
		LogNameSegment("Last"sv, definition.lastName);
	}

	uint32_t ComputeCRC(std::filesystem::path path) {
		std::ifstream         file(path, std::ios::binary | std::ios::ate);
		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		auto buffer = new char[size];
		if (file.read(buffer, size)) {
			return crc32_fast(buffer, size);
		}
		return 0;
	}

	bool LoadNameDefinitions() {
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
			logger::info("Loading \"{}\"", name);
			try {
				auto definition = decoder.decode(file);
				definition.crc32 = ComputeCRC(file);
				definition.name = name;
				if (has(definition.scope, NameDefinition::Scope::kName)) {
					loadedDefinitions[NameDefinition::Scope::kName][name] = definition;
				}
				if (has(definition.scope, NameDefinition::Scope::kTitle)) {
					loadedDefinitions[NameDefinition::Scope::kTitle][name] = definition;
				}
				if (has(definition.scope, NameDefinition::Scope::kObscurity)) {
					loadedDefinitions[NameDefinition::Scope::kObscurity][name] = definition;
				}
				LogDefinition(definition);
				++validFiles;
			} catch (const std::exception& error) {
				logger::critical("\tFailed to decode Name Definition {} with error: {} ", name, error.what());
			} catch (...) {
				logger::critical("\tFailed to decode Name Definition {} with error: {} ", name, "Unknown exception occurred.");
			}
		}
		return validFiles > 0;
	}

	Snapshot MakeSnapshot() {
		std::set<std::string> snapshots{};

		for (const auto& scope : loadedDefinitions) {
			for (const auto& [name, definition] : scope.second) {
				std::stringstream stream{};
				stream << name
					   << "@"
					   << std::setfill('0')
					   << std::setw(sizeof(uint32_t) * 2)
					   << std::uppercase
					   << std::hex
					   << definition.crc32;
				snapshots.insert(stream.str());
			}
		}
		return snapshots;
	}

}
