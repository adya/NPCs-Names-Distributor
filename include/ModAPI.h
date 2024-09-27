#pragma once
#include <memory>

#include "NND_API.h"

namespace Messaging
{
	using InterfaceVersion1 = ::NND_API::IVNND1;
	using NameContext = ::NND_API::NameContext;

	class NNDInterface : public InterfaceVersion1
	{
	private:
		NNDInterface() noexcept = default;
		virtual ~NNDInterface() noexcept = default;

	public:
		static NNDInterface* GetSingleton() noexcept {
			static NNDInterface singleton;
			return std::addressof(singleton);
		}

		virtual std::string_view GetName(RE::ActorHandle, NameContext) noexcept override;
		virtual std::string_view GetName(RE::Actor*, NameContext) noexcept override;
		virtual void             RevealName(RE::ActorHandle) noexcept override;
		virtual void             RevealName(RE::Actor*) noexcept override;
	};
}
