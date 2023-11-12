#pragma once

#include <vector>
#include <queue>
#include <utility>
#include <memory>

#include "Event.h"
#include "NPU.h"

namespace LLM
{
	class Message;

	class Router
	{
	public:
		enum Direction {
			kRight = 0,
			kUp,
			kDown,
			kLeft,
			kNpu
		};
	
	public:
		std::pair<int, int> coordinate_; //����

		unsigned int buffer_max_; //buffer��С
		unsigned int buffer_count_[5]; //buffer�е��¼�����
		std::queue<DES::Event*> input_event_buffer_[5]; //�¼�������
		std::queue<std::shared_ptr<Direction>> buffered_output_direction[5]; //���ڻ��汻�ݴ���¼��ķ��ͷ���
		//std::queue<DES::Event*> output_event_buffer_[5]; //�¼�������

		Router* adjcent_router_[4]; //���ڵ�router
		NPU* host_npu_; //������NPU

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
		std::vector<DES::Event*> RoutingAct(DES::Event* pass_event);
		//DES::Event* SendAct(DES::Event* _event);
		DES::Event* SendAct(DES::Event* pass_event);

	private:
		void HookRoutingAct(int current_time, Direction last_hop, std::vector<DES::Event*>& ret_vec);

	public:
		/*inline bool OutputBufferReady(Direction direction)
		{
			return this->output_event_buffer_[direction].size() < this->buffer_max_;
		}*/

		inline bool AdjcentDirectionReady(Router* adj, Direction direction)
		{
			/*return !adj->input_event_buffer_[direction].empty();*/
			return adj->buffer_count_[direction] != 0;
		}

		inline bool AdjcentReady(Direction direction)
		{
			//3-direction Ϊ�˷���ȡ��������ٶ�������router buffer��С���
			/*return this->adjcent_router_[direction]->input_event_buffer_[3 - direction].size() < this->buffer_max_;*/
			return this->adjcent_router_[direction]->buffer_count_[3 - direction] < this->buffer_max_;
		}

		inline Direction Routing(std::pair<int, int> destination)
		{
			//Ŀǰ����Ʋ��������󴫵�
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

		void BroadcastRouting(std::vector<std::pair<int, int>>& vec_destinations, std::vector<std::shared_ptr<Message>>& vec_messages);
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