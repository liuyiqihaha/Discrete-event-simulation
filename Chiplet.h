#pragma once
#include "NPU.h"
#include "Router.h"
#include "Simulator.h"

namespace LLM
{
	class Chip
	{
	public:
		enum ChipType
		{
			Chip1 = 0,
			Chip2,
			Chip3,
			Chip4,
			Chip5,
			Chip6
		};

	public:
		std::vector<std::vector<NPU*>> npus_;
		std::vector<std::vector<Router*>> routers_;
		int xDim;
		int yDim;
		int yDim_start_;  // �������ϵĶ��chip�ϳ��߼��ϵ�һ��chip��ÿ��chip����ʼcolumnΪyDim_start
		ChipType chiptype_;  // ��chip������

	public:
		Chip(ChipType chiptype, int yDim_start);
		~Chip();
		void Mapping();  // ȷ��chip�и�NPU�Ļ���������������ʼNPU��Ŀ��NPU
	};

	class Chiplet
	{
	public:
		std::vector<Chip*> chips_;
		std::vector<Router*> input_routers_;  // �������������prompt
	public:
		Chiplet(int sum_of_chips_);
		~Chiplet();
		void Mapping();  // ���ε���ÿ��chip��Mapping����
		void Run();
	};
}