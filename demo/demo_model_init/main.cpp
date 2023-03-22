#include <filesystem>
#include <iostream>

#include <aris.hpp>

auto xmlpath = std::filesystem::absolute(".");  // ��ȡ��ǰ�������ڵ�·��
const std::string xmlfile = "kuka.xml";

int main(int argc, char* argv[]) {
  aris::dynamic::MultiModel model;
  xmlpath = xmlpath / xmlfile;
  aris::core::fromXmlFile(model, xmlpath);
  model.init();
  std::cout << aris::core::toXmlString(model) << std::endl;
  std::cin.get();
  return 0;
}