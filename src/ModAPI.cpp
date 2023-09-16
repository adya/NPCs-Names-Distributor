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

	std::string_view NNDInterface::GetName(const RE::Actor* actor, NameContext context) noexcept {
		if (actor) {
			return NND::Distribution::Manager::GetSingleton()->GetName(GetNameStyle(context), actor);
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
