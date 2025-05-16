#include "ModAPI.h"
#include "Distributor.h"

namespace Messaging
{
	NND::NameStyle GetNameStyle(NameContext context) {
		switch (context) {
		case NameContext::kCrosshair:
			return NND::Options::NameContext::kCrosshair;
		case NameContext::kCrosshairMinion:
			return NND::Options::NameContext::kCrosshairMinion;
		case NameContext::kSubtitles:
			return NND::Options::NameContext::kSubtitles;
		case NameContext::kDialogue:
			return NND::Options::NameContext::kDialogue;
		case NameContext::kInventory:
			return NND::Options::NameContext::kInventory;
		case NameContext::kBarter:
			return NND::Options::NameContext::kBarter;
		case NameContext::kEnemyHUD:
			return NND::Options::NameContext::kEnemyHUD;
		case NameContext::kDialogueHistory:
			return NND::Options::NameContext::kDialogueHistory;
		default:
		case NameContext::kOther:
			return NND::Options::NameContext::kOther;
		}
	}

	std::string_view NNDInterface::GetName(RE::ActorHandle handle, NameContext context) noexcept {
		if (const auto actor = handle.get().get()) {
			return GetName(actor, context);
		}

		return ""sv;		
	}

	std::string_view NNDInterface::GetName(RE::Actor* actor, NameContext context) noexcept {
		if (actor) {
			const auto name = NND::Distribution::Manager::GetSingleton()->GetName(GetNameStyle(context), actor);

			// Special case for TrueHUD bug. This fixes the problem where I forgot to check for empty name.
			// While the original fix is pending https://github.com/ersh1/TrueHUD/pull/7, this workaround should be enough.
			// Note, that this also affects Compass Navigation Overhaul, which properly checks for empty name,
			// however, it uses the same actor->GetName() as fallback, so it should be fine.
			if (name.empty() && context == NameContext::kEnemyHUD && version == NND_API::InterfaceVersion::kV1) {
				return actor->GetName();
			}

			return name;
		}

		return ""sv;
	}

	void NNDInterface::RevealName(RE::ActorHandle handle) noexcept {
		if (const auto actor = handle.get().get()) {
			RevealName(actor);
		}
	}

	void NNDInterface::RevealName(RE::Actor* actor) noexcept {
		if (actor) {
			NND::Distribution::Manager::GetSingleton()->RevealName(actor);
		}
	}
}
