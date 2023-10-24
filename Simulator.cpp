#include <assert.h>

#include "Simulator.h"

namespace DES
{
	Simulator::Simulator() :
		global_time_(0),
		simulator_state(kStop){}

	Simulator::~Simulator()
	{}
	
	void Simulator::Enqueue(Event* _event)
	{
		// preserve for multithread
		assert(_event->handle_time_ >= this->global_time_);

		this->event_queue_.push(_event);
	}

	void Simulator::Dispatch(Event* _event)
	{
		std::vector<Event*> vec_event = _event->handler(_event);
		delete _event;
		for (auto ele : vec_event)
		{
			this->Enqueue(ele);
		}
	}

	void Simulator::Dequeue()
	{
		// preserve for 2 dimension event list
		Event* current_event;
		
		current_event = this->event_queue_.top();
		event_queue_.pop();
		this->global_time_ = current_event->handle_time_;
		this->Dispatch(current_event);

		while (!this->event_queue_.empty() && global_time_ == event_queue_.top()->handle_time_)
		{
			current_event = this->event_queue_.top();
			event_queue_.pop();
			this->Dispatch(current_event);
		}
	}

	void Simulator::Simulate()
	{
		// todo add multithread	
		this->simulator_state = kRunning;
		while (this->simulator_state==kRunning && !this->event_queue_.empty())
		{
			if (this->global_time_ > 5000)
			{
				this->simulator_state = kHalted;
				return;
			}
			this->Dequeue();
		}
		this->simulator_state = kStop;
	}
}