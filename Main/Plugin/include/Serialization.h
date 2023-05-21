#pragma once

namespace NND
{
	namespace Serialization
	{
		void Setup();

		void Load(SKSE::SerializationInterface*);
		void Save(SKSE::SerializationInterface*);
		void Revert(SKSE::SerializationInterface*);
	}
}
