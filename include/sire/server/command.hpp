#ifndef COMMAND_H_
#define COMMAND_H_

#include <aris.hpp>

namespace sire::server {
class Get : public aris::core::CloneObject<Get, aris::plan::Plan> {
 public:
  auto virtual prepareNrt() -> void;
  auto virtual collectNrt() -> void;
  explicit Get(const std::string& name = "Get_plan");
};

class GetForce : public aris::core::CloneObject<GetForce, aris::plan::Plan> {
 public:
  auto virtual prepareNrt() -> void;
  auto virtual collectNrt() -> void;
  explicit GetForce(const std::string& name = "GetForce_plan");
};

	/// \brief �������˴���ռ��ƶ���ĳ��λ�ˡ�
///
///
/// ### �������� ###
///
/// ָ��Ŀ��λ�ˣ�����ʹ�����²�����
/// + λ������Ԫ��������λ�� xyz Ϊ[0 , 0.5 , 1.1]����̬Ϊԭʼ��̬[0 , 0 , 0 ,
/// 1]����mvj --pe={0,0.5,1.1,0,0,0,1}��
/// + λ�˾���λ����Ȼ���ϣ���mvj --pm={1,0,0,0,0,1,0,0.5,0,0,1,1.1,0,0,0,1}��
/// + λ����ŷ���ǣ�λ����̬��Ȼ���ϣ���mvj --pe={0,0.5,1.1,0,0,0}��
///
/// ���⣬������ָ��λ�úͽǶȵĵ�λ�����ȵ�λĬ��Ϊ m ���Ƕ�Ĭ��Ϊ rad ��
/// + ָ��λ�õ�λ�����罫��λ����Ϊ m ���ף�����mvj --pq={0,0.5,1.1,0,0,0,1}
/// --pos_unit=m��
/// + ָ���Ƕȵ�λ�����罫��λ����Ϊ rad ����mvj --pe={0,0.5,1.1,0,0,0}
/// --ori_unit=rad��
///
/// ������ָ��ŷ���ǵ����࣬������ 321 313 123 212 ...
/// ���������͵�ŷ���ǣ�Ĭ��Ϊ 321 ��ŷ����
/// + ָ��ŷ����Ϊ 123 �ģ���mvj --pe={0,0.5,1.1,0,0,0} --ori_unit=rad
/// --eul_type=321��
///
/// ָ���ؽ��ٶȣ���λһ���� m/s �� rad/s ��Ӧ��ԶΪ������Ĭ��Ϊ0.1
/// + ָ�����е�����ٶȶ�Ϊ0.5����mvj --pe={0,0.5,1.1,0,0,0} --joint_vel=0.5��
/// + ָ�����е�����ٶȣ���mvj --pe={0,0.5,1.1,0,0,0}
/// --joint_vel={0.2,0.2,0.2,0.3,0.3,0.3}��
///
/// ָ���ؽڼ��ٶȣ���λһ���� m/s^2 �� rad/s^2 ��Ӧ��ԶΪ������Ĭ��Ϊ0.1
/// + ָ�����е���ļ��ٶȶ�Ϊ0.3����mvj --pe={0,0.5,1.1,0,0,0} --joint_vel=0.5
/// --joint_acc=0.3��
/// + ָ�����е���ļ��ٶȣ���mvj --pe={0,0.5,1.1,0,0,0} --joint_vel=0.5
/// --joint_acc={0.2,0.2,0.2,0.3,0.3,0.3}��
///
/// ָ���ؽڼ��ٶȣ���λһ���� m/s^2 �� rad/s^2 ��Ӧ��ԶΪ������Ĭ��Ϊ0.1
/// + ָ�����е���ļ��ٶȶ�Ϊ0.3����mvj --pe={0,0.5,1.1,0,0,0} --joint_vel=0.5
/// --joint_dec=0.3��
/// + ָ�����е���ļ��ٶȣ���mvj --pe={0,0.5,1.1,0,0,0} --joint_vel=0.5
/// --joint_dec={0.2,0.2,0.2,0.3,0.3,0.3}��
///
class SireMoveJ
    : public aris::core::CloneObject<SireMoveJ, aris::plan::Plan> {
 public:
  auto virtual prepareNrt() -> void override;
  auto virtual executeRT() -> int override;

  virtual ~SireMoveJ();
  explicit SireMoveJ(const std::string& name = "move_j");
  ARIS_DECLARE_BIG_FOUR(SireMoveJ);

 private:
  struct Imp;
  aris::core::ImpPtr<Imp> imp_;
};
}
#endif