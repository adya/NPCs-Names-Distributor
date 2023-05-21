#pragma once
#include "NameDefinition.h"
#include <shared_mutex>
#include "Options.h"

namespace NND
{
	
	namespace Distribution
	{
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

			NameRef GetName(NameStyle) const;
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

			NameRef  GetName(NameStyle, RE::Actor*, const char* originalName);
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
