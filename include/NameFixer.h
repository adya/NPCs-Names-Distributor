#pragma once
namespace NND
{
	namespace NameFixer
	{
		namespace details
		{
			inline bool IsNestedName(std::string_view name) {
				return std::ranges::count(name, '(') > 1 ||
				       std::ranges::count(name, ';') > 1 ||
				       std::ranges::count(name, ',') > 1 ||
				       std::ranges::count(name, '.') > 1 ||
				       std::ranges::count(name, '[') > 1;
			}

			inline bool NeedsFixing(std::string_view name) {
				return name == empty || name == "???" || IsNestedName(name);
			}
		}

		inline bool FixName(RE::Actor* actor) {
			if (const auto data = actor->extraList.GetExtraTextDisplayData()) {
				if (const auto name = data->GetDisplayName(actor->GetActorBase(), 0)) {
					if (details::NeedsFixing(name)) {
						actor->extraList.Remove(RE::ExtraDataType::kTextDisplayData, data);
						logger::info("{}'s name has been fixed.", actor->GetActorBase()->GetFullName());
						// Immediately refresh the name for NPC we are looking at.
						RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
						return true;
					}
				}
			}
			logger::info("{} does not need a fix.", actor->GetActorBase()->GetFullName());
			return false;
		}

		inline bool FixNameUnsafe(RE::Actor* actor) {
			if (const auto data = actor->extraList.GetExtraTextDisplayData()) {
				if (const auto name = data->GetDisplayName(actor->GetActorBase(), 0)) {
					actor->extraList.Remove(RE::ExtraDataType::kTextDisplayData, data);
					logger::info("{}'s name has been fixed.", actor->GetActorBase()->GetFullName());
					// Immediately refresh the name for NPC we are looking at.
					RE::PlayerCharacter::GetSingleton()->UpdateCrosshairs();
					return true;
				}
			}
			logger::info("{} does not need a fix.", actor->GetActorBase()->GetFullName());
			return false;
		}
	}
}
