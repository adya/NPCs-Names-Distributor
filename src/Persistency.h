#pragma once

namespace NND
{
	namespace Persistency
	{
		using ActorQueue = std::unordered_map<RE::FormID, const RE::Actor*>;
		class Manager
		{
		public:
			static Manager* GetSingleton() {
				static Manager singleton;
				return &singleton;
			}

			static void Register();

			bool IsLoadingGame() const {
				return isLoadingGame;
			}

			void StartLoadingGame() {
				isLoadingGame = true;
			}

			void FinishLoadingGame() {
				isLoadingGame = false;
			}

		private:
			static void Load(SKSE::SerializationInterface*);
			static void Save(SKSE::SerializationInterface*);
			static void Revert(SKSE::SerializationInterface*);

			bool isLoadingGame = false;

			// Singleton stuff :)
			Manager() = default;
			Manager(const Manager&) = delete;
			Manager(Manager&&) = delete;

			~Manager() = default;

			Manager& operator=(const Manager&) = delete;
			Manager& operator=(Manager&&) = delete;
		};
	}
}
