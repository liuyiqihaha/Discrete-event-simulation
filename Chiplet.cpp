#include "Chiplet.h"
#include "configuration.h"

namespace LLM
{
    Chip::Chip(ChipType chiptype, int yDim_start)
        :chiptype_(chiptype), 
        yDim_start_(yDim_start),
        xDim(8),
        yDim(7) // ��chip�ĳߴ�
    {
		this->npus_ = std::vector<std::vector<NPU*>>(xDim); // ��chip����NPU
        this->routers_ = std::vector<std::vector<Router*>>(xDim + 1); // ��chip����Router

        for (int x = 0; x < xDim; ++x)
        {
            npus_[x].resize(yDim);
            routers_[x].resize(yDim);
        }

        // ����chip���಻ͬ�����ò�ͬ��NPU
		for (int x = 0; x < xDim; ++x)
		{
			for (int y = 0; y < yDim; ++y)
			{
				this->routers_[x][y] = new Router(x, y + yDim_start_);
				if (NPU_TYPE[chiptype][x][y] == 0)
				{
					this->npus_[x][y] = new NPUWeiAct(this->routers_[x][y]);
				}
                else if (NPU_TYPE[chiptype][x][y] == 1)
                {
                    this->npus_[x][y] = new NPUStoreSend(this->routers_[x][y]);
                }
                else
                {
                    this->npus_[x][y] = new NPUActAct(this->routers_[x][y]);
                }
			}
		}

        //Router* new_router;

        //// ����chip���಻ͬ�����ò�ͬ��NPU
        //switch (chiptype)
        //{
        //case Chip::ChipType::Chip1:
        //{
        //    /*
        //    ����Chip1��
        //        ����ǰ5�У�ΪWeiAct��NPU
        //        ���ڵ�6�У�ΪStoreSend��NPU
        //        ���ڵ�7�У�ΪActAct��NPU
        //    */
        //    for (int x = 0; x < xDim; ++x)
        //    {
        //        for (int y = 0; y < 5; ++y)
        //        {
        //            new_router = new Router(x, y + yDim_start_);
        //            this->npus_[x][y] = new NPUWeiAct(new_router);
        //        }
        //    }
        //    for (int x = 0; x < xDim; ++x)
        //    {
        //        new_router = new Router(x, 5 + yDim_start_);
        //        this->npus_[x][5] = new NPUStoreSend(new_router);
        //        new_router = new Router(x, 6 + yDim_start_);
        //        this->npus_[x][6] = new NPUActAct(new_router);
        //    }
        //    break;
        //}
        //default:
        //    break;
        //}
    }

    Chip::~Chip()
    {
    }

