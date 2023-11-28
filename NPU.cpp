#include <iostream>

#include "NPU.h"

namespace LLM
{
	NPU::NPU() :
		compute_finish_time_(0),
		input_activate_buffer_max_(5120),
		which_activate_buffer_(1),
		weight_buffer_max_(5120 * 256),
		weight_buffer_height_(5120),
		weight_buffer_width_(256),
		N_P_(1),
		K_P_(32),
		M_P_(32),
		computed_time_(0),
		buffer_is_full_(false),
		wait_for_send_(false),
		send_delay_(2),
		reserved_buffer_count_(0),
		router_(nullptr)
	{
		this->time_to_compute_once_ = this->weight_buffer_height_ / this->K_P_;
		this->needed_compute_time_ = this->weight_buffer_width_ / this->M_P_;
		input_activate_buffer_count_[0] = 0;
		input_activate_buffer_count_[1] = 0;
	}

	void NPU::Mapping(std::vector<std::pair<int, int>> dest_)
	{
		if (dest_.size() == 1)
			this->message_->message_type_ = LLM::Message::kUnicast;
		else
			this->message_->message_type_ = LLM::Message::kBroadcast;
		// TODO：这样会有拷贝风险？
		this->message_->destination_ = dest_;
	}

	NPUWeiAct::NPUWeiAct() :
		NPU() {
		this->type_ = NPU::NpuType::WeiAct;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // 将会发送到Router中的kNpu对应的buffer中
	}

	NPUWeiAct::NPUWeiAct(Router* router) :
		NPU()
	{
		this->router_ = router;
		this->router_->host_npu_ = this;
		this->type_ = NPU::NpuType::WeiAct;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // 将会发送到Router中的kNpu对应的buffer中
	}

	/*NPUWeiAct::NPUWeiAct() :
		compute_finish_time_(0),
		input_activate_buffer_max_(12288),
		which_activate_buffer_(1),
		weight_buffer_max_(12288 * 128),
		N_P_(1),
		K_P_(32),
		M_P_(32)
	{
		this->time_to_compute_ = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
		input_activate_buffer_count_[0] = 0;
		input_activate_buffer_count_[1] = 0;
	}

	NPUWeiAct::NPUWeiAct(DES::DemoComponent* demo_component) :
		compute_finish_time_(0),
		input_activate_buffer_max_(12288),
		which_activate_buffer_(1),
		weight_buffer_max_(12288 * 128),
		N_P_(1),
		K_P_(32),
		M_P_(32),
		demo_component_(demo_component)
	{
		this->time_to_compute_ = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
		input_activate_buffer_count_[0] = 0;
		input_activate_buffer_count_[1] = 0;
	}*/

	NPUWeiAct::~NPUWeiAct()
	{
		// delete this->message_;
	}

	std::vector<DES::Event*> NPUWeiAct::Excute(DES::Event* _event)
	{

		std::vector<DES::Event*> vec_ret;

		switch (_event->event_type_)
		{
		case DES::Event::kNpuReceiveAct:
		{
			DES::Event* new_event = this->ReceiveAct(_event);
			if (new_event != nullptr)
			{
				vec_ret.push_back(new_event);
			}
			break;
		}
		case DES::Event::kNpuCompute:
		{
			DES::Event* new_event = this->Compute(_event);
			vec_ret.push_back(new_event);
			break;
		}
		case DES::Event::kNpuSendAct:
		{
			std::vector<DES::Event*> new_events = this->SendAct(_event);
			if (new_events.size() > 0)
			{
				vec_ret.insert(vec_ret.end(), new_events.begin(), new_events.end());
			}
			break;
		}
		default:
		{
			std::cout << "Unknown NPUWeiAct event type!!!" << std::endl;
			exit(1);
		}
		}

		return vec_ret;
	}

