#pragma once

#include <vector>

#include "Event.h"
#include "Demo_component.h"

namespace LLM
{

	class NPU
	{
	public:
		int compute_finish_time_; //是否正在计算

		int input_activate_buffer_max_; //输入buffer大小
		int which_activate_buffer_; //使用哪个buffer计算
		int input_activate_buffer_count_[2]; //每个buffer已被填充多少数据

		int weight_buffer_max_; //权重buffer大小
		int weight_buffer_height_;
		int weight_buffer_width_;

		int N_P_; //token并行度
		int K_P_; //输入通道并行度
		int M_P_; //输出通道并行度

		int time_to_compute_once_; //阵列处理输出32个数据的时间
		int needed_compute_time_; //阵列计算次数
		int computed_time_; //已经计算的次数

		NPU();

		virtual ~NPU() {};
		virtual std::vector<DES::Event*> Excute(DES::Event* _event) = 0;
	};
	
	class NPUWeiAct : public NPU
	{
	public:
		DES::DemoComponent* demo_component_; //暂时替代router

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