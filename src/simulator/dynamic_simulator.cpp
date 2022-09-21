
//
// Created by ZHOUYC on 2022/6/14.
//
#include "sire/simulator/dynamic_simulator.hpp"
#include <algorithm>
#include <aris.hpp>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace sire {
static std::thread sim_thread_;
static std::mutex sim_mutex_;

static std::array<double, 7 * 16> link_pm{
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
};

static std::array<double, 16> temp{1, 0, 0, 0, 0, 1, 0, 0,
                                   0, 0, 1, 0, 0, 0, 0, 1};

const double ee[4][4]{
    {0.0, 0.0, 1.0, 0.393},
    {0.0, 1.0, 0.0, 0.0},
    {-1.0, 0.0, 0.0, 0.642},
    {0.0, 0.0, 0.0, 1.0},
};

auto config_path = std::filesystem::absolute(".");  //��ȡ��ǰ�������ڵ�·��
const std::string xmlfile = "sire.xml";

struct Simulator::Imp {
  Simulator* simulator_;
  std::thread retrieve_rt_pm;
  Imp(Simulator* simulator) : simulator_(simulator) {}
  Imp(const Imp&) = delete;
};
Simulator::Simulator(const std::string& cs_config_path)
    : cs_(aris::server::ControlServer::instance()),
      cs_config_path_(cs_config_path),
      env_config_path_(cs_config_path) {
  aris::core::fromXmlFile(cs_, cs_config_path_);
  cs_.init();
  std::cout << aris::core::toXmlString(cs_) << std::endl;
  try {
    cs_.start();
    cs_.executeCmd("md");
    cs_.executeCmd("rc");
  } catch (const std::exception& err) {
    std::cout << "����ControlServer�������������ļ�" << std::endl;
  }
}

auto Simulator::instance(const std::string& cs_config_path)
    -> Simulator& {
  static Simulator instance(cs_config_path);
  return instance;
}

Simulator::~Simulator() { }

auto InitSimulator() -> void {
  if (!sim_thread_.joinable()) {
    //����sim_thread_�̣߳�ÿ100ms��ȡģ��linkλ��
    sim_thread_ = std::thread([]() {

    //��ʼ������pumaģ��
        aris::dynamic::PumaParam param;
        param.a1 = 0.040;
        param.a2 = 0.275;
        param.a3 = 0.025;
        param.d1 = 0.342;
        param.d3 = 0.0;
        param.d4 = 0.280;
        param.tool0_pe[2] = 0.073;
        auto m = aris::dynamic::createModelPuma(param);
        auto& cs = aris::server::ControlServer::instance();
        cs.resetModel(m.release());
        cs.resetMaster(
            aris::control::createDefaultEthercatMaster(6, 0,
            0).release());
        cs.resetController(
            aris::control::createDefaultEthercatController(
                6, 0, 0,
                dynamic_cast<aris::control::EthercatMaster&>(cs.master()))
                .release());
        for (int i = 0; i < 6; ++i) {
          cs.controller().motorPool()[i].setMaxPos(3.14);
          cs.controller().motorPool()[i].setMinPos(-3.14);
          cs.controller().motorPool()[i].setMaxVel(3.14);
          cs.controller().motorPool()[i].setMinVel(-3.14);
          cs.controller().motorPool()[i].setMaxAcc(100);
          cs.controller().motorPool()[i].setMinAcc(-100);
        }
        cs.resetPlanRoot(aris::plan::createDefaultPlanRoot().release());

      cs.init();
      // print the control server state
      std::cout << aris::core::toXmlString(cs) << std::endl;
      cs.start();

      //�߳�sim_thread_��getRtDataÿ100ms��ȡlinkλ��
      while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::any data;
        cs.getRtData(
            [](aris::server::ControlServer& cs, const aris::plan::Plan* p,
               std::any& data) -> void {
              auto m = dynamic_cast<aris::dynamic::Model*>(&cs.model());
              //��ȡ�˼�λ��
              for (int i = 1; i < m->partPool().size(); ++i) {
                m->partPool().at(i).getPm(link_pm.data() +
                                          static_cast<long>(16) * i);
              }
              //ת��ĩ��λ��
              aris::dynamic::s_pm_dot_inv_pm(link_pm.data() + 16 * 6, *ee,
                                             temp.data());
              std::copy(temp.begin(), temp.end(), link_pm.data() + 16 * 6);
              data = link_pm;
            },
            data);
        link_pm = std::any_cast<std::array<double, 16 * 7>>(data);
      }
    });
  }
}

auto SimPlan() -> void {
  //���ͷ���켣
  if (sim_thread_.joinable()) {
    auto& cs = aris::server::ControlServer::instance();
    try {
      cs.executeCmd("ds");
      cs.executeCmd("md");
      cs.executeCmd("en");
      cs.executeCmd("mvj --pe={0.393, 0, 0.642, 0, 1.5708, 0}");
      cs.executeCmd("mvj --pe={0.580, 0, 0.642, 0, 1.5708, 0}");
    } catch (std::exception& e) {
      std::cout << "cs:" << e.what() << std::endl;
    }
    return;
  }
}

void DynamicSimulator(std::array<double, 7 * 16>& link_pm_) {
  // guardΪ�ֲ�������������ջ�ϣ����������򼴵�����������
  std::lock_guard<std::mutex> guard(sim_mutex_);
  link_pm_ = link_pm;
}

}  // namespace sire
