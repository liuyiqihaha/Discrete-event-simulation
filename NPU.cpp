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
		computed_time_(0)
	{
		this->time_to_compute_once_ = this->weight_buffer_height_ / this->K_P_;
		this->needed_compute_time_ = this->weight_buffer_width_ / this->M_P_;
		input_activate_buffer_count_[0] = 0;
		input_activate_buffer_count_[1] = 0;
	}
	
	NPUWeiAct::NPUWeiAct() : 
		NPU(){}
	
	NPUWeiAct::NPUWeiAct(DES::DemoComponent* demo_component) :
		NPU(),
		demo_component_(demo_component){}

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
	{}

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
		case DES::Event::KNpuSendAct:
		{
			DES::Event* new_event = this->SendAct(_event);
			vec_ret.push_back(new_event);
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

	DES::Event* NPUWeiAct::ReceiveAct(DES::Event* _event)
	{
		this->input_activate_buffer_count_[1 - this->which_activate_buffer_]++;
		int current_time = _event->handle_time_;
		if (this->input_activate_buffer_count_[1 - this->which_activate_buffer_] < input_activate_buffer_max_)
		{
			std::cout << "recieved input act " << this->input_activate_buffer_count_[1 - this->which_activate_buffer_] << " at: " << current_time << std::endl;
			return nullptr;
		}
		else
		{
			this->input_activate_buffer_count_[1 - this->which_activate_buffer_] = 0;
			this->which_activate_buffer_ = 1 - this->which_activate_buffer_;
			if (current_time >= this->compute_finish_time_)
			{
				//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
				std::cout << "recieved all input act now at: " << current_time /*<< ", time need to compute: " << time_to_compute*/ << std::endl;
				this->compute_finish_time_ = current_time + this->time_to_compute_once_ * this->needed_compute_time_;
				return new_event;
			}
			else
			{
				//int time_to_compute = this->weight_buffer_max_ / this->N_P_ / this->K_P_ / this->M_P_;	
				DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, this->compute_finish_time_ /*+ time_to_compute*/, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
				std::cout << "recieved all input act now at: " << current_time << " but has to wait until: " << this->compute_finish_time_ << std::endl;
				this->compute_finish_time_ = this->compute_finish_time_ + this->time_to_compute_once_ * this->needed_compute_time_;
				return new_event;
			}
		}
	}

	DES::Event* NPUWeiAct::Compute(DES::Event* _event)
	{
		this->computed_time_++;
		int current_time = _event->handle_time_;
		if (this->computed_time_ < this->needed_compute_time_)
		{
			DES::Event* new_event = new DES::Event(DES::Event::kNpuCompute, current_time + time_to_compute_once_, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
			std::cout << "start compute now at: " << current_time << ", time need to compute: " << this->time_to_compute_once_ << " compute iter " << this->computed_time_ << std::endl;
			return new_event;
		}
		else
		{
			DES::Event* new_event = new DES::Event(DES::Event::KNpuSendAct, current_time + this->time_to_compute_once_, std::bind(&NPUWeiAct::Excute, this, std::placeholders::_1));
			std::cout << "start compute now at: " << current_time << ", time need to compute: " << this->time_to_compute_once_ << " compute iter " << this->computed_time_ << std::endl;
			this->computed_time_ = 0;
			return new_event;
		}
	}

	DES::Event* NPUWeiAct::SendAct(DES::Event* _event)
	{
		int current_time = _event->handle_time_;
		DES::Event* new_event = new DES::Event(current_time, std::bind(&DES::DemoComponent::Excute, this->demo_component_, std::placeholders::_1));
		std::cout << "act sent now at: " << current_time << std::endl;
		return new_event;
	}
}