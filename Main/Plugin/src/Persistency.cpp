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
				                    details::Read(interface, data.isTitleless) &&
				                    details::Read(interface, data.isObscuringTitle) &&
				                    details::Read(interface, data.hasDefaultObscurity) &&
				                    details::Read(interface, data.hasDefaultTitle) &&
				                    details::Read(interface, data.updateMask);

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
				       details::Write(interface, data.isTitleless) &&
				       details::Write(interface, data.isObscuringTitle) &&
				       details::Write(interface, data.hasDefaultObscurity) &&
				       details::Write(interface, data.hasDefaultTitle) &&
				       details::Write(interface, data.updateMask);
			}
		}

		namespace Snapshot
		{
			constexpr std::uint32_t recordType = 'CRC';

			bool Save(SKSE::SerializationInterface* interface) {
				if (!interface->OpenRecord(recordType, serializationVersion)) {
					return false;
				}
				logger::info("Saving options...");
				if (!details::Write(interface, Options::DisplayName::format) ||
				    !details::Write(interface, Options::Obscurity::defaultName))
					return false;

				const auto snapshot = MakeSnapshot();
				if (!snapshot.empty()) {
					logger::info("Saving {} snapshots:", snapshot.size());

					if (!details::Write(interface, snapshot.size()))
						return false;
					
					for (const auto& entry : snapshot) {
						logger::info("\t{}", entry);
						if (!details::Write(interface, entry))
							return false;
					}
				}
				return true;
			}

			bool Load(SKSE::SerializationInterface* interface, Distribution::NNDData::UpdateMask& mask) {
				logger::info("Loading options...");

				std::string oldFormat;
				std::string oldObscurity;

				if (!details::Read(interface, oldFormat) ||
				    !details::Read(interface, oldObscurity))
					return false;

				if (oldFormat != Options::DisplayName::format)
					mask |= Distribution::NNDData::UpdateMask::kDisplayName;
				if (oldObscurity != Options::Obscurity::defaultName)
					mask |= Distribution::NNDData::UpdateMask::kObscureName;

				std::size_t snapshotSize;
				if (!details::Read(interface, snapshotSize))
					return false;
				if (snapshotSize == 0)
					return true;

				NND::Snapshot oldSnapshot{};
				const auto    currentSnapshot = MakeSnapshot();

				logger::info("Loading {} snapshots:", snapshotSize);
				for (int i = 0; i < snapshotSize; ++i) {
					std::string entry;
					if (!details::Read(interface, entry))
						return false;
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
					mask |= Distribution::NNDData::UpdateMask::kDefinitions;
				}

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

			std::uint32_t loadedCount = 0;
			Distribution::Manager::GetSingleton()->UpdateNames([&interface, &loadedCount](auto& names) {
				std::uint32_t type, version, length;
				names.clear();
				auto mask = Distribution::NNDData::UpdateMask::kNone;
				while (interface->GetNextRecordInfo(type, version, length)) {
					if (type == Snapshot::recordType) {
						Snapshot::Load(interface, mask);
						logger::info("Loading names...");
					} else if (type == Data::recordType) {
						Distribution::NNDData data{};
						if (Data::Load(interface, data)) {
							data.updateMask |= mask;
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
