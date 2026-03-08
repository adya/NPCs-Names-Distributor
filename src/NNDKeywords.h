#pragma once
namespace NND
{
	inline constexpr std::string_view uniqueEDID{ "NNDUnique" };
	inline RE::BGSKeyword*            unique{ nullptr };

	inline constexpr std::string_view disableDefaultTitleEDID{ "NNDDisableDefaultTitle" };
	inline RE::BGSKeyword*            disableDefaultTitle{ nullptr };

	inline constexpr std::string_view disableDefaultObscurityEDID{ "NNDDisableDefaultObscurity" };
	inline RE::BGSKeyword*            disableDefaultObscurity{ nullptr };

	inline constexpr std::string_view knownEDID{ "NNDKnown" };
	inline RE::BGSKeyword*            known{ nullptr };

	/// Caches built-in keywords.
	///
	///	Returns flag indicating whether all keywords have been cached.
	///	If either of them wasn't then NND must be aborted.
	inline bool CacheKeywords() {
		logger::info("{:*^30}", "KEYWORDS");
		if (const auto& dataHandler = RE::TESDataHandler::GetSingleton()) {
			auto& keywords = dataHandler->GetFormArray<RE::BGSKeyword>();

			auto findUnique = [&](const auto& keyword) { return keyword && keyword->formEditorID == uniqueEDID.data(); };
			auto findKnown = [&](const auto& keyword) { return keyword && keyword->formEditorID == knownEDID.data(); };
			auto findDefaultTitle = [&](const auto& keyword) { return keyword && keyword->formEditorID == disableDefaultTitleEDID.data(); };
			auto findDefaultObscurity = [&](const auto& keyword) { return keyword && keyword->formEditorID == disableDefaultObscurityEDID.data(); };

			if (const auto result = std::ranges::find_if(keywords, findUnique); result != keywords.end()) {
				logger::info("Cached {}", uniqueEDID);
				unique = *result;
			}
			if (const auto result = std::ranges::find_if(keywords, findKnown); result != keywords.end()) {
				logger::info("Cached {}", knownEDID);
				known = *result;
			}
			if (const auto result = std::ranges::find_if(keywords, findDefaultTitle); result != keywords.end()) {
				logger::info("Cached {}", disableDefaultTitleEDID);
				disableDefaultTitle = *result;
			}
			if (const auto result = std::ranges::find_if(keywords, findDefaultObscurity); result != keywords.end()) {
				logger::info("Cached {}", disableDefaultObscurityEDID);
				disableDefaultObscurity = *result;
			}

			// If either of those wasn't found - create them.
			if (!unique || !known || !disableDefaultTitle) {
				if (const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSKeyword>()) {
					if (!unique) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = uniqueEDID;
							keywords.push_back(keyword);
							unique = keyword;
							logger::info("Created {}", uniqueEDID);
						}
					}
					if (!known) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = knownEDID;
							keywords.push_back(keyword);
							known = keyword;
							logger::info("Created {}", knownEDID);
						}
					}
					if (!disableDefaultTitle) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = disableDefaultTitleEDID;
							keywords.push_back(keyword);
							disableDefaultTitle = keyword;
							logger::info("Created {}", disableDefaultTitleEDID);
						}
					}
					if (!disableDefaultObscurity) {
						if (const auto keyword = factory->Create()) {
							keyword->formEditorID = disableDefaultObscurityEDID;
							keywords.push_back(keyword);
							disableDefaultObscurity = keyword;
							logger::info("Created {}", disableDefaultObscurityEDID);
						}
					}
				}
			}
		}
		return unique && disableDefaultTitle && disableDefaultObscurity && known;
	}
}
