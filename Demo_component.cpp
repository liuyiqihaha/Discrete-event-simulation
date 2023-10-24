#include <iostream>
#include "Demo_component.h"

namespace DES
{
	DemoComponent::DemoComponent(int x, int y) :
		x_(x),
		y_(y){}

	DemoComponent::~DemoComponent()
	{}

	std::vector<Event*> DemoComponent::Excute(Event* _event)
	{
		int current_time = _event->handle_time_;
		std::cout << "current_time: " << current_time << "\tx: " << this->x_++ << "\ty: " << this->y_++ << std::endl;
		std::vector<Event*> vec_ret;
		Event* new_event = new Event(current_time + 1000, std::bind(&DemoComponent::Excute, this, std::placeholders::_1));
		//Event* new_event = new Event();
		vec_ret.push_back(new_event);
		return vec_ret;
	}
}