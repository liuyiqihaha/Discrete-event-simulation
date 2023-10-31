#include "Event.h"

namespace DES
{
	Event::Event() :
		event_type_(kNpuCompute),
		event_name_(""),
		handle_time_(0),
		handler_id_(0),
		handler(nullptr){}

	Event::Event(int handle_time, EventHandler handler) :
		event_type_(kNpuCompute),
		event_name_(""),
		handle_time_(handle_time),
		handler_id_(0),
		handler(handler){}

	Event::Event(EventType event_type, int handle_time, EventHandler handler) :
		event_type_(event_type),
		event_name_(""),
		handle_time_(handle_time),
		handler_id_(0),
		handler(handler) {}


	Event::~Event()
	{}

	Event& Event::operator=(const Event& _event)
	{
		this->event_type_ = _event.event_type_;
		this->event_name_ = _event.event_name_;
		this->handle_time_ = _event.handle_time_;
		this->handler_id_ = _event.handler_id_;
		this->handler = _event.handler;
		return *this;
	}

	bool EventComparator::operator()(const Event* lhs, const Event* rhs) const 
	{
		return lhs->handle_time_ > rhs->handle_time_;
	}
}