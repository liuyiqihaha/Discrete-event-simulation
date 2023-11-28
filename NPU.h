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
		int compute_finish_time_; //�Ƿ����ڼ���

		int input_activate_buffer_max_; //����buffer��С
		int which_activate_buffer_; //ʹ���ĸ�buffer����
		int input_activate_buffer_count_[2]; //ÿ��buffer�ѱ�����������
		int reserved_buffer_count_;  // ָʾbuffer�Ѿ�Ԥ������˶���

		bool buffer_is_full_;  // ָʾ���ܷ���buffer��������
		bool wait_for_send_;  // NPU buffer���Ƿ������ݴ��䱻����

		int weight_buffer_max_; //Ȩ��buffer��С
		int weight_buffer_height_;
		int weight_buffer_width_;

		int N_P_; //token���ж�
		int K_P_; //����ͨ�����ж�
		int M_P_; //���ͨ�����ж�

		int time_to_compute_once_; //���д������32�����ݵ�ʱ��
		int needed_compute_time_; //���м������
		int computed_time_; //�Ѿ�����Ĵ���
		int send_delay_;  // ����ʱ��
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
		int input_token_buffer_cur_max_; // ��prompt��Ҫ�ռ�����token�ļ���
		int input_token_buffer_count_; // ��ǰ���ռ�����token�ļ���
		bool pair_act_collect_over_;  // ����token�ļ����Ƿ����ռ����
		NPUActAct* destination_npu_;  // ��Ӧ���ռ�Ȩ�ص�npu������������

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
		int weight_token_buffer_cur_max_; // ��prompt��Ҫ�ռ�����token��Ȩ��
		int weight_token_buffer_count_; // ��ǰ���ռ�����token��Ȩ��
		bool pair_weight_collect_over_;  // ����token��Ȩ���Ƿ����ռ����
		NPUStoreSend* source_npu_;  // ��Ӧ���ռ������npu������������

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