#pragma once
#include "Distributor.h"

namespace NND
{
	namespace Serialization
	{
		constexpr std::uint32_t serializationKey = 'NNDI';
		constexpr std::uint32_t recordType = 'DATA';
		constexpr std::uint32_t serializationVersion = 1;

		void Setup();

		void Load(SKSE::SerializationInterface* interface);
		void Save(SKSE::SerializationInterface* a_serializationInterface);
	}
}
