#pragma once
#include <memory>

#include "NND_API.h"

namespace Messaging
{
	using NameContext = ::NND_API::NameContext;

	class NNDInterface : public ::NND_API::IVNND3
	{
	private:
		NND_API::InterfaceVersion version{ NND_API::InterfaceVersion::kV3 };
		NNDInterface(NND_API::InterfaceVersion version) :
			version(version) {}
		virtual ~NNDInterface() noexcept = default;

	public:
		static NNDInterface* GetSingleton(NND_API::InterfaceVersion version) noexcept {
			static NNDInterface singleton(version);
			return std::addressof(singleton);
		}

		virtual std::string_view GetName(RE::ActorHandle, NameContext) noexcept override;
		virtual std::string_view GetName(RE::Actor*, NameContext) noexcept override;
		virtual void             RevealName(RE::ActorHandle) noexcept override;
		virtual void             RevealName(RE::Actor*) noexcept override;
		virtual bool             RevealName(RE::ActorHandle, NND_API::RevealReason) noexcept override;
		virtual bool             RevealName(RE::Actor*, NND_API::RevealReason) noexcept override;
	};
}
