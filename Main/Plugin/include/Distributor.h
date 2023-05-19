#pragma once
#include "NameDefinition.h"
#include <shared_mutex>

namespace NND
{
	
	namespace Distribution
	{
		enum NameFormat
		{
			/// Show a full name with a title.
			kDisplayName,

			/// Show only a name without a title.
			kFullName,

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
			bool isObscured = false;
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

			/// Reveals name for given RE::FormID if it was previously obscured.
			void RevealName(RE::FormID);

			NameRef  GetName(NameFormat, RE::Actor*, const char* originalName);
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

			const std::unique_ptr<RE::TESCondition> talkedToPC;

			void DeleteName(RE::FormID);

			// Singleton stuff :)
			Manager();
			Manager(const Manager&) = delete;
			Manager(Manager&&) = delete;

			~Manager() override = default;

			Manager& operator=(const Manager&) = delete;
			Manager& operator=(Manager&&) = delete;
		};

		
	}
}
