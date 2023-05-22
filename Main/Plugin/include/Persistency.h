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
				queuedActors.clear();
			}

			/// Queues Actors who's been loaded before Persistency Manager was able to Load their data.
			///
			///	All actors who are located in the current cell when game loads must be queued,
			///	so that they can be updated accordingly by Distribution Manager.
			///	The queue will be cleared when Loading is finished.
			void QueueActor(const RE::Actor* actor) {
				queuedActors[actor->formID] = actor;
			}

		private:
			static void Load(SKSE::SerializationInterface*);
			static void Save(SKSE::SerializationInterface*);
			static void Revert(SKSE::SerializationInterface*);

			bool isLoadingGame = false;

			ActorQueue queuedActors{};

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
