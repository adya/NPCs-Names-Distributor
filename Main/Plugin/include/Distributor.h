#pragma once
#include "NameDefinition.h"
#include <shared_mutex>

namespace NND
{
	
	namespace Distribution
	{
		enum NameFormat
		{
			/// Show full name with title.
			kFullName,

			/// Show only name without a title.
			kName,

			/// Show short name if available.
			kShortName
		};

		struct NNDData
		{
			RE::FormID formId {};

			Name name{};
			Name title{};
			Name obscurity{};

			Name shortDisplayName{};

			Name displayName{};

			bool isUnique = false;
			bool isKnown = false;
			bool isTitleless = false;

			void UpdateDisplayName();

			NameRef GetName(NameFormat) const;
		};

		class Manager : public RE::BSTEventSink<RE::TESFormDeleteEvent>
		{
		public:
			static Manager* GetSingleton() {
				static Manager singleton;
				return &singleton;
			}
			
			static void Register();

			using NamesMap = std::unordered_map<RE::FormID, NNDData>;

			NameRef  GetName(NameFormat, const RE::TESNPC*, const char* originalName);
			NNDData& SetName(const NNDData&);

			void            UpdateNames(std::function<void(NamesMap&)>);
			const NamesMap& GetAllNames();

		protected:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent*, RE::BSTEventSource<RE::TESFormDeleteEvent>*) override;

		private:

			using Lock = std::shared_mutex;
			using ReadLocker = std::shared_lock<Lock>;
			using WriteLocker = std::unique_lock<Lock>;

			mutable Lock _lock;
			NamesMap     names{};

			void DeleteName(RE::FormID);

			// Singleton stuff :)
			Manager() = default;
			Manager(const Manager&) = delete;
			Manager(Manager&&) = delete;

			~Manager() override = default;

			Manager& operator=(const Manager&) = delete;
			Manager& operator=(Manager&&) = delete;

			
		};

		
	}
}
