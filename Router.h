#pragma once

#include <vector>
#include <queue>
#include <utility>
#include <memory>

#include "Event.h"
#include "NPU.h"

namespace LLM
{

	class Router
	{
	public:
		enum Direction {
			kUp = 0,
			kLeft,
			kRight,
			kDown,
			kNpu
		};
	
	public:
		std::pair<int, int> coordinate_; //坐标

		int buffer_max_; //buffer大小
		std::queue<DES::Event*> event_buffer_[5]; //事件缓冲区

		Router* adjcent_router_[4]; //相邻的router
		NPU* host_npu_; //相连的NPU

		int receive_delay_; //receive
		int routing_delay_;
		int send_delay_; //send+wire
		int inter_chip_append_; //

	public:
		Router();
		Router(int x, int y);
		~Router();

	public:
		std::vector<DES::Event*> Excute(DES::Event* _event);
		DES::Event* ReceiveAct(DES::Event* _event);
		DES::Event* RoutingAct(DES::Event* _event);
		DES::Event* SendAct(DES::Event* _event);

	public:
		inline bool AdjcentReady(Direction direction)
		{
			//3-direction 为了方向取反，这里假定了所有router buffer大小相等
			return this->adjcent_router_[direction]->event_buffer_[3 - direction].size() != this->buffer_max_;
		}

		inline Direction Routing(std::pair<int, int> destination)
		{
			//目前的设计不存在向左传递
			if (this->coordinate_.second < destination.second)
			{
				return kRight;
			}
			else if (this->coordinate_.first < destination.first)
			{
				return kDown;
			}
			else if (this->coordinate_.first > destination.first)
			{
				return kUp;
			}
			else
			{
				return kNpu;
			}
		}
	};

	class Message
	{
	public:
		enum MessageType {
			kUnicast = 0,
			kBroadcast
		};

	public:
		MessageType message_type_;
		std::vector<std::pair<int, int>> destination_;
		//std::pair<int, int> source_;
		Router::Direction last_hop_;
		Router::Direction next_hop_;

	public:
		Message()
		{
			message_type_ = kUnicast;
			this->last_hop_ = Router::kLeft;
		}
		~Message()
		{}
		Message(const Message& obj);
	};
}