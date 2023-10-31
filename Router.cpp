#include <iostream>

#include "Router.h"

namespace LLM
{
	Router::Router() :
		buffer_max_(2),
		receive_delay_(2),
		routing_delay_(2),
		send_delay_(2),
		inter_chip_append_(5)
	{
		this->coordinate_.first = 0;
		this->coordinate_.second = 0;
		for (auto & i : this->adjcent_router_)
		{
			i = nullptr;
		}
		this->host_npu_ = nullptr;
	}

	Router::Router(int x, int y) :
		buffer_max_(2),
		receive_delay_(2),
		routing_delay_(2),
		send_delay_(2),
		inter_chip_append_(5)
	{
		this->coordinate_.first = x;
		this->coordinate_.second = y;
		for (auto& i : this->adjcent_router_)
		{
			i = nullptr;
		}
		this->host_npu_ = nullptr;
	}

	Router::~Router()
	{}

	std::vector<DES::Event*> Router::Excute(DES::Event* _event)
	{
		std::vector<DES::Event*> vec_ret;

		switch (_event->event_type_)
		{
		case DES::Event::kRouterReceiveAct:
		{
			DES::Event* new_event = this->ReceiveAct(_event);
			if (new_event != nullptr)
			{
				vec_ret.push_back(new_event);
			}
			break;
		}
		case DES::Event::kRouterRoutingAct:
		{
			DES::Event* new_event = this->RoutingAct(_event);
			if (new_event != nullptr)
			{
				vec_ret.push_back(new_event);
			}
			break;
		}
		case DES::Event::kRouterSendAct:
		{
			DES::Event* new_event = this->SendAct(_event);
			if (new_event != nullptr)
			{
				vec_ret.push_back(new_event);
			}
			break;
		}
		case DES::Event::KNpuSendAct:
		{
			break;
		}
		default:
		{
			std::cout << "Unknown Router event type!!!" << std::endl;
			exit(1);
		}
		}

		return vec_ret;
	}

	//DES::Event* Router::ReceiveAct(DES::Event* _event)
	//{
	//	int current_time = _event->handle_time_;
	//	std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
	//	//Message* message = (Message*)(_event->supporting_information);
	//	if (message->message_type_ == Message::kUnicast)
	//	{
	//		Direction next_hop_direction = this->Routing(message->destination_[0]);
	//		if (next_hop_direction == kNpu) //todo ¹ã²¥
	//		{
	//			return nullptr;
	//		}
	//		if (this->AdjcentReady(next_hop_direction))
	//		{
	//			DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->receive_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
	//			std::shared_ptr<Message> new_message = std::make_shared<Message>(*message);
	//			new_message->next_hop = next_hop_direction;
	//			new_message->last_hop = (Direction)(3 - next_hop_direction);
	//			new_event->supporting_information_ = std::static_pointer_cast<void>(new_message);
	//			std::cout << "router recieve act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
	//			return new_event;
	//		}
	//		else
	//		{
	//			this->event_buffer_[message->last_hop].push(_event);
	//			return nullptr;
	//		}
	//	}
	//	else //todo ¹ã²¥
	//	{
	//		return nullptr;
	//	}
	//}

	DES::Event* Router::ReceiveAct(DES::Event* _event)
	{

		int current_time = _event->handle_time_;
		std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
		std::shared_ptr<Message> copy_message = std::make_shared<Message>(*message);
		DES::Event* copy_event = new DES::Event();
		(*copy_event) = (*_event);
		copy_event->supporting_information_ = std::static_pointer_cast<void>(copy_message);
		this->event_buffer_[message->last_hop_].push(copy_event);

		DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + this->receive_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
		std::shared_ptr<Direction> direction = std::make_shared<Direction>();
		*direction = message->last_hop_;
		new_event->supporting_information_ = std::static_pointer_cast<void>(direction);

		std::cout << "router recieve act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
		return new_event;
	}

	DES::Event* Router::RoutingAct(DES::Event* pass_event)
	{
		int current_time = pass_event->handle_time_;
		std::shared_ptr<Direction> last_hop = std::static_pointer_cast<Direction>(pass_event->supporting_information_);
		DES::Event* _event = this->event_buffer_[*last_hop].front();
		
		std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
		//Message* message = (Message*)(_event->supporting_information);
		if (message->message_type_ == Message::kUnicast)
		{
			Direction next_hop_direction = this->Routing(message->destination_[0]);
			if (next_hop_direction == kNpu) //todo npu logic
			{
				std::cout << "act is mine" << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
				return nullptr;
			}
			if (this->AdjcentReady(next_hop_direction))
			{
				this->event_buffer_[*last_hop].pop();
				DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->routing_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
				std::shared_ptr<Message> new_message = std::make_shared<Message>(*message);
				new_message->next_hop_ = next_hop_direction;
				new_message->last_hop_ = (Direction)(3 - next_hop_direction);
				new_event->supporting_information_ = std::static_pointer_cast<void>(new_message);
				std::cout << "router routing act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
				return new_event;
			}
			else
			{
				//this->event_buffer_[message->last_hop].push(_event);
				return nullptr;
			}
		}
		else //todo ¹ã²¥
		{
			return nullptr;
		}
	}

	DES::Event* Router::SendAct(DES::Event* _event)
	{
		int current_time = _event->handle_time_;
		std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
		Direction next_hop_direction = message->next_hop_;
		if (next_hop_direction == kNpu) //todo npu logic
		{
			return nullptr;
		}
		else
		{
			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&Router::Excute, this->adjcent_router_[next_hop_direction], std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(std::make_shared<Message>(*message));
			std::cout << "router send act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;

			return new_event;
		}
	}


	Message::Message(const Message& obj)
	{
		this->message_type_ = obj.message_type_;
		this->destination_ = obj.destination_;
		this->last_hop_ = obj.last_hop_;
		this->next_hop_ = obj.next_hop_;
	}
}