	// DES::Event* NPUWeiAct::ReceiveAct(DES::Event* _event)
	// {
	// 	// 能传递过来数据，说明NPU的buffer一定未满
	// 	this->input_activate_buffer_count_[1 - this->which_activate_buffer_]++;
	// 	int current_time = _event->handle_time_;
	// 	if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] < input_activate_buffer_max_)
	// 	{
	// 		std::cout << "npu(" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at " << (1 - this->which_activate_buffer_) << " buffer at time " << current_time << std::endl;
	// 		return nullptr;
	// 	}
	// 	else
	// 	{
	// 		// 用于预取的buffer已经收集满
	// 		if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0) // 如果用于计算的buffer此时是空
	// 		{
	// 			this->which_activate_buffer_ = 1 - this->which_activate_buffer_;  // 更新用于计算的buffer index
	// 			//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
	// 			DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
	// 			std::cout << "npu(" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << " and can start compute" << std::endl;
	// 			return new_event;
	// 		}
	// 		else  // 此时用于计算的buffer正在计算
	// 		{
	// 			this->buffer_is_full_ = true;  // 所有buffer已满
	// 			//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
	// 			std::cout << "npu(" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time << " but has to wait" << std::endl;
	// 			return nullptr;
	// 		}
	// 	}
	// }

	DES::Event* NPUWeiAct::ReceiveAct(DES::Event* _event)
	{
		this->input_activate_buffer_count_[1 - this->which_activate_buffer_]++;
		if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] < input_activate_buffer_max_)
		{
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at " << (1 - this->which_activate_buffer_) << " buffer at time " << _event->handle_time_ << std::endl;
			return nullptr;
		}
		else
		{
			int current_time = _event->handle_time_;
			if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0) // 如果用于计算的buffer此时是空
			{
				//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at " << (1 - this->which_activate_buffer_) << " buffer at time " << current_time << std::endl;
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << " and can start compute" << std::endl;
				// 正在计算的buffer index变为store buffer
				this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
				return new_event;
			}
			// TODO：在router中处理kNpuReceiveAct事件的逻辑中增加对NPUbuffer全满的支持
			else
			{
				this->buffer_is_full_ = true;  // 所有buffer已满
				//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at " << (1 - this->which_activate_buffer_) << " buffer at time " << current_time << std::endl;
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time << " but has to wait" << std::endl;
				return nullptr;
			}
		}
	}

	DES::Event* NPUWeiAct::Compute(DES::Event* _event)
	{
		// this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
		int current_time = _event->handle_time_;
		std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")start compute now at: " << current_time << ", time need to compute: " << this->time_to_compute_once_ << std::endl;
		DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time + this->time_to_compute_once_, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
		return new_event;
	}

	std::vector<DES::Event*> NPUWeiAct::SendAct(DES::Event* _event)
	{
		// 一个sendAct事件结束，应该产生什么新事件
		std::vector<DES::Event*> new_events;

		if (this->router_->buffer_count_[Router::Direction::kNpu] >= this->router_->buffer_max_)
		{
			this->wait_for_send_ = true;
			return new_events;
		}
		else
		{
			// NPU中的router一定可以接收该数据
			this->input_activate_buffer_count_[this->which_activate_buffer_] -= 1;  // 计算buffer减少数据
			int current_time = _event->handle_time_;
			// TODO：增加一些目的地址等信息
			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&LLM::Router::Excute, this->router_, std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(this->message_);
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")act sent now at: " << current_time << " to direction " << this->message_->last_hop_ << std::endl;
			this->router_->update_buffer_count_(Router::Direction::kNpu, 1);  // 别忘了更新！！！
			new_events.emplace_back(new_event);

			if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0)  // 传输了一个token（行）的激活数据，不需要再继续计算了
			{
				// reserve和真正使用的buffer空间减少
				this->reserved_buffer_count_ -= this->input_activate_buffer_max_;

				// 如果用于预取的buffer已经满了，那么可以开启计算
				if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] == this->input_activate_buffer_max_)
				{
					// 增加一些初始化的支持，可以开启用于预取的buffer
					this->buffer_is_full_ = false;
					this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
					DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
					new_events.emplace_back(new_event);
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") have recieved all input and start compute at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << std::endl;
					// 查看wait_queue_中是否有阻塞的数据
					int wake_sum = std::min((int)this->wait_queue_.size(), this->input_activate_buffer_max_);
					for (int t = 0; t < wake_sum; ++t)
						// if (this->wait_queue_.size() > 0)
					{
						std::shared_ptr<Router::Direction> last_hop = this->wait_queue_.front();
						this->wait_queue_.pop();
						// 开启一个该方向的RouterRoutingAct事件
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing to npu buffer at time" << current_time + t << std::endl;
					}
				}
				return new_events;
			}
			else  // 还有需要计算的数据
			{
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
				new_events.emplace_back(new_event);
				return new_events;
			}
		}
	}
	NPUStoreSend::NPUStoreSend() :
		NPU(),
		input_token_buffer_max_(3),
		input_token_buffer_cur_max_(3),
		input_token_buffer_count_(0),
		pair_act_collect_over_(false),
		destination_npu_(nullptr)
	{
		this->type_ = NPU::NpuType::StoreSend;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // 将会发送到Router中的kNpu对应的buffer中
	}

	NPUStoreSend::NPUStoreSend(Router* router) :
		NPU(),
		input_token_buffer_max_(3),
		input_token_buffer_cur_max_(3),
		input_token_buffer_count_(0),
		pair_act_collect_over_(false),
		destination_npu_(nullptr)
	{
		this->router_ = router;
		this->router_->host_npu_ = this;
		this->type_ = NPU::NpuType::StoreSend;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // 将会发送到Router中的kNpu对应的buffer中
	}

	NPUStoreSend::~NPUStoreSend()
	{
		// delete this->message_;
	}

	void NPUStoreSend::Mapping(LLM::NPUActAct* destination_npu)
	{
		this->destination_npu_ = destination_npu;
		this->destination_npu_->source_npu_ = this;
		// this->Mapping(this->destination_npu_->router_->coordinate_.first, this->destination_npu_->router_->coordinate_.second);
	}

	std::vector<DES::Event*> NPUStoreSend::Excute(DES::Event* _event)
	{
		std::vector<DES::Event*> vec_ret;

		switch (_event->event_type_)
		{
		case DES::Event::kNpuReceiveAct:
		{
			DES::Event* new_event = this->ReceiveAct(_event);
			if (new_event != nullptr)
			{
				vec_ret.emplace_back(new_event);
			}
			break;
		}
		case DES::Event::kNpuSendAct:
		{
			std::vector<DES::Event*> new_events = this->SendAct(_event);
			if (new_events.size() > 0)
			{
				vec_ret.insert(vec_ret.end(), new_events.begin(), new_events.end());
			}
			break;
		}

		default:
		{
			std::cout << "Unknown NPUActAct event type!!!" << std::endl;
			exit(1);
		}
		}
		return vec_ret;
	}

	DES::Event* NPUStoreSend::ReceiveAct(DES::Event* _event)
	{
		// 如果收集完毕并还未计算完毕，那么也不会有新数据进入
		if (this->pair_act_collect_over_ == false)
		{
			// 该NPU只需要使用一个buffer即可，不需要双缓冲，或者把缓冲最大值变大
			int store_buffer_index_ = 1 - this->which_activate_buffer_;
			this->input_activate_buffer_count_[store_buffer_index_] += 1;
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " in " << this->input_token_buffer_count_ << " token at time" << _event->handle_time_ << std::endl;
			if (this->input_activate_buffer_count_[store_buffer_index_] == this->input_activate_buffer_max_)  // 积累了一个token（行）的激活数据
			{
				this->input_activate_buffer_count_[store_buffer_index_] = 0;
				this->input_token_buffer_count_ += 1;  // 积累了一个token（行）的激活数据
				if (this->input_token_buffer_count_ == this->input_token_buffer_cur_max_)  // 如果已经收集齐所有token
				{
					// TODO：注意这里，为了在sendAct事件中从满buffer开始减1
					this->input_activate_buffer_count_[store_buffer_index_] = this->input_activate_buffer_max_;
					// TODO：增加指示激活均收集完的表示，在这里，一个NPU只会将激活发送给另一个NPU，一对一
					this->pair_act_collect_over_ = true;
					// 收集齐所有token的激活数据后，可以尝试发送第一个token的数据
					// TODO：由于其之后是激活与激活相乘，因此合适发送激活的第一个token需要考虑：收集齐即发送或等权重收集齐再发送
					if (this->destination_npu_->pair_weight_collect_over_)
					{
						int current_time = _event->handle_time_;
						// TODO：为NPU增加一个传输到router的时延
						DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time, std::bind(&NPUStoreSend::Excute, this, std::placeholders::_1));
						return new_event;
					}
				}
			}
		}
		// 其他情况不会产生新event
		return nullptr;
	}

	std::vector<DES::Event*> NPUStoreSend::SendAct(DES::Event* _event)
	{
		// 一个sendAct事件结束，应该产生什么新事件
		std::vector<DES::Event*> new_events;

		if (this->router_->buffer_count_[Router::Direction::kNpu] >= this->router_->buffer_max_)
		{
			this->wait_for_send_ = true;
			return new_events;
		}
		else
		{
			// NPU中的router一定可以接收该数据
			int store_buffer_index_ = 1 - this->which_activate_buffer_;
			this->input_activate_buffer_count_[store_buffer_index_] -= 1;
			int current_time = _event->handle_time_;

			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&LLM::Router::Excute, this->router_, std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(this->message_);
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") sent act at: " << this->input_activate_buffer_count_[store_buffer_index_] << " in " << this->input_token_buffer_count_ - 1 << " token at time " << _event->handle_time_ << std::endl;
			std::cout << "src_npu_type: " << this->type_ << " " << std::static_pointer_cast<Message>(new_event->supporting_information_)->src_npu_type_ << std::endl;
			// router的buffer[kNpu]将确定接收到数据
			this->router_->update_buffer_count_(Router::Direction::kNpu, 1);
			new_events.emplace_back(new_event);

			if (this->input_activate_buffer_count_[store_buffer_index_] == 0)  // 传输了一个token（行）的激活数据
			{
				// reserve和真正使用的buffer空间减少
				this->input_token_buffer_count_ -= 1;
				if (this->input_token_buffer_count_ > 0)
				{
					// 准备传输下一token（行）
					this->input_activate_buffer_count_[store_buffer_index_] = this->input_activate_buffer_max_;
				}
				else
				{
					// TODO：这里先注释掉，在计算完之后统一更新
					// this->pair_act_collect_over_ = false;  // 为下一次收集做准备
					// this->destination_npu_->pair_weight_collect_over_ = false;  // 变为false，等待下一次积累权重
					// this->destination_npu_->weight_token_buffer_count_ = 0;  // 也更新为0
					// 如果全部激活已经传输完毕
					return new_events;
				}
			}
			// 还有激活需要传输
			new_event = new DES::Event(DES::Event::kNpuSendAct, current_time + this->send_delay_, std::bind(&NPUStoreSend::Excute, this, std::placeholders::_1));
			new_events.emplace_back(new_event);
			return new_events;
		}
	}


	NPUActAct::NPUActAct() :
		NPU(),
		weight_token_buffer_max_(3),
		weight_token_buffer_cur_max_(3),
		weight_token_buffer_count_(0),
		pair_weight_collect_over_(false),
		source_npu_(nullptr),
		reserved_weight_buffer_count_(0)
	{
		this->type_ = NPU::NpuType::ActAct;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // 将会发送到Router中的kNpu对应的buffer中
	}

	NPUActAct::NPUActAct(Router* router) :
		NPU(),
		weight_token_buffer_max_(3),
		weight_token_buffer_cur_max_(3),
		weight_token_buffer_count_(0),
		pair_weight_collect_over_(false),
		source_npu_(nullptr),
		reserved_weight_buffer_count_(0)
	{
		this->router_ = router;
		this->router_->host_npu_ = this;
		this->type_ = NPU::NpuType::ActAct;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // 将会发送到Router中的kNpu对应的buffer中
	}

	NPUActAct::~NPUActAct()
	{
		// delete this->message_;
	}

	std::vector<DES::Event*> NPUActAct::Excute(DES::Event* _event)
	{
		std::vector<DES::Event*> vec_ret;

		switch (_event->event_type_)
		{
		case DES::Event::kNpuReceiveWeight:
		{
			DES::Event* new_event = this->ReceiveWeight(_event);
			if (new_event != nullptr)
			{
				vec_ret.emplace_back(new_event);
			}
			break;
		}
		case DES::Event::kNpuReceiveAct:
		{
			DES::Event* new_event = this->ReceiveAct(_event);
			if (new_event != nullptr)
			{
				vec_ret.emplace_back(new_event);
			}
			break;
		}
		case DES::Event::kNpuCompute:
		{
			DES::Event* new_event = this->Compute(_event);
			if (new_event != nullptr)
			{
				vec_ret.emplace_back(new_event);
			}
			break;
		}
		case DES::Event::kNpuSendAct:
		{
			std::vector<DES::Event*> new_events = this->SendAct(_event);
			if (new_events.size() > 0)
			{
				vec_ret.insert(vec_ret.end(), new_events.begin(), new_events.end());
			}
			break;
		}
		default:
		{
			std::cout << "Unknown NPUActAct event type!!!" << std::endl;
			exit(1);
		}
		}
		return vec_ret;
	}

	// TODO：在router中增加对weight的适配
	DES::Event* NPUActAct::ReceiveWeight(DES::Event* _event)
	{
		// 如果收集完毕并还未计算完毕，那么也不会有新数据进入，说明此时在进行收集weight的工作
		if (this->pair_weight_collect_over_ == false)
		{
			// 说明在_event->handle_time_，this已经接收到数据
			int store_buffer_index_ = 1 - this->which_activate_buffer_;
			this->input_activate_buffer_count_[store_buffer_index_] += 1;
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")recieved weight: " << this->input_activate_buffer_count_[store_buffer_index_] << " in " << this->weight_token_buffer_count_ << " token at time " << _event->handle_time_ << std::endl;
			if (this->input_activate_buffer_count_[store_buffer_index_] == this->input_activate_buffer_max_)  // 积累了一个token（行）的权重数据
			{
				this->input_activate_buffer_count_[store_buffer_index_] = 0;
				// TODO：在所有计算均完毕后，要将其设置为0
				this->weight_token_buffer_count_ += 1;  // 积累了一个token（行）的激活数据
				if (this->weight_token_buffer_count_ == this->weight_token_buffer_cur_max_)  // 如果已经收集齐所有token
				{
					// 用于指示收集weight，已经可以结束
					// this->reserved_buffer_count_ = 0;
					// TODO：增加指示激活均收集完的表示，在这里，一个NPU只会将激活发送给另一个NPU，一对一
					this->pair_weight_collect_over_ = true;
					// 收集齐所有token的权重数据后，可以尝试接收第一个token的数据
					// TODO：由于其之后是激活与激活相乘，因此合适开始接受第一个激活token需要考虑：收集齐或等激活收集齐再发送
					if (this->source_npu_->pair_act_collect_over_)
					{
						int current_time = _event->handle_time_;
						// TODO：这里是否需要为current_time加一个时延？
						DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time, std::bind(&NPUStoreSend::Excute, this->source_npu_, std::placeholders::_1));
						return new_event;
					}
				}
			}
		}
		// 其他情况不会产生新event
		return nullptr;
	}

	// 能够receive Act，就代表该NPU上的乒乓buffer里还有空位
	// TODO：在router上添加支持，即发现NPU乒乓buffer都用了，那么就不能再发送数据了
	DES::Event* NPUActAct::ReceiveAct(DES::Event* _event)
	{
		// 如果收集完毕并还未计算完毕，那么也不会有新数据进入，说明此时在进行收集weight的工作
		if (this->pair_weight_collect_over_ == false)
		{
			DES::Event* new_event = this->ReceiveWeight(_event);
			return new_event;
		}
		// 进行矩阵乘中收集激活数据
		else
		{
			// TODO：在其router中增加对buffer_is_full_的支持，而不应在这里提供支持，记得检查
			int store_buffer_index_ = 1 - this->which_activate_buffer_;  // 乒乓buffer中用于存储的index
			this->input_activate_buffer_count_[store_buffer_index_]++;
			if (this->input_activate_buffer_count_[store_buffer_index_] < input_activate_buffer_max_)
			{
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " at " << store_buffer_index_ << " buffer at time " << _event->handle_time_ << std::endl;
				return nullptr;
			}
			else
			{
				int current_time = _event->handle_time_;
				if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0) // 如果用于计算的buffer此时是空 
				{
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " at " << store_buffer_index_ << " buffer at time " << current_time << std::endl;
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << " and can start compute" << std::endl;
					// 正在计算的buffer index变为store buffer
					this->which_activate_buffer_ = store_buffer_index_;
					DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
					return new_event;
				}
				// 说明此时用于计算的buffer正在计算，而用于预取的buffer也收集完毕，此时乒乓buffer已没有空间
				// TODO：在router中处理kNpuReceiveAct事件的逻辑中增加对NPUbuffer全满的支持
				else
				{
					this->buffer_is_full_ = true;
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " at " << store_buffer_index_ << " buffer at time " << current_time << std::endl;
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time << " but has to wait" << std::endl;
					return nullptr;
				}
			}
		}
	}

	// TODO：compute事件可以被压缩删除，但是sendAct将负责唤醒compute，好像不能被删除
	DES::Event* NPUActAct::Compute(DES::Event* _event)
	{
		// this->input_activate_buffer_count_[this->which_activate_buffer_] = 0;
		int current_time = _event->handle_time_;
		std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")start compute now at: " << current_time << ", time need to compute: " << this->time_to_compute_once_ << std::endl;
		DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time + this->time_to_compute_once_, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
		return new_event;
	}

	std::vector<DES::Event*> NPUActAct::SendAct(DES::Event* _event)
	{
		// 一个sendAct事件结束，应该产生什么新事件
		std::vector<DES::Event*> new_events;

		if (this->router_->buffer_count_[Router::Direction::kNpu] >= this->router_->buffer_max_)
		{
			this->wait_for_send_ = true;
			return new_events;
		}
		else
		{
			// NPU中的router一定可以接收该数据
			this->input_activate_buffer_count_[this->which_activate_buffer_] -= 1;  // 计算buffer减少数据
			int current_time = _event->handle_time_;
			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&LLM::Router::Excute, this->router_, std::placeholders::_1));
			// TODO: 注意这里要将last_hop设置为kNpu
			new_event->supporting_information_ = std::static_pointer_cast<void>(this->message_);
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")act sent now at: " << current_time << std::endl;
			this->router_->update_buffer_count_(Router::Direction::kNpu, 1);  // 更新router中的buffer占用量
			new_events.emplace_back(new_event);

			std::cout << "temp     " << this->input_activate_buffer_count_[this->which_activate_buffer_] << std::endl;
			if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0)  // 传输了一个token（行）的激活数据，不需要再继续计算了
			{
				// reserve和真正使用的buffer空间减少
				this->reserved_buffer_count_ -= this->input_activate_buffer_max_;
				// TODO：这里是不是正确的？
				this->weight_token_buffer_count_ -= 1;
				std::cout << "tempp     " << this->weight_token_buffer_count_ << std::endl;
				// 说明已经不需要计算矩阵乘了
				if (this->weight_token_buffer_count_ == 0)
				{
					// TODO：检查这里 直接变成0
					this->reserved_weight_buffer_count_ = 0;
					this->pair_weight_collect_over_ = false; // 为下一次收集做准备
					int wake_sum = std::min((int)this->weight_wait_queue_.size(), this->input_activate_buffer_max_ * this->weight_token_buffer_cur_max_);
					std::cout << "npu (1, 6) wake_sum is " << wake_sum << std::endl;
					for (int t = 0; t < wake_sum; ++t)
					{
						std::shared_ptr<Router::Direction> last_hop = this->weight_wait_queue_.front();
						this->weight_wait_queue_.pop();
						// 开启一个该方向的RouterRoutingAct事件
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing weight to npu buffer at time" << current_time + t << std::endl;
					}

					this->source_npu_->reserved_buffer_count_ = 0;
					this->source_npu_->pair_act_collect_over_ = false;  // 为下一次收集做准备
					wake_sum = std::min((int)this->source_npu_->wait_queue_.size(), this->source_npu_->input_activate_buffer_max_ * this->source_npu_->input_token_buffer_cur_max_);
					std::cout << "npu (0, 5) wake_sum is " << wake_sum << std::endl;
					for (int t = 0; t < wake_sum; ++t)
					{
						std::shared_ptr<Router::Direction> last_hop = this->source_npu_->wait_queue_.front();
						this->source_npu_->wait_queue_.pop();
						// 开启一个该方向的RouterRoutingAct事件
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->source_npu_->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->source_npu_->router_->coordinate_.first << ", " << this->source_npu_->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing input to npu buffer at time" << current_time + t << std::endl;
					}
				}

				// TODO：这里需不需要加一个else？
				// 如果用于预取的buffer已经满了
				if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] == this->input_activate_buffer_max_)
				{
					// 增加一些初始化的支持，可以开启用于预取的buffer
					this->buffer_is_full_ = false;
					this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
					DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
					new_events.emplace_back(new_event);
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") have recieved all input and start compute at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << std::endl;
					// TODO：查看wait_queue_中是否有阻塞的数据，这里是否应该在其他情况下也检测
					int wake_sum = std::min((int)this->wait_queue_.size(), this->input_activate_buffer_max_);
					for (int t = 0; t < wake_sum; ++t)
					{
						std::shared_ptr<Router::Direction> last_hop = this->wait_queue_.front();
						this->wait_queue_.pop();
						// 开启一个该方向的RouterRoutingAct事件
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing act to npu buffer at time" << current_time + t << std::endl;
					}
				}
				return new_events;
			}
			else  // 还有需要计算的数据
			{
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
				new_events.emplace_back(new_event);
				return new_events;
			}
		}
	}

}