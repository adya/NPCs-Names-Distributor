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
			bool Write(SKSE::SerializationInterface* a_interface, const T& data) {
				return a_interface->WriteRecordData(&data, sizeof(T));
			}

			template <>
			bool Write(SKSE::SerializationInterface* a_interface, const std::string& data) {
				const std::size_t size = data.length();
				return a_interface->WriteRecordData(size) && a_interface->WriteRecordData(data.data(), static_cast<std::uint32_t>(size));
			}

			template <typename T>
			bool Read(SKSE::SerializationInterface* a_interface, T& result) {
				return a_interface->ReadRecordData(&result, sizeof(T));
			}

			template <>
			bool Read(SKSE::SerializationInterface* a_interface, std::string& result) {
				std::size_t size = 0;
				if (!a_interface->ReadRecordData(size)) {
					return false;
				}
				if (size > 0) {
					result.resize(size);
					if (!a_interface->ReadRecordData(result.data(), static_cast<std::uint32_t>(size))) {
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

			bool Load(SKSE::SerializationInterface* a_interface, Distribution::NNDData& data) {
				const bool result = details::Read(a_interface, data.formId) &&
				                    details::Read(a_interface, data.name) &&
				                    details::Read(a_interface, data.title) &&
				                    details::Read(a_interface, data.obscurity) &&
				                    details::Read(a_interface, data.shortDisplayName) &&
				                    details::Read(a_interface, data.displayName) &&
				                    details::Read(a_interface, data.isUnique) &&
				                    details::Read(a_interface, data.isObscured) &&
				                    details::Read(a_interface, data.allowDefaultTitle) &&
				                    details::Read(a_interface, data.allowDefaultObscurity) &&
				                    details::Read(a_interface, data.isObscuringTitle);

				if (!result || !a_interface->ResolveFormID(data.formId, data.formId)) {
					logger::warn("Failed to load name for NPCs with FormID [0x{:X}]", data.formId);
					return false;
				}

				return true;
			}

			bool Save(SKSE::SerializationInterface* a_interface, const Distribution::NNDData& data) {
				if (!a_interface->OpenRecord(recordType, serializationVersion)) {
					return false;
				}

				return details::Write(a_interface, data.formId) &&
				       details::Write(a_interface, data.name) &&
				       details::Write(a_interface, data.title) &&
				       details::Write(a_interface, data.obscurity) &&
				       details::Write(a_interface, data.shortDisplayName) &&
				       details::Write(a_interface, data.displayName) &&
				       details::Write(a_interface, data.isUnique) &&
				       details::Write(a_interface, data.isObscured) &&
				       details::Write(a_interface, data.allowDefaultTitle) &&
				       details::Write(a_interface, data.allowDefaultObscurity) &&
				       details::Write(a_interface, data.isObscuringTitle);
			}
		}

		namespace Snapshot
		{
			constexpr std::uint32_t recordType = 'CRC';

			bool Save(SKSE::SerializationInterface* a_interface) {
				if (!a_interface->OpenRecord(recordType, serializationVersion)) {
					return false;
				}

				const auto snapshot = MakeSnapshot();
				if (!snapshot.empty()) {
					logger::info("Saving {} snapshots:", snapshot.size());

					if (!details::Write(a_interface, snapshot.size()))
						return false;

					for (const auto& entry : snapshot) {
						logger::info("\t{}", entry);
						if (!details::Write(a_interface, entry))
							return false;
					}
				}
				return true;
			}

			bool Load(SKSE::SerializationInterface* a_interface, bool& definitionsChanged) {
				size_t snapshotSize;
				if (!details::Read(a_interface, snapshotSize))
					return false;
				if (snapshotSize == 0)
					return true;

				NND::Snapshot oldSnapshot{};
				const auto    currentSnapshot = MakeSnapshot();

				logger::info("Loading {} snapshots:", snapshotSize);
				for (size_t i = 0; i < snapshotSize; ++i) {
					std::string entry;
					if (!details::Read(a_interface, entry))
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
					definitionsChanged = true;
				}

				return true;
			}
		}

	}

	namespace Persistency
	{
		void Manager::Register() {
			const auto serializationInterface = SKSE::GetSerializationInterface();
			serializationInterface->SetUniqueID(serializationKey);
			serializationInterface->SetSaveCallback(Save);
			serializationInterface->SetLoadCallback(Load);
			serializationInterface->SetRevertCallback(Revert);
		}

		void Manager::Load(SKSE::SerializationInterface* a_interface) {
			logger::info("{:*^30}", "LOADING");

			const auto&   manager = Distribution::Manager::GetSingleton();
			std::uint32_t loadedCount = 0;

			manager->UpdateNames([&](auto& names) {
				std::uint32_t type, version, length;
				names.clear();
				bool definitionsChanged = false;
				while (a_interface->GetNextRecordInfo(type, version, length)) {
					if (type == Snapshot::recordType) {
						Snapshot::Load(a_interface, definitionsChanged);
						logger::info("Loading names...");
					} else if (type == Data::recordType) {
						Distribution::NNDData data{};
						if (Data::Load(a_interface, data)) {
							if (const auto actor = RE::TESForm::LookupByID(data.formId); actor->formType == RE::FormType::ActorCharacter) {
#ifndef NDEBUG
								logger::info("\tLoaded [0x{:X}] ('{}')", data.formId, data.name != empty ? data.displayName : actor->As<RE::Actor>()->GetActorBase()->GetFullName());
#endif
								manager->UpdateData(data, actor->As<RE::Actor>(), definitionsChanged);
							}
							names[data.formId] = data;
							++loadedCount;
						}
					}
				}
			});

			logger::info("Loaded {} names", loadedCount);
		}

		void Manager::Save(SKSE::SerializationInterface* a_interface) {
			logger::info("{:*^30}", "SAVING");
			Snapshot::Save(a_interface);

			auto names = Distribution::Manager::GetSingleton()->GetAllNames();

			logger::info("Saving {} names...", names.size());

			std::uint32_t savedCount = 0;
			for (const auto& data : names | std::views::values) {
				if (!Data::Save(a_interface, data)) {
					logger::error("Failed to save name for [0x{:X}]", data.formId);
					continue;
				}
#ifndef NDEBUG
				logger::info("\tSaved [0x{:X}] {} ({})", data.formId, data.name, data.title);
#endif
				++savedCount;
			}

			logger::info("Saved {} names", savedCount);
		}

		void Manager::Revert(SKSE::SerializationInterface*) {
			logger::info("{:*^30}", "REVERTING");
			Distribution::Manager::GetSingleton()->UpdateNames([](auto& names) {
				names.clear();
			});
			logger::info("\tNames cache has been cleared.");
		}
	}
}
