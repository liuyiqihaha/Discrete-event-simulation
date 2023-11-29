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
		int yDim_start_;  // 将物理上的多个chip合成逻辑上的一个chip，每个chip的起始column为yDim_start
		ChipType chiptype_;  // 该chip的种类

	public:
		Chip(ChipType chiptype, int yDim_start);
		~Chip();
		void Mapping();  // 确定chip中各NPU的互联，即激活传输的起始NPU和目的NPU
	};

	class Chiplet
	{
	public:
		std::vector<Chip*> chips_;
		std::vector<Router*> input_routers_;  // 用于输入最初的prompt
	public:
		Chiplet(int sum_of_chips_);
		~Chiplet();
		void Mapping();  // 依次调用每个chip的Mapping函数
		void Run();
	};
}