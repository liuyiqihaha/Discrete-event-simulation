// LLM_pipeline_simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Event.h"
#include "Demo_component.h"
#include "Simulator.h"
#include "NPU.h"
#include "Router.h"
#include "Chiplet.h"

int main()
{
    std::cout << "Hello World!\n";

	/*DES::DemoComponent* demo_component = new DES::DemoComponent(6, 17);
	DES::Simulator* simulator = new DES::Simulator();
	DES::Event* _event = new DES::Event(10, std::bind(&DES::DemoComponent::excute, demo_component, std::placeholders::_1));
	simulator->Enqueue(_event);
	simulator->Simulate();*/
	
	//DES::DemoComponent* demo_component = new DES::DemoComponent(6, 17);
	//LLM::NPU* npu = new LLM::NPUWeiAct(demo_component);
	//npu->input_activate_buffer_count_[0] = 5118;
	//npu->input_activate_buffer_count_[1] = 5118;
	//DES::Event* _event1 = new DES::Event(DES::Event::kNpuReceiveAct, 10, std::bind(&LLM::NPU::Excute, npu, std::placeholders::_1));
	//DES::Event* _event2 = new DES::Event(DES::Event::kNpuReceiveAct, 20, std::bind(&LLM::NPU::Excute, npu, std::placeholders::_1));
	//DES::Event* _event3 = new DES::Event(DES::Event::kNpuReceiveAct, 30, std::bind(&LLM::NPU::Excute, npu, std::placeholders::_1));
	//DES::Event* _event4 = new DES::Event(DES::Event::kNpuReceiveAct, 40, std::bind(&LLM::NPU::Excute, npu, std::placeholders::_1));
	//DES::Simulator* simulator = new DES::Simulator();
	//simulator->Enqueue(_event1);
	//simulator->Enqueue(_event2);
	//simulator->Enqueue(_event3);
	//simulator->Enqueue(_event4);
	//simulator->Simulate();


	//LLM::Router* router1 = new LLM::Router(0,0);
	//LLM::Router* router2 = new LLM::Router(0,1);
	//LLM::Router* router3 = new LLM::Router(1,1);
	//LLM::Router* router4 = new LLM::Router(1, 0);
	//LLM::Router* router5 = new LLM::Router(-1, 0);
	//router1->adjcent_router_[LLM::Router::kRight] = router2;
	//router2->adjcent_router_[LLM::Router::kLeft] = router1;
	//router2->adjcent_router_[LLM::Router::kDown] = router3;
	//router3->adjcent_router_[LLM::Router::kUp] = router2;

	//router1->adjcent_router_[LLM::Router::kDown] = router4;
	//router1->adjcent_router_[LLM::Router::kUp] = router5;

	//router5->adjcent_router_[LLM::Router::kDown] = router1;
	//router4->adjcent_router_[LLM::Router::kUp] = router1;

	//DES::Event* _event1 = new DES::Event(DES::Event::kRouterReceiveAct, 10, std::bind(&LLM::Router::Excute, router1, std::placeholders::_1));
	////DES::Event* _event2 = new DES::Event(DES::Event::kRouterReceiveAct, 100, std::bind(&LLM::Router::Excute, router2, std::placeholders::_1));
	////DES::Event* _event3 = new DES::Event(DES::Event::kRouterReceiveAct, 100, std::bind(&LLM::Router::Excute, router2, std::placeholders::_1));
	//std::shared_ptr<LLM::Message> new_message = std::make_shared<LLM::Message>();
	//
	//std::pair<int, int> dest1;
	//dest1.first = 1;
	//dest1.second = 1;

	//std::pair<int, int> dest2;
	//dest2.first = 0;
	//dest2.second = 1;

	//std::pair<int, int> dest3;
	//dest3.first = 1;
	//dest3.second = 0;

	//std::pair<int, int> dest4;
	//dest4.first = -1;
	//dest4.second = 0;

	//new_message->destination_.push_back(dest1);
	//new_message->destination_.push_back(dest2);
	//new_message->destination_.push_back(dest3);
	//new_message->destination_.push_back(dest4);
	//new_message->message_type_ = LLM::Message::kBroadcast;

	//_event1->supporting_information_ = std::static_pointer_cast<void>(new_message);

	//DES::Simulator* simulator = new DES::Simulator();
	//simulator->Enqueue(_event1);
	////simulator->Enqueue(_event2);
	////simulator->Enqueue(_event3);
	//simulator->Simulate();

	LLM::Chiplet chiplet(2);
	chiplet.Mapping();
	chiplet.Run();

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
