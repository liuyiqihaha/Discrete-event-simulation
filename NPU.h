#pragma once

#include <vector>

#include "Event.h"
#include "Demo_component.h"

namespace LLM
{

	class NPU
	{
	public:
		int compute_finish_time_; //�Ƿ����ڼ���

		int input_activate_buffer_max_; //����buffer��С
		int which_activate_buffer_; //ʹ���ĸ�buffer����
		int input_activate_buffer_count_[2]; //ÿ��buffer�ѱ�����������

		int weight_buffer_max_; //Ȩ��buffer��С
		int weight_buffer_height_;
		int weight_buffer_width_;

		int N_P_; //token���ж�
		int K_P_; //����ͨ�����ж�
		int M_P_; //���ͨ�����ж�

		int time_to_compute_once_; //���д������32�����ݵ�ʱ��
		int needed_compute_time_; //���м������
		int computed_time_; //�Ѿ�����Ĵ���

		NPU();

		virtual ~NPU() {};
		virtual std::vector<DES::Event*> Excute(DES::Event* _event) = 0;
	};
	
	class NPUWeiAct : public NPU
	{
	public:
		DES::DemoComponent* demo_component_; //��ʱ���router

		std::vector<DES::Event*> Excute(DES::Event* _event) override;

		DES::Event* ReceiveAct(DES::Event* _event);
		DES::Event* Compute(DES::Event* _event);
		DES::Event* SendAct(DES::Event* _event);

	public:
		NPUWeiAct();
		NPUWeiAct(DES::DemoComponent* demo_component);

		~NPUWeiAct();
	};
}