    void Chip::Mapping()
    {
        switch (this->chiptype_)
        {
        case Chip::ChipType::Chip1:
        {
            
            // ����chip���಻ͬ������NPU֮��Ļ�����ϵ��������������Ե�һ��Chip�Ĵ���
            // NPU(0, 0)..NPU(0, 4)Ҫ�����������ݸ�NPU(0, 5)��NPU(1, 5)
            // NPU(2, 0)..NPU(2, 4)Ҫ�����������ݸ�NPU(2, 5)��NPU(3, 5)
            std::vector<std::pair<int, int>> dests;
            dests.resize(2);
            for (int i = 0; i < 8; i += 2)
            {
                dests[0].first = i;
                dests[0].second = this->yDim_start_ + 5;
                dests[1].first = i + 1;
                dests[1].second = this->yDim_start_ + 5;
                for (int j = 0; j < 5; ++j)
                    this->npus_[i][j]->Mapping(dests);
            }
            for (int i = 1; i < 8; i += 2)
            {
                dests[0].first = i;
                dests[0].second = this->yDim_start_ + 6;
                dests[1].first = i - 1;
                dests[1].second = this->yDim_start_ + 6;
                for (int j = 0; j < 5; ++j)
                    this->npus_[i][j]->Mapping(dests);
            }

            // NPU(0, 5)..NPU(7, 5)��NPU(0, 6)..NPU(7, 6)һһ��Ӧ
            dests.resize(1);
            for (int i = 0; i < 8; ++i)
            {
                dests[0].first = i;
                dests[0].second = this->yDim_start_ + 6;
                this->npus_[i][5]->Mapping(dests);
            }

            // NPU(0, 6)..NPU(7, 6)��NPU(0, 10)..NPU(3, 10)��NPU(4, 9)..NPU(7, 9)һһ��Ӧ
            dests.resize(1);
            for (int i = 0; i < 8; ++i)
            {
                if (i >= 0 && i <= 3)
                    dests[0].second = this->yDim_start_ + 11;
                else
                    dests[0].second = this->yDim_start_ + 10;
                dests[0].first = i;
                this->npus_[i][6]->Mapping(dests);
            }

            // ������chip1���е�mapping
            

            /*
            // TODO��һ��demoʵ�������Ե���
            std::pair<int, int> dest1(0, 5);
            std::pair<int, int> dest2(1, 6);
            std::pair<int, int> dest3(2, 5);
            this->npus_[0][0]->Mapping(std::vector<std::pair<int, int>>{dest1});
            this->npus_[1][0]->Mapping(std::vector<std::pair<int, int>>{dest2});
            this->npus_[0][5]->Mapping(std::vector<std::pair<int, int>>{dest2});
            this->npus_[1][6]->Mapping(std::vector<std::pair<int, int>>{dest3});
            dynamic_cast<NPUStoreSend*>(this->npus_[0][5])->Mapping(dynamic_cast<NPUActAct*>(this->npus_[1][6]));

            this->npus_[0][0]->input_activate_buffer_max_ = 2;
            this->npus_[1][0]->input_activate_buffer_max_ = 2;
            this->npus_[0][5]->input_activate_buffer_max_ = 2;
            this->npus_[1][6]->input_activate_buffer_max_ = 2;
            dynamic_cast<NPUStoreSend*>(this->npus_[0][5])->input_token_buffer_cur_max_ = 3;
            dynamic_cast<NPUActAct*>(this->npus_[1][6])->weight_token_buffer_cur_max_ = 3;
            */

            // һ��demoʵ�������Զಥ
            std::vector<std::pair<int, int>> dests04;
            for (int i = 0; i < 2; ++i)
            {
                dests04.emplace_back(std::pair<int, int>{i, 5});
            }
            this->npus_[0][4]->Mapping(dests04);
            this->npus_[0][4]->input_activate_buffer_max_ = 4;

            std::vector<std::pair<int, int>> dests14;
            for (int i = 0; i < 2; ++i)
            {
                dests14.emplace_back(std::pair<int, int>{i, 6});
            }
            this->npus_[1][4]->Mapping(dests14);
            this->npus_[1][4]->input_activate_buffer_max_ = 4;

            std::vector<std::pair<int, int>> dests24;
            for (int i = 2; i < 4; ++i)
            {
                dests24.emplace_back(std::pair<int, int>{i, 5});
            }
            this->npus_[2][4]->Mapping(dests24);
            this->npus_[2][4]->input_activate_buffer_max_ = 4;

            std::vector<std::pair<int, int>> dests34;
            for (int i = 2; i < 4; ++i)
            {
                dests34.emplace_back(std::pair<int, int>{i, 6});
            }
            this->npus_[3][4]->Mapping(dests34);
            this->npus_[3][4]->input_activate_buffer_max_ = 4;

            this->npus_[0][5]->Mapping(std::vector<std::pair<int, int>>{ {0, 6}});
            dynamic_cast<NPUStoreSend*>(this->npus_[0][5])->Mapping(dynamic_cast<NPUActAct*>(this->npus_[0][6]));
            this->npus_[0][5]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUStoreSend*>(this->npus_[0][5])->input_token_buffer_cur_max_ = 3;
            this->npus_[0][6]->Mapping(std::vector<std::pair<int, int>>{ {0, 7}});
            this->npus_[0][6]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUActAct*>(this->npus_[0][6])->weight_token_buffer_cur_max_ = 3;

            this->npus_[1][5]->Mapping(std::vector<std::pair<int, int>>{ {1, 6}});
            dynamic_cast<NPUStoreSend*>(this->npus_[1][5])->Mapping(dynamic_cast<NPUActAct*>(this->npus_[1][6]));
            this->npus_[1][5]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUStoreSend*>(this->npus_[1][5])->input_token_buffer_cur_max_ = 3;
            this->npus_[1][6]->Mapping(std::vector<std::pair<int, int>>{ {1, 7}});
            this->npus_[1][6]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUActAct*>(this->npus_[1][6])->weight_token_buffer_cur_max_ = 3;

            this->npus_[2][5]->Mapping(std::vector<std::pair<int, int>>{ {2, 6}});
            dynamic_cast<NPUStoreSend*>(this->npus_[2][5])->Mapping(dynamic_cast<NPUActAct*>(this->npus_[2][6]));
            this->npus_[2][5]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUStoreSend*>(this->npus_[2][5])->input_token_buffer_cur_max_ = 3;
            this->npus_[2][6]->Mapping(std::vector<std::pair<int, int>>{ {2, 7}});
            this->npus_[2][6]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUActAct*>(this->npus_[2][6])->weight_token_buffer_cur_max_ = 3;

            this->npus_[3][5]->Mapping(std::vector<std::pair<int, int>>{ {3, 6}});
            dynamic_cast<NPUStoreSend*>(this->npus_[3][5])->Mapping(dynamic_cast<NPUActAct*>(this->npus_[3][6]));
            this->npus_[3][5]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUStoreSend*>(this->npus_[3][5])->input_token_buffer_cur_max_ = 3;
            this->npus_[3][6]->Mapping(std::vector<std::pair<int, int>>{ {3, 7}});
            this->npus_[3][6]->input_activate_buffer_max_ = 4;
            dynamic_cast<NPUActAct*>(this->npus_[3][6])->weight_token_buffer_cur_max_ = 3;
            break;
        }
        case Chip::ChipType::Chip2:
        {
			std::vector<std::pair<int, int>> dests;
			dests.resize(2);
			for (int i = 0; i < 8; i += 2)
			{
                if (i < 4)
                {
					dests[0].first = i;
					dests[0].second = this->yDim_start_ + 4;
					dests[1].first = i + 1;
					dests[1].second = this->yDim_start_ + 4;
                }
                else
                {
					dests[0].first = i;
					dests[0].second = this->yDim_start_ + 3;
					dests[1].first = i + 1;
					dests[1].second = this->yDim_start_ + 3;
                }
				this->npus_[i][0]->Mapping(dests);
                this->npus_[i][1]->Mapping(dests);
			}
			for (int i = 1; i < 8; i += 2)
			{
                if (i < 4)
                {
					dests[0].first = i;
					dests[0].second = this->yDim_start_ + 4;
					dests[1].first = i - 1;
					dests[1].second = this->yDim_start_ + 4;
                }
                else
                {
					dests[0].first = i;
					dests[0].second = this->yDim_start_ + 3;
					dests[1].first = i - 1;
					dests[1].second = this->yDim_start_ + 3;
                }
				this->npus_[i][0]->Mapping(dests);
				this->npus_[i][1]->Mapping(dests);
			}

            {
				dests[0].first = 0;
				dests[0].second = this->yDim_start_ + 4;
				dests[1].first = 1;
				dests[1].second = this->yDim_start_ + 4;
                this->npus_[0][2]->Mapping(dests);

				dests[0].first = 2;
				dests[0].second = this->yDim_start_ + 4;
				dests[1].first = 3;
				dests[1].second = this->yDim_start_ + 4;
				this->npus_[1][2]->Mapping(dests);

				dests[0].first = 4;
				dests[0].second = this->yDim_start_ + 3;
				dests[1].first = 5;
				dests[1].second = this->yDim_start_ + 3;
				this->npus_[2][2]->Mapping(dests);

				dests[0].first = 6;
				dests[0].second = this->yDim_start_ + 3;
				dests[1].first = 7;
				dests[1].second = this->yDim_start_ + 3;
				this->npus_[3][2]->Mapping(dests);
            }

            dests.resize(0);
            for (int i = 0; i < 8; i++)
            {
                if (i >= 4)
                {
                    std::pair<int, int> dest1{ i, this->yDim_start_ + 4 };
                    dests.push_back(dest1);
                }
                std::pair<int, int> dest2{ i, this->yDim_start_ + 5 };
                std::pair<int, int> dest3{ i, this->yDim_start_ + 6 };
                dests.push_back(dest2);
                dests.push_back(dest3);
            }
			for (int i = 0; i < 8; i++)
			{
				if (i < 4)
				{
                    this->npus_[i][3]->Mapping(dests);
                    this->npus_[i][4]->Mapping(dests);
				}
                else
                {
					this->npus_[i][3]->Mapping(dests);
					this->npus_[i][2]->Mapping(dests);
                }
			}

            dests.resize(0);
            for (int i = 0; i < 8; i++)
			{
                for (int j = 0; j < 7; j++)
                {
                    if (i > 5 && j > 4)
                    {
                        continue;
                    }
                    std::pair<int, int> dest1{ i, this->yDim_start_ + j + 7 };
					std::pair<int, int> dest2{ i, this->yDim_start_ + j + 14 };
					std::pair<int, int> dest3{ i, this->yDim_start_ + j + 21};
                    dests.push_back(dest1);
                    dests.push_back(dest2);
                    dests.push_back(dest3);
                }
			}
			for (int i = 0; i < 8; i++)
			{
				if (i >= 4)
				{
					this->npus_[i][4]->Mapping(dests);
				}
				this->npus_[i][5]->Mapping(dests);
				this->npus_[i][6]->Mapping(dests);
			}

            break;
        }
        case Chip::ChipType::Chip3:
        {
            std::vector<std::pair<int, int>> dests;
            dests.resize(1);

            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 7; j++)
                {
                    if (i > 5 && j > 4)
                    {
                        continue;
                    }
					dests[0].first = i;
					dests[0].second = this->yDim_start_ + j + 7;
                    this->npus_[i][j]->Mapping(dests);
                }
            }
        }
		case Chip::ChipType::Chip4:
		{
			std::vector<std::pair<int, int>> dests;
            dests.resize(0);

			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 7; j++)
				{
					if (i > 5 && j > 4)
					{
						continue;
					}
					std::pair<int, int> dest1{ i, this->yDim_start_ + j + 7 };
					dests.push_back(dest1);
				}
			}
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 7; j++)
				{
					if (i > 5 && j > 4)
					{
						continue;
					}
					this->npus_[i][j]->Mapping(dests);
				}
			}
		}
		case Chip::ChipType::Chip5:
		{
			std::vector<std::pair<int, int>> dests;
			dests.resize(0);

			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 5; j++)
				{
					std::pair<int, int> dest1{ i, this->yDim_start_ + j + 7 };
					dests.push_back(dest1);
				}
			}

			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					std::pair<int, int> dest1{ i, this->yDim_start_ + j + 14 };
					dests.push_back(dest1);
				}
			}

			for (int i = 0; i < 4; i++)
			{
				std::pair<int, int> dest1{ i, this->yDim_start_ + 3 + 7 };
				dests.push_back(dest1);
			}

			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 7; j++)
				{
					if (i > 5 && j > 4)
					{
						continue;
					}
					this->npus_[i][j]->Mapping(dests);
				}
			}
		}
        default:
            break;
        }
    }

    Chiplet::Chiplet(int sum_of_chips_)
        :chips_(std::vector<Chip*>(sum_of_chips_))
    {
        // ��ʼ�����е�chips
        this->chips_[0] = new Chip(Chip::ChipType::Chip1, 0);
        this->chips_[1] = new Chip(Chip::ChipType::Chip1, this->chips_[0]->yDim);

        // �����������ݵ�routers��ά����chips_[0]��xDim��ͬ
        this->input_routers_.resize(this->chips_[0]->xDim);
        for (int i = 0; i < this->chips_[0]->xDim; ++i)
        {
            this->input_routers_[i] = new Router(i, -1);
        }

        //TODO����ʼ��chips������router�Ļ���
        for (int index = 0; index < this->chips_.size(); ++index)
        {
            for (int i = 0; i < this->chips_[index]->xDim; ++i)
            {
                for (int j = 0; j < this->chips_[index]->yDim - 1; ++j)
                {
                    this->chips_[index]->npus_[i][j]->router_->adjcent_router_[Router::Direction::kRight] = this->chips_[index]->npus_[i][j + 1]->router_;
                    this->chips_[index]->npus_[i][j + 1]->router_->adjcent_router_[Router::Direction::kLeft] = this->chips_[index]->npus_[i][j]->router_;
                }
                if (index < this->chips_.size() - 1)
                {
                    this->chips_[index]->npus_[i][this->chips_[index]->yDim - 1]->router_->adjcent_router_[Router::Direction::kRight] = this->chips_[index + 1]->npus_[i][0]->router_;
                    this->chips_[index + 1]->npus_[i][0]->router_->adjcent_router_[Router::Direction::kLeft] = this->chips_[index]->npus_[i][this->chips_[index]->yDim - 1]->router_;
                }
            }
            for (int j = 0; j < this->chips_[index]->yDim; ++j)
            {
                for (int i = 0; i < this->chips_[index]->xDim - 1; ++i)
                {
                    this->chips_[index]->npus_[i][j]->router_->adjcent_router_[Router::Direction::kDown] = this->chips_[index]->npus_[i + 1][j]->router_;
                    this->chips_[index]->npus_[i + 1][j]->router_->adjcent_router_[Router::Direction::kUp] = this->chips_[index]->npus_[i][j]->router_;
                }
            }
        }

        // ����router������router�Ļ���
        for (int i = 0; i < this->input_routers_.size(); ++i)
        {
            this->input_routers_[i]->adjcent_router_[Router::Direction::kRight] = this->chips_[0]->npus_[i][0]->router_;
            this->chips_[0]->npus_[i][0]->router_->adjcent_router_[Router::Direction::kLeft] = this->input_routers_[i];
            if (i > 0)
            {
                this->input_routers_[i]->adjcent_router_[Router::Direction::kUp] = this->input_routers_[i - 1];
                this->input_routers_[i - 1]->adjcent_router_[Router::Direction::kDown] = this->input_routers_[i];
            }
        }
    }

    Chiplet::~Chiplet()
    {
        for (auto chip : this->chips_)
        {
            chip->~Chip();
        }
    }

    void Chiplet::Mapping()
    {
        for (auto chip : this->chips_)
        {
            chip->Mapping();
        }
    }

    void Chiplet::Run()
    {
        /* ���Ե���demo
        this->input_routers_[0]->buffer_max_ = 20;
        this->input_routers_[1]->buffer_max_ = 20;

        std::shared_ptr<LLM::Message> new_message0 = std::make_shared<LLM::Message>();
        std::pair<int, int> dest0;
        dest0.first = 0;
        dest0.second = 0;
        new_message0->destination_.push_back(dest0);

        std::shared_ptr<LLM::Message> new_message1 = std::make_shared<LLM::Message>();
        std::pair<int, int> dest1;
        dest1.first = 1;
        dest1.second = 0;
        new_message1->destination_.push_back(dest1);

        DES::Simulator* simulator = new DES::Simulator();

        // ��һ�ַ���
        // ���Խ�router1��routing delay����Ϊ0
        this->input_routers_[0]->routing_delay_ = 0;
        this->input_routers_[1]->routing_delay_ = 0;
        std::shared_ptr<LLM::Direction> last_direction = std::make_shared<LLM::Direction>(LLM::Direction::kLeft);
        for (int i = 0; i < 12; ++i)
        {
            if (i == 0)
            {
                DES::Event* _event0 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[0][0]->router_, std::placeholders::_1));
                _event0->supporting_information_ = std::static_pointer_cast<void>(new_message0);
                this->chips_[0]->npus_[0][0]->router_->input_event_buffer_[LLM::Direction::kLeft].push(_event0);
                this->chips_[0]->npus_[0][0]->router_->update_buffer_count_(LLM::Direction::kLeft, 1);

                DES::Event* _event1 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[1][0]->router_, std::placeholders::_1));
                _event1->supporting_information_ = std::static_pointer_cast<void>(new_message1);
                this->chips_[0]->npus_[1][0]->router_->input_event_buffer_[LLM::Direction::kLeft].push(_event1);
                this->chips_[0]->npus_[1][0]->router_->update_buffer_count_(LLM::Direction::kLeft, 1);
            }
            else
            {
                DES::Event* _event0 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->input_routers_[0], std::placeholders::_1));
                _event0->supporting_information_ = std::static_pointer_cast<void>(new_message0);
                this->input_routers_[0]->input_event_buffer_[LLM::Direction::kLeft].push(_event0);
                this->input_routers_[0]->buffer_count_[LLM::Direction::kLeft] += 1;
                this->input_routers_[0]->buffered_output_direction[LLM::Direction::kRight].push(last_direction);

                DES::Event* _event1 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->input_routers_[1], std::placeholders::_1));
                _event1->supporting_information_ = std::static_pointer_cast<void>(new_message1);
                this->input_routers_[1]->input_event_buffer_[LLM::Direction::kLeft].push(_event1);
                this->input_routers_[1]->buffer_count_[LLM::Direction::kLeft] += 1;
                this->input_routers_[1]->buffered_output_direction[LLM::Direction::kRight].push(last_direction);
            }
        }

        DES::Event* new_event0 = new DES::Event(DES::Event::kRouterRoutingAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[0][0]->router_, std::placeholders::_1));
        std::shared_ptr<LLM::Direction> direction0 = std::make_shared<LLM::Direction>();
        *direction0 = LLM::Direction::kLeft;
        new_event0->supporting_information_ = std::static_pointer_cast<void>(direction0);

        DES::Event* new_event1 = new DES::Event(DES::Event::kRouterRoutingAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[1][0]->router_, std::placeholders::_1));
        std::shared_ptr<LLM::Direction> direction1 = std::make_shared<LLM::Direction>();
        *direction1 = LLM::Direction::kLeft;
        new_event1->supporting_information_ = std::static_pointer_cast<void>(direction1);

        simulator->Enqueue(new_event0);
        simulator->Enqueue(new_event1);

        simulator->Simulate();
        */

        // ���Զಥdemo
		// ����˼·�ǣ�һ���������ݴ�NPU(0, 0)���룬�����м��㣬�㲥������NPU(0, 4)..NPU(3, 4)
		//          ��һ���������ݴ�NPU(1, 0)���룬�����м��㣬�㲥������NPU(0, 4)..NPU(3, 4)
		//          NPU(0, 4)��NPU(2, 4)���м��㣬����ݸ�NPU(0, 5)..NPU(3, 5)
		//          NPU(1, 4)��NPU(3, 4)���м��㣬����ݸ�NPU(0, 6)..NPU(3, 6)
		//          NPU(0, 5)..NPU(3, 5)�洢����Ϊ���NPU(0, 6)..NPU(3, 6)��Ϊweight
		//          NPU(0, 5)�ļ���ݸ�NPU(0, 6)����NPU(0, 6)�м��㣬NPU(1, 5)..NPU(3, 5)����
		//          NPU(0, 6)..NPU(3, 6)�еļ������ݷֱ𴫵ݸ�NPU(0, 7)..NPU(3, 7)
		// ��������������router�ȸ�����Ϣ����Ҫ��������router������¼�
        this->input_routers_[0]->buffer_max_ = 20;
        this->input_routers_[1]->buffer_max_ = 20;

        // �ʼ�������¼���Ҫ�㲥��NPU(0, 4)..NPU(3, 4)
        std::vector<std::pair<int, int>> input_dests;
        for (int i = 0; i < 4; ++i)
        {
            input_dests.emplace_back(std::pair<int, int>{i, 4});
        }

        std::shared_ptr<LLM::Message> new_message0 = std::make_shared<LLM::Message>();
        new_message0->message_type_ = Message::kBroadcast;
        new_message0->destination_ = input_dests;

        DES::Simulator* simulator = new DES::Simulator();
        this->input_routers_[0]->routing_delay_ = 0;
        this->input_routers_[1]->routing_delay_ = 0;
        std::shared_ptr<Router::Direction> last_direction = std::make_shared<Router::Direction>(Router::Direction::kLeft);
        for (int i = 0; i < 6; ++i)
        {
            // ��һ�ݼ������ݣ�Ҫֱ�ӷŵ�npu��input_event_buffer_��
            if (i == 0)
            {
                DES::Event* _event0 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[0][0]->router_, std::placeholders::_1));
                _event0->supporting_information_ = std::static_pointer_cast<void>(new_message0);
                this->chips_[0]->npus_[0][0]->router_->input_event_buffer_[Router::Direction::kLeft].push(_event0);
                this->chips_[0]->npus_[0][0]->router_->update_buffer_count_(Router::Direction::kLeft, 1);

                DES::Event* _event1 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[1][0]->router_, std::placeholders::_1));
                _event1->supporting_information_ = std::static_pointer_cast<void>(new_message0);
                this->chips_[0]->npus_[1][0]->router_->input_event_buffer_[Router::Direction::kLeft].push(_event1);
                this->chips_[0]->npus_[1][0]->router_->update_buffer_count_(Router::Direction::kLeft, 1);
            }
            // ����ļ������ݣ��ŵ�input_routers�У��ȴ���hook����
            else
            {
                DES::Event* _event0 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->input_routers_[0], std::placeholders::_1));
                _event0->supporting_information_ = std::static_pointer_cast<void>(new_message0);
                this->input_routers_[0]->input_event_buffer_[Router::Direction::kLeft].push(_event0);
                this->input_routers_[0]->buffer_count_[Router::Direction::kLeft] += 1;
                this->input_routers_[0]->buffered_output_direction[Router::Direction::kRight].push(last_direction);

                DES::Event* _event1 = new DES::Event(DES::Event::kRouterReceiveAct, 0, std::bind(&LLM::Router::Excute, this->input_routers_[1], std::placeholders::_1));
                _event1->supporting_information_ = std::static_pointer_cast<void>(new_message0);
                this->input_routers_[1]->input_event_buffer_[Router::Direction::kLeft].push(_event1);
                this->input_routers_[1]->buffer_count_[Router::Direction::kLeft] += 1;
                this->input_routers_[1]->buffered_output_direction[Router::Direction::kRight].push(last_direction);
            }
        }
        // �ʼΪ�������Ҫ�������ݵ�NPU router����kRouterRoutingAct�¼�
        DES::Event* new_event0 = new DES::Event(DES::Event::kRouterRoutingAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[0][0]->router_, std::placeholders::_1));
        std::shared_ptr<Router::Direction> direction0 = std::make_shared<Router::Direction>();
        *direction0 = Router::Direction::kLeft;
        new_event0->supporting_information_ = std::static_pointer_cast<void>(direction0);

        DES::Event* new_event1 = new DES::Event(DES::Event::kRouterRoutingAct, 0, std::bind(&LLM::Router::Excute, this->chips_[0]->npus_[1][0]->router_, std::placeholders::_1));
        std::shared_ptr<Router::Direction> direction1 = std::make_shared<Router::Direction>();
        *direction1 = Router::Direction::kLeft;
        new_event1->supporting_information_ = std::static_pointer_cast<void>(direction1);

        // ������Ϣ�������
        simulator->Enqueue(new_event0);
        simulator->Enqueue(new_event1);

        simulator->Simulate();
    }
}