#include <iostream>

#include "Router.h"
#include "NPU.h"

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
		for (auto& i : this->buffer_count_)
		{
			i = 0;
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
		for (auto& i : this->buffer_count_)
		{
			i = 0;
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
			std::vector<DES::Event*> new_events = this->RoutingAct(_event);
			if (!new_events.empty())
			{
				vec_ret.insert(vec_ret.end(), new_events.begin(), new_events.end());
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
		case DES::Event::kNpuSendAct:
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

	DES::Event* Router::ReceiveAct(DES::Event* _event)
	{
		//获取当前时间
		int current_time = _event->handle_time_;

		//复制事件，以后的项目我再写裸指针我就是狗
		std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
		std::shared_ptr<Message> copy_message = std::make_shared<Message>(*message);
		DES::Event* copy_event = new DES::Event();
		(*copy_event) = (*_event);
		copy_event->supporting_information_ = std::static_pointer_cast<void>(copy_message);

		//事件入队
		this->input_event_buffer_[message->last_hop_].push(copy_event);
		//this->buffer_count_[message->last_hop_]++;

		//触发routing事件，让routing检索输入buffer
		DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + this->receive_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
		std::shared_ptr<Direction> direction = std::make_shared<Direction>();
		*direction = message->last_hop_;
		new_event->supporting_information_ = std::static_pointer_cast<void>(direction);

		std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") recieve act now at: " << current_time << ", buffer_" << *direction << " exact buffer is " << this->input_event_buffer_[*direction].size() << std::endl;
		return new_event;
	}

	std::vector<DES::Event*> Router::RoutingAct(DES::Event* pass_event)
	{
		std::vector<DES::Event*> vec_ret;
		
		//获取当前时间
		int current_time = pass_event->handle_time_;

		//获取触发routing的事件
		std::shared_ptr<Direction> last_hop = std::static_pointer_cast<Direction>(pass_event->supporting_information_);
		
		// TODO: 这样加是不是对的啊...
		if (this->input_event_buffer_[*last_hop].empty())
			return vec_ret;

		DES::Event* _event = this->input_event_buffer_[*last_hop].front();
		
		//获取事件附带信息
		std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);

		if (message->message_type_ == Message::kUnicast)
		{
			Direction next_hop_direction = this->Routing(message->destination_[0]);
			if (next_hop_direction == kNpu) //todo npu logic
			{
				int npu_total_buffer_count_;
				int* reserved_count_ = nullptr;
				switch (host_npu_->type_)
				{
				case NPU::NpuType::WeiAct:
				{
					npu_total_buffer_count_ = 2 * this->host_npu_->input_activate_buffer_max_;
					reserved_count_ = &this->host_npu_->reserved_buffer_count_;
					break;
				}
				case NPU::NpuType::StoreSend:
				{
					npu_total_buffer_count_ = dynamic_cast<NPUStoreSend*>(this->host_npu_)->input_token_buffer_cur_max_ * this->host_npu_->input_activate_buffer_max_;
					reserved_count_ = &this->host_npu_->reserved_buffer_count_;
					break;
				}
				case NPU::NpuType::ActAct:
				{
					NPUActAct* temp_host = dynamic_cast<NPUActAct*>(this->host_npu_);
					if (temp_host->pair_weight_collect_over_)
					{
						npu_total_buffer_count_ = 2 * this->host_npu_->input_activate_buffer_max_;
						reserved_count_ = &this->host_npu_->reserved_buffer_count_;
					}
					else
					{
						npu_total_buffer_count_ = temp_host->weight_token_buffer_cur_max_ * this->host_npu_->input_activate_buffer_max_;
						reserved_count_ = &temp_host->reserved_weight_buffer_count_;
					}
					break;
				}
				}
				if (*reserved_count_ < npu_total_buffer_count_)
				{
					DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->routing_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
					std::shared_ptr<Message> new_message = std::make_shared<Message>(*message);
					new_message->src_npu_type_ = message->src_npu_type_;
					new_message->next_hop_ = next_hop_direction;  // TODO: 是不是只要设置next_hop_就可以了
					new_event->supporting_information_ = std::static_pointer_cast<void>(new_message);
					vec_ret.emplace_back(new_event);
					if (this->host_npu_->type_ == NPU::NpuType::ActAct && dynamic_cast<NPUActAct*>(this->host_npu_)->pair_weight_collect_over_ == false)
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing weight is mine at time " << current_time << std::endl;
					else
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing act is mine at time " << current_time << std::endl;
					std::cout << *reserved_count_ << " " << npu_total_buffer_count_ << std::endl;
					this->input_event_buffer_[*last_hop].pop();
					this->update_buffer_count_(*last_hop, -1);
					// this->buffer_count_[*last_hop] -= 1; 
					delete _event;
					// NPU的reserve buffer信息在此就要更新
					*reserved_count_ += 1;

					//触发钩子事件
					this->HookRoutingAct(current_time, *last_hop, vec_ret);
				}
				else  // 如果npu没有空闲的buffer  TODO: 改成跟router一样的逻辑
				{
					// 如果传进来的是作为weight
					if (host_npu_->type_ == NPU::NpuType::ActAct && message->src_npu_type_ == NPU::NpuType::WeiAct)
					{
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing weight is mine and stop" << std::endl;
						dynamic_cast<NPUActAct*>(this->host_npu_)->weight_wait_queue_.push(last_hop);
					}
					else
					{
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing act is mine and stop" << std::endl;
						this->host_npu_->wait_queue_.push(last_hop);
					}
				}
				return vec_ret;
			}
			if (this->AdjcentReady(next_hop_direction))
			{
				//生成发送事件，并更新附加message信息
				DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->routing_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
				std::shared_ptr<Message> new_message = std::make_shared<Message>(*message);
				new_message->next_hop_ = next_hop_direction;
				new_message->last_hop_ = (Direction)(3 - next_hop_direction);
				new_event->supporting_information_ = std::static_pointer_cast<void>(new_message);

				//接收方准备好了，从输入buffer弹出并删除
				this->input_event_buffer_[*last_hop].pop();
				delete _event;
				this->buffer_count_[*last_hop]--;

				std::cout << "router routing act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;

				this->adjcent_router_[next_hop_direction]->update_buffer_count_(3 - next_hop_direction, 1);

				vec_ret.push_back(new_event);

				//触发钩子事件
				this->HookRoutingAct(current_time, *last_hop, vec_ret);
				return vec_ret;
			}
			else
			{
				std::cout << "router (" << this->adjcent_router_[next_hop_direction]->coordinate_.first << ", " << this->adjcent_router_[next_hop_direction]->coordinate_.second << ") buffer is full so router(" << this->coordinate_.first << ", " << this->coordinate_.second << ") need to wait" << std::endl;
				//this->input_event_buffer_[message->last_hop].push(_event);
				this->buffered_output_direction[next_hop_direction].push(last_hop);
				return vec_ret;
			}
		}
		else //todo 广播
		{
			
			// 解析广播信息
			std::vector<std::shared_ptr<Message>> vec_messages;
			bool send_npu_blocked = this->BroadcastRouting(message->destination_, vec_messages, message, pass_event, vec_ret);

			// 用于存储新的目的地，用于处理部分阻塞
			std::vector<std::pair<int, int>> vec_new_destinations;

			// 用于存储未就绪的下一条方向，用于处理部分阻塞
			std::vector<Direction> vec_next_hop_unready;

			// 遍历广播信息
			for (auto& ele : vec_messages)
			{
				if (this->AdjcentReady(ele->next_hop_))
				{
					DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->routing_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
					new_event->supporting_information_ = std::static_pointer_cast<void>(ele);
					std::cout << "router BROADCAST routing act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
					vec_ret.push_back(new_event);
					this->adjcent_router_[ele->next_hop_]->update_buffer_count_(3 - ele->next_hop_, 1);
				}
				else
				{
					//如果阻塞了，需要将阻塞的目的地添加到新的“原事件”
					vec_new_destinations.insert(vec_new_destinations.end(), ele->destination_.begin(), ele->destination_.end());

					//记录哪个方向被阻塞了
					vec_next_hop_unready.push_back(ele->next_hop_);
				}
			}

			// 广播事件是否处理完全
			if (vec_new_destinations.empty())
			{
				if (!send_npu_blocked)
				{
					//处理完全了，从输入buffer弹出并删除
					this->input_event_buffer_[*last_hop].pop();
					delete _event;
					this->buffer_count_[*last_hop]--;
					this->HookRoutingAct(current_time, *last_hop, vec_ret);
				}
				else  // 只有传递到当前npu的事件被阻塞了
				{
					// 更新事件目的地等信息
					message->message_type_ = Message::kUnicast;
					message->destination_ = std::vector<std::pair<int, int>>();
					message->destination_.emplace_back(std::pair<int, int>({ this->coordinate_.first, this->coordinate_.second }));
					std::cout << "发生了" << message->destination_[0].first << " " << message->destination_[0].second << " " << this->coordinate_.first << " " << this->coordinate_.second << " " << message->destination_.size() << std::endl;
					_event->supporting_information_ = std::static_pointer_cast<void>(message);
				}
			}
			else
			{
				std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing BROADCAST not completely" << std::endl;
				//没处理完全，更新目的地
				if (send_npu_blocked)
				{
			vec_new_destinations.emplace_back(std::pair<int, int>({ this->coordinate_.first, this->coordinate_.second }));
				}
					
				message->destination_ = vec_new_destinations;
				_event->supporting_information_ = std::static_pointer_cast<void>(message);

				//将所有被阻塞的方向，都加入暂存事件的发送方向vec
				for (auto& ele : vec_next_hop_unready)
				{
					this->buffered_output_direction[ele].push(last_hop);
				}
			}
			return vec_ret;
		}
	}

	DES::Event* Router::SendAct(DES::Event* _event)
	{
		//获取当前时间
		int current_time = _event->handle_time_;

		//获取附加信息中，下一条的方向
		std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
		Direction next_hop_direction = message->next_hop_;

		if (next_hop_direction == kNpu) //todo npu logic
		{
			DES::Event* new_event = new DES::Event(DES::Event::kNpuReceiveAct, current_time + this->send_delay_, std::bind(&LLM::NPU::Excute, this->host_npu_, std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(std::make_shared<Message>(*message));
			std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") send act to its npu now at: " << current_time << std::endl;
			return new_event;
		}
		else
		{
			//生成下一跳的接受事件
			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&Router::Excute, this->adjcent_router_[next_hop_direction], std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(std::make_shared<Message>(*message));
			std::cout << "router send act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
			return new_event;
		}
	}

	void Router::HookRoutingAct(int current_time, Direction last_hop, std::vector<DES::Event*>& ret_vec)
	{
		//todo npu
		if (last_hop == kNpu)
		{
			if (this->host_npu_->wait_for_send_ == true)
			{
				this->host_npu_->wait_for_send_ = false;
				std::cout << "npu (" << this->coordinate_.first << ", " << this->coordinate_.second << ") Hooked now at: " << current_time << std::endl;
				DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time, std::bind(&NPU::Excute, this->host_npu_, std::placeholders::_1));
				ret_vec.emplace_back(new_event);
			}
		}
		else
		{
			//panyudong method实现，每个输出方向都有一个fifo用来存阻塞事件的来源方向
			if (this->adjcent_router_[last_hop] != nullptr)
			{
				if (!adjcent_router_[last_hop]->buffered_output_direction[3 - last_hop].empty())
				{
					std::shared_ptr<Direction> direction = adjcent_router_[last_hop]->buffered_output_direction[3 - last_hop].front();
					adjcent_router_[last_hop]->buffered_output_direction[3 - last_hop].pop();
					std::cout << "Hooked now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
					if (!adjcent_router_[last_hop]->input_event_buffer_[*direction].empty())
					{
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time, std::bind(&Router::Excute, this->adjcent_router_[last_hop], std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(direction);
						ret_vec.emplace_back(new_event);
					}
				}
			}
			
			/*if (this->adjcent_router_[last_hop] != nullptr)
			{
				for (int i = 3; i >= 0; i--)
				{
					std::shared_ptr<Direction> direction = std::make_shared<Direction>();
					*direction = (Direction)(i);

					if (this->AdjcentDirectionReady(this->adjcent_router_[last_hop], *direction))
					{
						std::cout << "Hooked now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time, std::bind(&Router::Excute, this->adjcent_router_[last_hop], std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(direction);
						ret_vec.push_back(new_event);
					}
				}
			}*/
		}
		return;
	}

	bool Router::BroadcastRouting(std::vector<std::pair<int, int>>& vec_destinations, std::vector<std::shared_ptr<Message>>& vec_messages, std::shared_ptr<Message>& initial_message, DES::Event* pass_event, std::vector<DES::Event*>& new_events_)
	{
		bool send_npu_blocked = false;
		bool has_npu = false; //本地需要处理
		std::vector<std::pair<int, int>> vec_up_destinations; //向上广播的包，下同
		std::vector<std::pair<int, int>> vec_down_destinations;
		std::vector<std::pair<int, int>> vec_right_destinations;

		for (auto& ele : vec_destinations)
		{
			if (this->coordinate_.second == ele.second && this->coordinate_.first > ele.first)
			{
				//向上广播
				vec_up_destinations.push_back(ele);
			}
			else if (this->coordinate_.second == ele.second && this->coordinate_.first < ele.first)
			{
				//向下广播
				vec_down_destinations.push_back(ele);
			}
			else if (this->coordinate_.second < ele.second)
			{
				//向右广播
				vec_right_destinations.push_back(ele);
			}
			else if (!has_npu && this->coordinate_.first == ele.first && this->coordinate_.second == ele.second)
			{
				has_npu = true;
				std::cout << "BROADCAST act is mine" << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;

				//获取当前时间
				int current_time = pass_event->handle_time_;
				//获取触发routing的事件
				std::shared_ptr<Direction> last_hop = std::static_pointer_cast<Direction>(pass_event->supporting_information_);

				// TODO：这里要根据NPU的不同，有不同的处理逻辑
				int npu_total_buffer_count_;
				int* reserved_count_ = nullptr;
				switch (host_npu_->type_)
				{
				case NPU::NpuType::WeiAct:
				{
					npu_total_buffer_count_ = 2 * this->host_npu_->input_activate_buffer_max_;
					reserved_count_ = &this->host_npu_->reserved_buffer_count_;
					break;
				}
				case NPU::NpuType::StoreSend:
				{
					npu_total_buffer_count_ = dynamic_cast<NPUStoreSend*>(this->host_npu_)->input_token_buffer_cur_max_ * this->host_npu_->input_activate_buffer_max_;
					reserved_count_ = &this->host_npu_->reserved_buffer_count_;
					break;
				}
				case NPU::NpuType::ActAct:
				{
					NPUActAct* temp_host = dynamic_cast<NPUActAct*>(this->host_npu_);
					if (temp_host->pair_weight_collect_over_)
					{
						npu_total_buffer_count_ = 2 * this->host_npu_->input_activate_buffer_max_;
						reserved_count_ = &this->host_npu_->reserved_buffer_count_;
					}
					else
					{
						npu_total_buffer_count_ = temp_host->weight_token_buffer_cur_max_ * this->host_npu_->input_activate_buffer_max_;
						reserved_count_ = &temp_host->reserved_weight_buffer_count_;
					}
					break;
				}
				}

				if (this->coordinate_.first == 1 && this->coordinate_.second == 6)
				{
					std::cout << "router 16 " << host_npu_->type_ << " " << initial_message->src_npu_type_ << std::endl;
					// std::cout << *reserved_count_ << " " << npu_total_buffer_count_ << std::endl;
				}

				// 如果npu有预留空间
				if (*reserved_count_ < npu_total_buffer_count_)
				{
					DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->routing_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
					std::shared_ptr<Message> new_message = std::make_shared<Message>(*initial_message);
					new_message->src_npu_type_ = initial_message->src_npu_type_;
					new_message->next_hop_ = kNpu;  // TODO: 是不是只要设置next_hop_就可以了
					new_event->supporting_information_ = std::static_pointer_cast<void>(new_message);
					new_events_.emplace_back(new_event);
					if (this->host_npu_->type_ == NPU::NpuType::ActAct && dynamic_cast<NPUActAct*>(this->host_npu_)->pair_weight_collect_over_ == false)
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing weight is mine at time " << current_time << std::endl;
					else
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing act is mine at time " << current_time << std::endl;
					std::cout << *reserved_count_ << " " << npu_total_buffer_count_ << std::endl;
					// this->input_event_buffer_[*last_hop].pop();
					// this->update_buffer_count_(*last_hop, -1);
					// NPU的reserve buffer信息在此就要更新
					*reserved_count_ += 1;

					//触发钩子事件
					// this->HookRoutingAct(current_time, *last_hop, new_events_);
				}
				else  // 如果npu没有空闲的buffer  TODO: 改成跟router一样的逻辑
				{
					// 如果传进来的是作为weight
					if (host_npu_->type_ == NPU::NpuType::ActAct && initial_message->src_npu_type_ == NPU::NpuType::WeiAct)
					{
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing weight is mine and stop" << std::endl;
						dynamic_cast<NPUActAct*>(this->host_npu_)->weight_wait_queue_.push(last_hop);
					}
					else
					{
						std::cout << "router (" << this->coordinate_.first << ", " << this->coordinate_.second << ") routing act is mine and stop" << std::endl;
						this->host_npu_->wait_queue_.push(last_hop);
					}
					send_npu_blocked = true;
				}
			}
		}

		if (!vec_right_destinations.empty())
		{
			// 生成向右发送事件，下同
			std::shared_ptr<Message> new_message = std::make_shared<Message>();
			//if (vec_right_destinations.size() == 1)
			//{
			//	new_message->message_type_ = Message::kUnicast;
			//}
			//else
			//{
			new_message->message_type_ = Message::kBroadcast;
			//}
			new_message->destination_ = vec_right_destinations;
			new_message->last_hop_ = kLeft;
			new_message->next_hop_ = kRight;
			vec_messages.push_back(new_message);
		}
		if (!vec_up_destinations.empty())
		{
			std::shared_ptr<Message> new_message = std::make_shared<Message>();
			if (vec_up_destinations.size() == 1)
			{
				new_message->message_type_ = Message::kUnicast;
			}
			else
			{
				new_message->message_type_ = Message::kBroadcast;
			}
			new_message->destination_ = vec_up_destinations;
			new_message->last_hop_ = kDown;
			new_message->next_hop_ = kUp;
			vec_messages.push_back(new_message);
		}
		if (!vec_down_destinations.empty())
		{
			std::shared_ptr<Message> new_message = std::make_shared<Message>();
			if (vec_down_destinations.size() == 1)
			{
				new_message->message_type_ = Message::kUnicast;
			}
			else
			{
				new_message->message_type_ = Message::kBroadcast;
			}
			new_message->destination_ = vec_down_destinations;
			new_message->last_hop_ = kUp;
			new_message->next_hop_ = kDown;
			vec_messages.push_back(new_message);
		}
		return send_npu_blocked;
	}

	Message::Message(const Message& obj)
	{
		this->message_type_ = obj.message_type_;
		this->src_npu_type_ = obj.src_npu_type_;
		this->destination_ = obj.destination_;
		this->last_hop_ = obj.last_hop_;
		this->next_hop_ = obj.next_hop_;
	}
}



//DES::Event* Router::RoutingAct(DES::Event* pass_event)
	//{
	//	//获取当前时间
	//	int current_time = pass_event->handle_time_;

	//	//获取触发routing的事件
	//	std::shared_ptr<Direction> last_hop = std::static_pointer_cast<Direction>(pass_event->supporting_information_);
	//	DES::Event* _event = this->input_event_buffer_[*last_hop].front();

	//	//获取事件附带信息
	//	std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);

	//	if (message->message_type_ == Message::kUnicast)
	//	{
	//		//获得下一跳方向
	//		Direction next_hop_direction = this->Routing(message->destination_[0]);

	//		if (next_hop_direction == kNpu) //todo npu logic
	//		{
	//			std::cout << "act is mine" << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
	//			return nullptr;
	//		}

	//		//如果对应输出buffer有空位
	//		if (this->OutputBufferReady(next_hop_direction))
	//		{
	//			//更新信息
	//			message->next_hop_ = next_hop_direction;
	//			message->last_hop_ = (Direction)(3 - next_hop_direction);
	//			
	//			//输出buffer准备好了，从输入buffer弹出，压入输出buffer
	//			this->input_event_buffer_[*last_hop].pop();
	//			this->output_event_buffer_[next_hop_direction].push(_event);

	//			
	//			//触发send事件，让send检索输出buffer
	//			DES::Event* new_event = new DES::Event(DES::Event::kRouterSendAct, current_time + this->routing_delay_, std::bind(&Router::Excute, this, std::placeholders::_1));
	//			std::shared_ptr<Direction> direction = std::make_shared<Direction>();
	//			*direction = message->next_hop_;
	//			new_event->supporting_information_ = std::static_pointer_cast<void>(direction);

	//			std::cout << "router routing act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
	//			return new_event;
	//		}
	//		//如果没有空位
	//		else
	//		{
	//			//this->input_event_buffer_[message->last_hop].push(_event);
	//			return nullptr;
	//		}
	//	}
	//	else //todo 广播
	//	{
	//		return nullptr;
	//	}
	//}

//DES::Event* Router::SendAct(DES::Event* pass_event)
	//{
	//	//获取当前时间
	//	int current_time = pass_event->handle_time_;

	//	//获取触发send的事件
	//	std::shared_ptr<Direction> next_hop_direction = std::static_pointer_cast<Direction>(pass_event->supporting_information_);
	//	DES::Event* _event = this->output_event_buffer_[*next_hop_direction].front();


	//	if (*next_hop_direction == kNpu) //todo npu logic
	//	{
	//		return nullptr;
	//	}
	//	else
	//	{
	//		if (this->AdjcentReady(*next_hop_direction))
	//		{
	//			//获取事件附带信息
	//			std::shared_ptr<Message> message = std::static_pointer_cast<Message>(_event->supporting_information_);
	//			
	//			//触发目标router的receive事件
	//			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&Router::Excute, this->adjcent_router_[*next_hop_direction], std::placeholders::_1));
	//			new_event->supporting_information_ = std::static_pointer_cast<void>(std::make_shared<Message>(*message));
	//			
	//			//从输出buffer弹出, 并删除多余信息
	//			this->output_event_buffer_[*next_hop_direction].pop();
	//			delete _event;

	//			std::cout << "router send act now at: " << current_time << ", coordinate: " << this->coordinate_.first << ", " << this->coordinate_.second << std::endl;
	//			return new_event;
	//		}
	//		else
	//		{
	//			return nullptr;
	//		}
	//	}
	//}