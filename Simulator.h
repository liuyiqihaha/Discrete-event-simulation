#pragma once

#include <vector>
#include <string>
#include <functional>
#include <queue>

#include "Event.h"

namespace DES
{
	
	using EventQueue = std::priority_queue<Event*, std::vector<Event*>, EventComparator>;
	
	class Simulator
	{
	public:
		enum SimulatorState {
			kStop = 0,
			kRunning,
			kHalted,
		};
	public:
		int global_time_;
		SimulatorState simulator_state;
		EventQueue event_queue_;
	public:
		Simulator();
		~Simulator();
		
		void Enqueue(Event* _event);
		void Dequeue();
		void Simulate();
		void Dispatch(Event* _event);
	};
}