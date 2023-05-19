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
				                    Read(interface, data.isObscured) &&
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
				       Write(serializationInterface, data.isObscured) &&
				       Write(serializationInterface, data.isTitleless);
			}

		}

		void Setup() {
			const auto serializationInterface = SKSE::GetSerializationInterface();
			serializationInterface->SetUniqueID(serializationKey);
			serializationInterface->SetSaveCallback(Save);
			serializationInterface->SetLoadCallback(Load);
			serializationInterface->SetRevertCallback(Revert);
		}

		void Load(SKSE::SerializationInterface* interface) {
			logger::info("{:*^30}", "LOADING");
			logger::info("Loading names...");

			std::uint32_t loadedCount = 0;
			Distribution::Manager::GetSingleton()->UpdateNames([&interface, &loadedCount](auto& names) {
				std::uint32_t type, version, length;
				names.clear();
				while (interface->GetNextRecordInfo(type, version, length)) {
					if (type == recordType && version == serializationVersion) {
						Distribution::NNDData data{};
						if (details::Deserialize(interface, data)) {
#ifndef NDEBUG
							logger::info("Loaded [0x{:X}] {} ({})", data.formId, data.name, data.title);
#endif
							names[data.formId] = data;
							++loadedCount;
						}
					}
				}
			});

			logger::info("Loaded {} names", loadedCount);
		}

		void Save(SKSE::SerializationInterface* interface) {
			logger::info("{:*^30}", "SAVING");

			auto names = Distribution::Manager::GetSingleton()->GetAllNames();

			logger::info("Saving {} names...", names.size());

			std::uint32_t savedCount = 0;
			for (const auto& data : names | std::views::values) {
				if (!details::Serialize(interface, data)) {
					logger::error("Failed to save name for [0x{:X}]", data.formId);
					continue;
				}
				++savedCount;
			}

			logger::info("Saved {} names", savedCount);
		}

		void Revert(SKSE::SerializationInterface* interface) {
			logger::info("{:*^30}", "REVERTING");
			Distribution::Manager::GetSingleton()->UpdateNames([](auto& names) {
				names.clear();
			});
		}
	}
}
