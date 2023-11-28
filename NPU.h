#pragma once

#include <vector>
#include <algorithm>
#include <queue>

#include "Event.h"
#include "Demo_component.h"
#include "Router.h"

namespace LLM
{
	//class Router;
	class Message;

	class NPU
	{
	public:
		enum NpuType {
			WeiAct = 0,
			StoreSend,
			ActAct,
		};

	public:
		NpuType type_;
		int compute_finish_time_; //是否正在计算

		int input_activate_buffer_max_; //输入buffer大小
		int which_activate_buffer_; //使用哪个buffer计算
		int input_activate_buffer_count_[2]; //每个buffer已被填充多少数据
		int reserved_buffer_count_;  // 指示buffer已经预分配出了多少

		bool buffer_is_full_;  // 指示还能否像buffer传送数据
		bool wait_for_send_;  // NPU buffer中是否与数据传输被阻塞

		int weight_buffer_max_; //权重buffer大小
		int weight_buffer_height_;
		int weight_buffer_width_;

		int N_P_; //token并行度
		int K_P_; //输入通道并行度
		int M_P_; //输出通道并行度

		int time_to_compute_once_; //阵列处理输出32个数据的时间
		int needed_compute_time_; //阵列计算次数
		int computed_time_; //已经计算的次数
		int send_delay_;  // 传输时延
		Router* router_;

		std::queue<std::shared_ptr<Router::Direction>> wait_queue_;
		std::shared_ptr<LLM::Message> message_;

		NPU();
		void Mapping(std::vector<std::pair<int, int>> dest_);

		virtual ~NPU() {};
		virtual std::vector<DES::Event*> Excute(DES::Event* _event) = 0;
	};
	
	class NPUWeiAct : public NPU
	{
	public:
		std::vector<DES::Event*> Excute(DES::Event* _event) override;

		DES::Event* ReceiveAct(DES::Event* _event);
		DES::Event* Compute(DES::Event* _event);
		std::vector<DES::Event*> SendAct(DES::Event* _event);

	public:
		NPUWeiAct();
		NPUWeiAct(Router* router);

		~NPUWeiAct();
	};

	class NPUActAct;

	class NPUStoreSend : public NPU
	{
	public:
		int input_token_buffer_max_;  // 
		int input_token_buffer_cur_max_; // 此prompt需要收集多少token的激活
		int input_token_buffer_count_; // 当前以收集多少token的激活
		bool pair_act_collect_over_;  // 所有token的激活是否已收集完毕
		NPUActAct* destination_npu_;  // 对应的收集权重的npu，两者相运算

		std::vector<DES::Event*> Excute(DES::Event* _event) override;
		void Mapping(LLM::NPUActAct* destination_npu);

		DES::Event* ReceiveAct(DES::Event* _event);
		std::vector<DES::Event*> SendAct(DES::Event* _event);
		// DES::Event* SendReceiveAct(DES::Event* _event);

	public:
		NPUStoreSend();
		NPUStoreSend(Router* router);
		~NPUStoreSend();
	};


	class NPUActAct : public NPU
	{
	public:
		int weight_token_buffer_max_;  // 
		int weight_token_buffer_cur_max_; // 此prompt需要收集多少token的权重
		int weight_token_buffer_count_; // 当前以收集多少token的权重
		bool pair_weight_collect_over_;  // 所有token的权重是否已收集完毕
		NPUStoreSend* source_npu_;  // 对应的收集激活的npu，两者相运算

		std::queue<std::shared_ptr<Router::Direction>> weight_wait_queue_;
		int reserved_weight_buffer_count_;

	public:
		NPUActAct();
		NPUActAct(Router* router);
		~NPUActAct();

		std::vector<DES::Event*> Excute(DES::Event* _event) override;

		DES::Event* ReceiveWeight(DES::Event* _event);
		DES::Event* ReceiveAct(DES::Event* _event);
		std::vector<DES::Event*> SendAct(DES::Event* _event);
		DES::Event* Compute(DES::Event* _event);
	};
}