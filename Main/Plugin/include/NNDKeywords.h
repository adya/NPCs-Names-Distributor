#pragma once
namespace NND
{
	inline constexpr std::string_view uniqueEDID{ "NNDUnique" };
	inline RE::BGSKeyword* unique{ nullptr };

	inline constexpr std::string_view titlelessEDID{ "NNDTitleless" };
	inline RE::BGSKeyword* titleless{ nullptr };

	inline constexpr std::string_view obscureEDID{ "NNDObscured" };
	inline RE::BGSKeyword* obscure{ nullptr };

	/// Caches built-in keywords.
	///
	///	Returns flag indicating whether all keywords have been cached.
	///	If either of them wasn't NND must be aborted.
	inline bool CacheKeywords() {
		logger::info("{:*^30}", "KEYWORDS");
		if (const auto& dataHandler = RE::TESDataHandler::GetSingleton()) {
			auto& keywords = dataHandler->GetFormArray<RE::BGSKeyword>();

			auto findUnique = [&](const auto& keyword) { return keyword && keyword->formEditorID == uniqueEDID.data(); };
			auto findObscured = [&](const auto& keyword) { return keyword && keyword->formEditorID == obscureEDID.data(); };
			auto findTitleless = [&](const auto& keyword) { return keyword && keyword->formEditorID == titlelessEDID.data(); };

			if (const auto result = std::ranges::find_if(keywords, findUnique); result != keywords.end()) {
				logger::info("Cached {}", uniqueEDID);
				unique = *result;
			}
			if (const auto result = std::ranges::find_if(keywords, findObscured); result != keywords.end()) {
				logger::info("Cached {}", obscureEDID);
				obscure = *result;
			}
			if (const auto result = std::ranges::find_if(keywords, findTitleless); result != keywords.end()) {
				logger::info("Cached {}", titlelessEDID);
				titleless = *result;
			}
		
			// If either of those wasn't found - create them.
			if (!unique || !obscure || !titleless) {
				if (const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSKeyword>()) {
					if (!unique) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = uniqueEDID;
							keywords.push_back(keyword);
							unique = keyword;
							logger::info("Created {}", uniqueEDID);
						}
					}
					if (!obscure) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = obscureEDID;
							keywords.push_back(keyword);
							obscure = keyword;
							logger::info("Created {}", obscureEDID);
						}
					}
					if (!titleless) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = titlelessEDID;
							keywords.push_back(keyword);
							titleless = keyword;
							logger::info("Created {}", titlelessEDID);
						}
					}
				}
			}
		}
		return unique && titleless && obscure;
	}
}
