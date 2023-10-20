#pragma once
#include <string>
#include <functional>
#include <vector>

namespace DES
{
	class Event;
	
	using EventHandler = std::function<std::vector<Event*>(Event*)>;
	
	class Event
	{
	public:
		enum EventType {
			kNpuCompute = 0,
		};

	public:
		EventType event_type_;
		std::string event_name_;
		int handle_time_;
		int handler_id_;

	public:
		Event();
		Event(int handle_time, EventHandler handler);
		~Event();
		Event& operator=(const Event& _event);

		EventHandler handler;
	};

	class EventComparator
	{
	public:
		bool operator()(const Event* lhs, const Event* rhs) const;
	};
}
