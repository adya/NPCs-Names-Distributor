#include "NameDefinitionDecoder.h"
#include "json.hpp"
#include <fstream>

namespace NND
{
	using json = nlohmann::json;

	static constexpr auto kFirst = "First"sv;
	static constexpr auto kLast = "Last"sv;
	static constexpr auto kConjunctions = "Conjunctions"sv;

	static constexpr auto kMale = "Male"sv;
	static constexpr auto kFemale = "Female"sv;
	static constexpr auto kAny = "Any"sv;

	static constexpr auto kNames = "Names"sv;
	static constexpr auto kChance = "Chance"sv;

	static constexpr auto kPrefix = "Prefix"sv;
	static constexpr auto kSuffix = "Suffix"sv;
	static constexpr auto kCircumfix = "Circumfix"sv;

	static constexpr auto kBehavior = "Behavior"sv;
	static constexpr auto kCombine = "Combine"sv;
	static constexpr auto kScopes = "Scopes"sv;

	static constexpr auto kScopeName = "Obscuring"sv;
	static constexpr auto kScopeTitle = "Title"sv;
	static constexpr auto kScopeObscuring = "Obscuring"sv;

	using key = std::string_view;
	using either_key = std::vector<std::string_view>;

	using NamePart = NameDefinition::NamePart;
	using GenderNames = NameDefinition::GenderNames;

	// To be moved to clib_util
	namespace clib_util
	{
		namespace string
		{
			inline bool replace_all(std::string& a_str, std::string_view a_search, std::string_view a_replace)
			{
				if (a_search.empty()) {
					return false;
				}

				std::size_t pos = 0;
				bool        wasReplaced = false;
				while ((pos = a_str.find(a_search, pos)) != std::string::npos) {
					a_str.replace(pos, a_search.length(), a_replace);
					pos += a_replace.length();
					wasReplaced = true;
				}

				return wasReplaced;
			}

			inline bool replace_first_instance(std::string& a_str, std::string_view a_search, std::string_view a_replace)
			{
				if (a_search.empty()) {
					return false;
				}

				if (const std::size_t pos = a_str.find(a_search); pos != std::string::npos) {
					a_str.replace(pos, a_search.length(), a_replace);
					return true;
				}

				return false;
			}

			inline bool replace_last_instance(std::string& a_str, std::string_view a_search, std::string_view a_replace)
			{
				if (a_search.empty()) {
					return false;
				}

				if (const std::size_t pos = a_str.rfind(a_search); pos != std::string::npos) {
					a_str.replace(pos, a_search.length(), a_replace);
					return true;
				}

				return false;
			}
		}

	}

	// May throw
	json modernize(const std::filesystem::path& a_path)
	{
		std::ifstream ifile(a_path);
		json          data = nlohmann::json::parse(ifile);
		ifile.close();
		auto flat = data.flatten();
		json modernized{};
		bool wasModernized = false;
		for (auto& it : flat.items()) {
			auto key = it.key();
			if (clib_util::string::replace_all(key, "NND_", "") ||                   // truncate obsolete NND_ prefix
				clib_util::string::replace_first_instance(key, "Given", "First") ||  // Replace Given/Family with more universal terms for name parts.
				clib_util::string::replace_first_instance(key, "Family", "Last")) {
				wasModernized = true;
			}
			modernized[key] = it.value();
		}
		if (wasModernized) {
			modernized = modernized.unflatten();
			std::ofstream ofile(a_path);
			ofile << std::setw(4) << modernized << std::endl;
			return modernized;
		}

		return data;
	}

	NameDefinition NameDefinitionDecoder::decode(const std::filesystem::path& a_path)
	{
		std::ifstream f(a_path);
		try {
			const json data = modernize(a_path);
			/*NameDefinition definition{};

                details::decodeNamePart(data, { kFirst, kGiven, prefixed(kGiven) });*/
			return {};
		} catch (const std::exception& error) {
			logger::critical("Encountered an error: {}", error.what());
			// maybe rethrow custom error?
			return {};
		}
	}
}
