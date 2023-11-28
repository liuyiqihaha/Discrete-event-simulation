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
		// TODO���������п������գ�
		this->message_->destination_ = dest_;
	}

	NPUWeiAct::NPUWeiAct() :
		NPU() {
		this->type_ = NPU::NpuType::WeiAct;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // ���ᷢ�͵�Router�е�kNpu��Ӧ��buffer��
	}

	NPUWeiAct::NPUWeiAct(Router* router) :
		NPU()
	{
		this->router_ = router;
		this->router_->host_npu_ = this;
		this->type_ = NPU::NpuType::WeiAct;
		this->message_ = std::make_shared<LLM::Message>();
		this->message_->src_npu_type_ = this->type_;
		this->message_->last_hop_ = Router::Direction::kNpu;  // ���ᷢ�͵�Router�е�kNpu��Ӧ��buffer��
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
	// 	// �ܴ��ݹ������ݣ�˵��NPU��bufferһ��δ��
	// 	this->input_activate_buffer_count_[1 - this->which_activate_buffer_]++;
	// 	int current_time = _event->handle_time_;
	// 	if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] < input_activate_buffer_max_)
	// 	{
	// 		std::cout << "npu(" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at " << (1 - this->which_activate_buffer_) << " buffer at time " << current_time << std::endl;
	// 		return nullptr;
	// 	}
	// 	else
	// 	{
	// 		// ����Ԥȡ��buffer�Ѿ��ռ���
	// 		if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0) // ������ڼ����buffer��ʱ�ǿ�
	// 		{
	// 			this->which_activate_buffer_ = 1 - this->which_activate_buffer_;  // �������ڼ����buffer index
	// 			//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
	// 			DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
	// 			std::cout << "npu(" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << " and can start compute" << std::endl;
	// 			return new_event;
	// 		}
	// 		else  // ��ʱ���ڼ����buffer���ڼ���
	// 		{
	// 			this->buffer_is_full_ = true;  // ����buffer����
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
			if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0) // ������ڼ����buffer��ʱ�ǿ�
			{
				//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at " << (1 - this->which_activate_buffer_) << " buffer at time " << current_time << std::endl;
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << " and can start compute" << std::endl;
				// ���ڼ����buffer index��Ϊstore buffer
				this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
				return new_event;
			}
			// TODO����router�д���kNpuReceiveAct�¼����߼������Ӷ�NPUbufferȫ����֧��
			else
			{
				this->buffer_is_full_ = true;  // ����buffer����
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
		// һ��sendAct�¼�������Ӧ�ò���ʲô���¼�
		std::vector<DES::Event*> new_events;

		if (this->router_->buffer_count_[Router::Direction::kNpu] >= this->router_->buffer_max_)
		{
			this->wait_for_send_ = true;
			return new_events;
		}
		else
		{
			// NPU�е�routerһ�����Խ��ո�����
			this->input_activate_buffer_count_[this->which_activate_buffer_] -= 1;  // ����buffer��������
			int current_time = _event->handle_time_;
			// TODO������һЩĿ�ĵ�ַ����Ϣ
			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&LLM::Router::Excute, this->router_, std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(this->message_);
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")act sent now at: " << current_time << " to direction " << this->message_->last_hop_ << std::endl;
			this->router_->update_buffer_count_(Router::Direction::kNpu, 1);  // �����˸��£�����
			new_events.emplace_back(new_event);

			if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0)  // ������һ��token���У��ļ������ݣ�����Ҫ�ټ���������
			{
				// reserve������ʹ�õ�buffer�ռ����
				this->reserved_buffer_count_ -= this->input_activate_buffer_max_;

				// �������Ԥȡ��buffer�Ѿ����ˣ���ô���Կ�������
				if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] == this->input_activate_buffer_max_)
				{
					// ����һЩ��ʼ����֧�֣����Կ�������Ԥȡ��buffer
					this->buffer_is_full_ = false;
					this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
					DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
					new_events.emplace_back(new_event);
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") have recieved all input and start compute at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << std::endl;
					// �鿴wait_queue_���Ƿ�������������
					int wake_sum = std::min((int)this->wait_queue_.size(), this->input_activate_buffer_max_);
					for (int t = 0; t < wake_sum; ++t)
						// if (this->wait_queue_.size() > 0)
					{
						std::shared_ptr<Router::Direction> last_hop = this->wait_queue_.front();
						this->wait_queue_.pop();
						// ����һ���÷����RouterRoutingAct�¼�
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing to npu buffer at time" << current_time + t << std::endl;
					}
				}
				return new_events;
			}
			else  // ������Ҫ���������
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
		this->message_->last_hop_ = Router::Direction::kNpu;  // ���ᷢ�͵�Router�е�kNpu��Ӧ��buffer��
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
		this->message_->last_hop_ = Router::Direction::kNpu;  // ���ᷢ�͵�Router�е�kNpu��Ӧ��buffer��
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
		// ����ռ���ϲ���δ������ϣ���ôҲ�����������ݽ���
		if (this->pair_act_collect_over_ == false)
		{
			// ��NPUֻ��Ҫʹ��һ��buffer���ɣ�����Ҫ˫���壬���߰ѻ������ֵ���
			int store_buffer_index_ = 1 - this->which_activate_buffer_;
			this->input_activate_buffer_count_[store_buffer_index_] += 1;
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " in " << this->input_token_buffer_count_ << " token at time" << _event->handle_time_ << std::endl;
			if (this->input_activate_buffer_count_[store_buffer_index_] == this->input_activate_buffer_max_)  // ������һ��token���У��ļ�������
			{
				this->input_activate_buffer_count_[store_buffer_index_] = 0;
				this->input_token_buffer_count_ += 1;  // ������һ��token���У��ļ�������
				if (this->input_token_buffer_count_ == this->input_token_buffer_cur_max_)  // ����Ѿ��ռ�������token
				{
					// TODO��ע�����Ϊ����sendAct�¼��д���buffer��ʼ��1
					this->input_activate_buffer_count_[store_buffer_index_] = this->input_activate_buffer_max_;
					// TODO������ָʾ������ռ���ı�ʾ�������һ��NPUֻ�Ὣ����͸���һ��NPU��һ��һ
					this->pair_act_collect_over_ = true;
					// �ռ�������token�ļ������ݺ󣬿��Գ��Է��͵�һ��token������
					// TODO��������֮���Ǽ����뼤����ˣ���˺��ʷ��ͼ���ĵ�һ��token��Ҫ���ǣ��ռ��뼴���ͻ��Ȩ���ռ����ٷ���
					if (this->destination_npu_->pair_weight_collect_over_)
					{
						int current_time = _event->handle_time_;
						// TODO��ΪNPU����һ�����䵽router��ʱ��
						DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time, std::bind(&NPUStoreSend::Excute, this, std::placeholders::_1));
						return new_event;
					}
				}
			}
		}
		// ����������������event
		return nullptr;
	}

	std::vector<DES::Event*> NPUStoreSend::SendAct(DES::Event* _event)
	{
		// һ��sendAct�¼�������Ӧ�ò���ʲô���¼�
		std::vector<DES::Event*> new_events;

		if (this->router_->buffer_count_[Router::Direction::kNpu] >= this->router_->buffer_max_)
		{
			this->wait_for_send_ = true;
			return new_events;
		}
		else
		{
			// NPU�е�routerһ�����Խ��ո�����
			int store_buffer_index_ = 1 - this->which_activate_buffer_;
			this->input_activate_buffer_count_[store_buffer_index_] -= 1;
			int current_time = _event->handle_time_;

			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&LLM::Router::Excute, this->router_, std::placeholders::_1));
			new_event->supporting_information_ = std::static_pointer_cast<void>(this->message_);
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") sent act at: " << this->input_activate_buffer_count_[store_buffer_index_] << " in " << this->input_token_buffer_count_ - 1 << " token at time " << _event->handle_time_ << std::endl;
			std::cout << "src_npu_type: " << this->type_ << " " << std::static_pointer_cast<Message>(new_event->supporting_information_)->src_npu_type_ << std::endl;
			// router��buffer[kNpu]��ȷ�����յ�����
			this->router_->update_buffer_count_(Router::Direction::kNpu, 1);
			new_events.emplace_back(new_event);

			if (this->input_activate_buffer_count_[store_buffer_index_] == 0)  // ������һ��token���У��ļ�������
			{
				// reserve������ʹ�õ�buffer�ռ����
				this->input_token_buffer_count_ -= 1;
				if (this->input_token_buffer_count_ > 0)
				{
					// ׼��������һtoken���У�
					this->input_activate_buffer_count_[store_buffer_index_] = this->input_activate_buffer_max_;
				}
				else
				{
					// TODO��������ע�͵����ڼ�����֮��ͳһ����
					// this->pair_act_collect_over_ = false;  // Ϊ��һ���ռ���׼��
					// this->destination_npu_->pair_weight_collect_over_ = false;  // ��Ϊfalse���ȴ���һ�λ���Ȩ��
					// this->destination_npu_->weight_token_buffer_count_ = 0;  // Ҳ����Ϊ0
					// ���ȫ�������Ѿ��������
					return new_events;
				}
			}
			// ���м�����Ҫ����
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
		this->message_->last_hop_ = Router::Direction::kNpu;  // ���ᷢ�͵�Router�е�kNpu��Ӧ��buffer��
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
		this->message_->last_hop_ = Router::Direction::kNpu;  // ���ᷢ�͵�Router�е�kNpu��Ӧ��buffer��
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

	// TODO����router�����Ӷ�weight������
	DES::Event* NPUActAct::ReceiveWeight(DES::Event* _event)
	{
		// ����ռ���ϲ���δ������ϣ���ôҲ�����������ݽ��룬˵����ʱ�ڽ����ռ�weight�Ĺ���
		if (this->pair_weight_collect_over_ == false)
		{
			// ˵����_event->handle_time_��this�Ѿ����յ�����
			int store_buffer_index_ = 1 - this->which_activate_buffer_;
			this->input_activate_buffer_count_[store_buffer_index_] += 1;
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")recieved weight: " << this->input_activate_buffer_count_[store_buffer_index_] << " in " << this->weight_token_buffer_count_ << " token at time " << _event->handle_time_ << std::endl;
			if (this->input_activate_buffer_count_[store_buffer_index_] == this->input_activate_buffer_max_)  // ������һ��token���У���Ȩ������
			{
				this->input_activate_buffer_count_[store_buffer_index_] = 0;
				// TODO�������м������Ϻ�Ҫ��������Ϊ0
				this->weight_token_buffer_count_ += 1;  // ������һ��token���У��ļ�������
				if (this->weight_token_buffer_count_ == this->weight_token_buffer_cur_max_)  // ����Ѿ��ռ�������token
				{
					// ����ָʾ�ռ�weight���Ѿ����Խ���
					// this->reserved_buffer_count_ = 0;
					// TODO������ָʾ������ռ���ı�ʾ�������һ��NPUֻ�Ὣ����͸���һ��NPU��һ��һ
					this->pair_weight_collect_over_ = true;
					// �ռ�������token��Ȩ�����ݺ󣬿��Գ��Խ��յ�һ��token������
					// TODO��������֮���Ǽ����뼤����ˣ���˺��ʿ�ʼ���ܵ�һ������token��Ҫ���ǣ��ռ����ȼ����ռ����ٷ���
					if (this->source_npu_->pair_act_collect_over_)
					{
						int current_time = _event->handle_time_;
						// TODO�������Ƿ���ҪΪcurrent_time��һ��ʱ�ӣ�
						DES::Event* new_event = new DES::Event(DES::Event::kNpuSendAct, current_time, std::bind(&NPUStoreSend::Excute, this->source_npu_, std::placeholders::_1));
						return new_event;
					}
				}
			}
		}
		// ����������������event
		return nullptr;
	}

	// �ܹ�receive Act���ʹ����NPU�ϵ�ƹ��buffer�ﻹ�п�λ
	// TODO����router�����֧�֣�������NPUƹ��buffer�����ˣ���ô�Ͳ����ٷ���������
	DES::Event* NPUActAct::ReceiveAct(DES::Event* _event)
	{
		// ����ռ���ϲ���δ������ϣ���ôҲ�����������ݽ��룬˵����ʱ�ڽ����ռ�weight�Ĺ���
		if (this->pair_weight_collect_over_ == false)
		{
			DES::Event* new_event = this->ReceiveWeight(_event);
			return new_event;
		}
		// ���о�������ռ���������
		else
		{
			// TODO������router�����Ӷ�buffer_is_full_��֧�֣�����Ӧ�������ṩ֧�֣��ǵü��
			int store_buffer_index_ = 1 - this->which_activate_buffer_;  // ƹ��buffer�����ڴ洢��index
			this->input_activate_buffer_count_[store_buffer_index_]++;
			if (this->input_activate_buffer_count_[store_buffer_index_] < input_activate_buffer_max_)
			{
				std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " at " << store_buffer_index_ << " buffer at time " << _event->handle_time_ << std::endl;
				return nullptr;
			}
			else
			{
				int current_time = _event->handle_time_;
				if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0) // ������ڼ����buffer��ʱ�ǿ� 
				{
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved input act: " << this->input_activate_buffer_count_[store_buffer_index_] << " at " << store_buffer_index_ << " buffer at time " << current_time << std::endl;
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << " and can start compute" << std::endl;
					// ���ڼ����buffer index��Ϊstore buffer
					this->which_activate_buffer_ = store_buffer_index_;
					DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
					return new_event;
				}
				// ˵����ʱ���ڼ����buffer���ڼ��㣬������Ԥȡ��bufferҲ�ռ���ϣ���ʱƹ��buffer��û�пռ�
				// TODO����router�д���kNpuReceiveAct�¼����߼������Ӷ�NPUbufferȫ����֧��
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

	// TODO��compute�¼����Ա�ѹ��ɾ��������sendAct��������compute�������ܱ�ɾ��
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
		// һ��sendAct�¼�������Ӧ�ò���ʲô���¼�
		std::vector<DES::Event*> new_events;

		if (this->router_->buffer_count_[Router::Direction::kNpu] >= this->router_->buffer_max_)
		{
			this->wait_for_send_ = true;
			return new_events;
		}
		else
		{
			// NPU�е�routerһ�����Խ��ո�����
			this->input_activate_buffer_count_[this->which_activate_buffer_] -= 1;  // ����buffer��������
			int current_time = _event->handle_time_;
			DES::Event* new_event = new DES::Event(DES::Event::kRouterReceiveAct, current_time + this->send_delay_, std::bind(&LLM::Router::Excute, this->router_, std::placeholders::_1));
			// TODO: ע������Ҫ��last_hop����ΪkNpu
			new_event->supporting_information_ = std::static_pointer_cast<void>(this->message_);
			std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")act sent now at: " << current_time << std::endl;
			this->router_->update_buffer_count_(Router::Direction::kNpu, 1);  // ����router�е�bufferռ����
			new_events.emplace_back(new_event);

			std::cout << "temp     " << this->input_activate_buffer_count_[this->which_activate_buffer_] << std::endl;
			if (this->input_activate_buffer_count_[this->which_activate_buffer_] == 0)  // ������һ��token���У��ļ������ݣ�����Ҫ�ټ���������
			{
				// reserve������ʹ�õ�buffer�ռ����
				this->reserved_buffer_count_ -= this->input_activate_buffer_max_;
				// TODO�������ǲ�����ȷ�ģ�
				this->weight_token_buffer_count_ -= 1;
				std::cout << "tempp     " << this->weight_token_buffer_count_ << std::endl;
				// ˵���Ѿ�����Ҫ����������
				if (this->weight_token_buffer_count_ == 0)
				{
					// TODO��������� ֱ�ӱ��0
					this->reserved_weight_buffer_count_ = 0;
					this->pair_weight_collect_over_ = false; // Ϊ��һ���ռ���׼��
					int wake_sum = std::min((int)this->weight_wait_queue_.size(), this->input_activate_buffer_max_ * this->weight_token_buffer_cur_max_);
					std::cout << "npu (1, 6) wake_sum is " << wake_sum << std::endl;
					for (int t = 0; t < wake_sum; ++t)
					{
						std::shared_ptr<Router::Direction> last_hop = this->weight_wait_queue_.front();
						this->weight_wait_queue_.pop();
						// ����һ���÷����RouterRoutingAct�¼�
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing weight to npu buffer at time" << current_time + t << std::endl;
					}

					this->source_npu_->reserved_buffer_count_ = 0;
					this->source_npu_->pair_act_collect_over_ = false;  // Ϊ��һ���ռ���׼��
					wake_sum = std::min((int)this->source_npu_->wait_queue_.size(), this->source_npu_->input_activate_buffer_max_ * this->source_npu_->input_token_buffer_cur_max_);
					std::cout << "npu (0, 5) wake_sum is " << wake_sum << std::endl;
					for (int t = 0; t < wake_sum; ++t)
					{
						std::shared_ptr<Router::Direction> last_hop = this->source_npu_->wait_queue_.front();
						this->source_npu_->wait_queue_.pop();
						// ����һ���÷����RouterRoutingAct�¼�
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->source_npu_->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->source_npu_->router_->coordinate_.first << ", " << this->source_npu_->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing input to npu buffer at time" << current_time + t << std::endl;
					}
				}

				// TODO�������費��Ҫ��һ��else��
				// �������Ԥȡ��buffer�Ѿ�����
				if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] == this->input_activate_buffer_max_)
				{
					// ����һЩ��ʼ����֧�֣����Կ�������Ԥȡ��buffer
					this->buffer_is_full_ = false;
					this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
					DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
					new_events.emplace_back(new_event);
					std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ") have recieved all input and start compute at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << std::endl;
					// TODO���鿴wait_queue_���Ƿ������������ݣ������Ƿ�Ӧ�������������Ҳ���
					int wake_sum = std::min((int)this->wait_queue_.size(), this->input_activate_buffer_max_);
					for (int t = 0; t < wake_sum; ++t)
					{
						std::shared_ptr<Router::Direction> last_hop = this->wait_queue_.front();
						this->wait_queue_.pop();
						// ����һ���÷����RouterRoutingAct�¼�
						DES::Event* new_event = new DES::Event(DES::Event::kRouterRoutingAct, current_time + t, std::bind(&Router::Excute, this->router_, std::placeholders::_1));
						new_event->supporting_information_ = std::static_pointer_cast<void>(last_hop);
						new_events.emplace_back(new_event);
						std::cout << "npu (" << this->router_->coordinate_.first << ", " << this->router_->coordinate_.second << ")hook router buffer_" << *last_hop << " to routing act to npu buffer at time" << current_time + t << std::endl;
					}
				}
				return new_events;
			}
			else  // ������Ҫ���������
			{
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUActAct::Excute, this, std::placeholders::_1));
				new_events.emplace_back(new_event);
				return new_events;
			}
		}
	}

}