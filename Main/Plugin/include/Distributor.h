#pragma once
#include "NameDefinition.h"
#include "Options.h"
#include <shared_mutex>

namespace NND
{
	namespace Distribution
	{
		struct NNDData
		{
			RE::FormID formId{};

			Name name{};
			Name title{};
			Name obscurity{};

			Name shortDisplayName{};

			Name displayName{};

			bool isUnique = false;
			bool isObscured = false;

			bool allowDefaultTitle = true;
			bool allowDefaultObscurity = true;

			/// Flag indicating that current title was picked from a Name Definition
			/// that is also used on Obscuring scope, thus obscurity should reuse that title instead of creating another one.
			bool isObscuringTitle = false;
		
			void UpdateDisplayName(const RE::Actor*);

			NameRef GetName(NameStyle, const RE::Actor*) const;

			friend class Manager;
		private:
			NameRef GetTitle(const RE::Actor*) const;
			NameRef GetObscurity(const RE::Actor*) const;
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
			bool RevealName(const RE::Actor*, bool forceGreet);

			NameRef  GetName(NameStyle, RE::Actor*);
			NNDData& SetData(const NNDData&);
			NNDData& CreateData(RE::Actor*);

			NNDData& UpdateDataFlags(NNDData&, RE::Actor*) const;
			NNDData& UpdateData(NNDData&, RE::Actor*, bool definitionsChanged) const;
			
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

			void MakeName(NNDData&, const RE::Actor*) const;
			void MakeTitle(NNDData&, const RE::Actor*) const;
			void MakeObscureName(NNDData&, const RE::Actor*) const;

			void DeleteName(RE::FormID);
			bool ActorSupportsObscurity(RE::Actor*) const;

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
