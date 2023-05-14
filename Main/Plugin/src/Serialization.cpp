#include "Serialization.h"

namespace NND
{
	namespace Serialization
	{
		namespace details
		{
			template <typename T>
			bool Write(SKSE::SerializationInterface* interface, const T& data) {
				return interface->WriteRecordData(&data, sizeof(T));
			}

			template <>
			bool Write(SKSE::SerializationInterface* interface, const std::string& data) {
				const std::size_t size = data.length();
				return interface->WriteRecordData(size) && interface->WriteRecordData(data.data(), static_cast<std::uint32_t>(size));
			}

			template <typename T>
			bool Read(SKSE::SerializationInterface* interface, T& result) {
				return interface->ReadRecordData(&result, sizeof(T));
			}

			template <>
			bool Read(SKSE::SerializationInterface* interface, std::string& result) {
				std::size_t size = 0;
				if (!interface->ReadRecordData(size)) {
					return false;
				}
				if (size > 0) {
					result.resize(size);
					if (!interface->ReadRecordData(result.data(), static_cast<std::uint32_t>(size))) {
						return false;
					}
				} else {
					result = empty;
				}
				return true;
			}

			bool Deserialize(SKSE::SerializationInterface* interface, Distribution::NNDData& data) {
				const bool result = Read(interface, data.formId) &&
				                    Read(interface, data.name) &&
				                    Read(interface, data.title) &&
				                    Read(interface, data.obscurity) &&
				                    Read(interface, data.shortDisplayName) &&
				                    Read(interface, data.displayName) &&
				                    Read(interface, data.isUnique) &&
				                    Read(interface, data.isKnown) &&
				                    Read(interface, data.isTitleless);


				if (!result || !interface->ResolveFormID(data.formId, data.formId)) {
					logger::warn("Failed to load name for NPCs with FormID [0x{:X}]", data.formId);
					return false;
				}

				return true;
			}

			bool Serialize(SKSE::SerializationInterface* serializationInterface, const Distribution::NNDData& data) {
				if (!serializationInterface->OpenRecord(recordType, serializationVersion)) {
					return false;
				}

				return Write(serializationInterface, data.formId) &&
				       Write(serializationInterface, data.name) &&
				       Write(serializationInterface, data.title) &&
				       Write(serializationInterface, data.obscurity) &&
				       Write(serializationInterface, data.shortDisplayName) &&
				       Write(serializationInterface, data.displayName) &&
				       Write(serializationInterface, data.isUnique) &&
				       Write(serializationInterface, data.isKnown) &&
				       Write(serializationInterface, data.isTitleless);
			}

		}

		struct NNDData
		{
			RE::FormID formId;

			Name name{};
			Name title{};
			Name obscurity{};

			Name shortDisplayName{};

			Name displayName{};

			bool isUnique = false;
			bool isKnown = false;
			bool isTitleless = false;
		};

		void Setup() {
			const auto serializationInterface = SKSE::GetSerializationInterface();
			serializationInterface->SetUniqueID(serializationKey);
			serializationInterface->SetSaveCallback(Save);
			serializationInterface->SetLoadCallback(Load);
		}

		void Load(SKSE::SerializationInterface* a_serializationInterface) {
			logger::info("{:*^30}", "SERIALIZATION");
			logger::info("Loading names...");

			auto createdObjectManager = RE::BGSCreatedObjectManager::GetSingleton();
			RE::BSSpinLockGuard lockGuard(createdObjectManager->lock);

			std::uint32_t type, version, length;
			std::uint32_t loadedCount = 0;
			while (a_serializationInterface->GetNextRecordInfo(type, version, length)) {
				if (type == recordType) {
					Distribution::NNDData data{};
					if (details::Deserialize(a_serializationInterface, data)) {
						logger::info("Loaded [0x{:X}] {} ({})", data.formId, data.name, data.title);
						Distribution::names[data.formId] = data;
						++loadedCount;
					}
				}
			}

			logger::info("Loaded {} names", loadedCount);
		}

		void Save(SKSE::SerializationInterface* serializationInterface) {
			logger::info("{:*^30}", "SERIALIZATION");
			logger::info("Saving {} names...", Distribution::names.size());

			auto                createdObjectManager = RE::BGSCreatedObjectManager::GetSingleton();
			RE::BSSpinLockGuard lockGuard(createdObjectManager->lock);

			std::uint32_t savedCount = 0;
			for (const auto& data : Distribution::names | std::views::values) {
				if (!details::Serialize(serializationInterface, data)) {
					logger::error("Failed to save name for [0x{:X}]", data.formId);
					continue;
				}
				++savedCount;
			}

			logger::info("Saved {} names", savedCount);

		}
	}
}
