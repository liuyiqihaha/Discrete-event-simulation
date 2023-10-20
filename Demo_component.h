#pragma once
#include <string>
#include <functional>
#include "Event.h"

namespace DES
{
	class DemoComponent
	{
	public:
		int x_;
		int y_;

	public:
		DemoComponent(int x, int y);
		~DemoComponent();
		std::vector<Event*> excute(Event* _event);
	};
}