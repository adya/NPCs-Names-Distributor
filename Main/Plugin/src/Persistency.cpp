#include "Persistency.h"

#include "Distributor.h"
#include "LookupNameDefinitions.h"

namespace NND
{
	namespace Persistency
	{
		constexpr std::uint32_t serializationKey = 'NNDI';
		constexpr std::uint32_t serializationVersion = 1;

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
		}

		namespace Data
		{
			constexpr std::uint32_t recordType = 'DATA';

			bool Load(SKSE::SerializationInterface* interface, Distribution::NNDData& data) {
				const bool result = details::Read(interface, data.formId) &&
				                    details::Read(interface, data.name) &&
				                    details::Read(interface, data.title) &&
				                    details::Read(interface, data.obscurity) &&
				                    details::Read(interface, data.shortDisplayName) &&
				                    details::Read(interface, data.displayName) &&
				                    details::Read(interface, data.isUnique) &&
				                    details::Read(interface, data.isObscured) &&
				                    details::Read(interface, data.isTitleless);

				if (!result || !interface->ResolveFormID(data.formId, data.formId)) {
					logger::warn("Failed to load name for NPCs with FormID [0x{:X}]", data.formId);
					return false;
				}

				return true;
			}

			bool Save(SKSE::SerializationInterface* interface, const Distribution::NNDData& data) {
				if (!interface->OpenRecord(recordType, serializationVersion)) {
					return false;
				}

				return details::Write(interface, data.formId) &&
				       details::Write(interface, data.name) &&
				       details::Write(interface, data.title) &&
				       details::Write(interface, data.obscurity) &&
				       details::Write(interface, data.shortDisplayName) &&
				       details::Write(interface, data.displayName) &&
				       details::Write(interface, data.isUnique) &&
				       details::Write(interface, data.isObscured) &&
				       details::Write(interface, data.isTitleless);
			}
		}

		namespace Snapshot
		{
			constexpr std::uint32_t recordType = 'CRC';

			bool Save(SKSE::SerializationInterface* interface) {
				const auto snapshot = MakeSnapshot();

				if (snapshot.empty()) {
					return true;
				}

				logger::info("Saving {} snapshots:", snapshot.size());

				if (!interface->OpenRecord(recordType, serializationVersion)) {
					return false;
				}

				auto result = details::Write(interface, snapshot.size());
				for (const auto& entry : snapshot) {
					logger::info("\t{}", entry);
					result = details::Write(interface, entry) && result;
				}

				return result;
			}

			bool Load(SKSE::SerializationInterface* interface, bool& hasChanged) {
				NND::Snapshot oldSnapshot{};
				const auto    currentSnapshot = MakeSnapshot();

				std::size_t snapshotSize;
				auto        result = details::Read(interface, snapshotSize);
				if (!result || snapshotSize == 0) {
					return false;
				}
				logger::info("Loading {} snapshots:", snapshotSize);
				for (int i = 0; i < snapshotSize; ++i) {
					std::string entry;
					details::Read(interface, entry);
					oldSnapshot.insert(entry);
					logger::info("\t{}", entry);
				}

				NND::Snapshot diff{};
				std::ranges::set_difference(currentSnapshot, oldSnapshot, std::inserter(diff, diff.end()));

				if (!diff.empty()) {
					logger::info("Detected changes in Name Definitions:");
					for (const auto& entry : diff) {
						logger::info("\t{}", entry);
					}
					logger::info("Data will be updated.");
				}

				hasChanged = !diff.empty();

				return true;
			}
		}
	}

	// Public
	namespace Persistency
	{
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
				bool hasChanged = false;
				while (interface->GetNextRecordInfo(type, version, length)) {
					if (type == Snapshot::recordType) {
						Snapshot::Load(interface, hasChanged);
					} else if (type == Data::recordType) {
						Distribution::NNDData data{};
						if (Data::Load(interface, data)) {
							// TODO: Check if hasChanged true and data uses default values - run the update.
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
			Snapshot::Save(interface);

			auto names = Distribution::Manager::GetSingleton()->GetAllNames();

			logger::info("Saving {} names...", names.size());

			std::uint32_t savedCount = 0;
			for (const auto& data : names | std::views::values) {
				if (!Data::Save(interface, data)) {
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
			logger::info("\tNames cache has been cleared.");
		}
	}
}
