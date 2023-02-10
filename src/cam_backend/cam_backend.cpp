#include "sire/cam_backend/cam_backend.hpp"

#include "sire/collision/collision_calculator.hpp"
#include "sire/collision/collision_filter.hpp"

#include <aris/core/serialization.hpp>
#include <aris/dynamic/model.hpp>

#include <hpp/fcl/broadphase/broadphase_dynamic_AABB_tree.h>
#include <hpp/fcl/distance.h>
#include <hpp/fcl/math/transform.h>
#include <hpp/fcl/mesh_loader/assimp.h>
#include <hpp/fcl/mesh_loader/loader.h>
#include <hpp/fcl/shape/geometric_shapes.h>

#include <cmath>

namespace sire::cam_backend {
struct CamBackend::Imp {
  vector<bool> collision_result;
  vector<set<std::pair<collision::geometry::GeometryId,
                       collision::geometry::GeometryId>>>
      collided_objects_result;

  aris::dynamic::Model robot_model;
  collision::CollisionCalculator collision_calculator;
};

void CamBackend::cptCollisionByEEPose(
    double* ee_pe, collision::CollidedObjectsCallback& callback) {
  // aris generalMotionĩ��Ĭ�ϱ�ʾ��Euler321 ZYX
  imp_->robot_model.setOutputPos(ee_pe);
  if (imp_->robot_model.inverseKinematics()) return;
  imp_->robot_model.forwardKinematics();
  aris::Size partSize = imp_->robot_model.partPool().size();
  vector<double> part_pq(partSize * 7);
  for (aris::Size i = 0; i < partSize; ++i) {
    imp_->robot_model.partPool().at(i).getPq(part_pq.data() + i * 7);
  }
  imp_->collision_calculator.updateLocation(part_pq.data());
  imp_->collision_calculator.hasCollisions(callback);
}

void CamBackend::init(string model_xml_path, string collision_xml_path) {
  auto config_path = std::filesystem::absolute(".");  // ��ȡ��ǰ�������ڵ�·��
  const string model_config_name = "cam_model.xml";
  auto model_config_path = config_path / model_config_name;
  aris::core::fromXmlFile(imp_->robot_model, model_config_path);

  const string collision_config_name = "collision_calculator.xml";
  auto collision_config_path = config_path / collision_config_name;
  aris::core::fromXmlFile(imp_->collision_calculator, collision_config_path);
  imp_->robot_model.init();
  imp_->collision_calculator.init();
}
/*
 * ���ڵ����⣺
 * �����е�ۻ��м��������᣿������������ô��ʾ��ô���㣿
 *
 * TODO:
 * 1. �ⲿ��
 * 2. �ж�Z�Ƿ������һ���˶��غ�
 * 3. Option��ô������
 *
 *  tool_z_vec�ڲ����ò����ʱ����normalһ�£��������ò���֮��Ͳ�һ���ˣ���Ҫ�����AxisA6����תʹ��
 *  ��λ m
 */
void CamBackend::cptCollisionMap(int option, aris::Size resolution,
                                 aris::Size pSize, double* points,
                                 double* tilt_angles, double* forward_angles,
                                 double* normal, double* tangent) {
  imp_->collision_result.resize(resolution * pSize);
  imp_->collided_objects_result.resize(resolution * pSize);
  // AxisA6
  if (option == 0) {
    auto& gm = imp_->robot_model.generalMotionPool().at(0);
    auto& marker = imp_->robot_model.partPool().back().markerPool().at(0);
    double step_angle = 360.0 / resolution;
    for (aris::Size i = 0; i < resolution; ++i) {
      for (aris::Size j = 0; j < pSize; ++j) {
        // 1. �ӹ�������ϵ( normal tangent)
        double* point = points + j * 3;
        double* forward_vec = tangent + j * 3;
        double* normal_vec = normal + j * 3;
        double forward_angle = forward_angles[j];
        double tilt_angle = tilt_angles[j];
        double y_vec[3] = {
            normal_vec[1] * forward_vec[2] - forward_vec[1] * normal_vec[2],
            -normal_vec[0] * forward_vec[2] + forward_vec[0] * normal_vec[2],
            normal_vec[0] * forward_vec[1] - forward_vec[0] * normal_vec[1]};
        double tool_point_pm[16] = {forward_vec[0],
                                    y_vec[0],
                                    normal_vec[0],
                                    point[0],
                                    forward_vec[1],
                                    y_vec[1],
                                    normal_vec[1],
                                    point[1],
                                    forward_vec[2],
                                    y_vec[2],
                                    normal_vec[2],
                                    point[2],
                                    0.0,
                                    0.0,
                                    0.0,
                                    1.0};
        // 2. ����ǰ�������ת�ӹ�������ϵ��normal tangent)
        double tilt_forward_re[3]{0.0, forward_angle / 180.0 * aris::PI,
                                  tilt_angle / 180.0 * aris::PI};  // 321
        double tilt_forward_pm[16];
        aris::dynamic::s_re2pm(tilt_forward_re, tilt_forward_pm, "321");
        double tool_pm[16];
        aris::dynamic::s_pm_dot_pm(tool_point_pm, tilt_forward_pm, tool_pm);
        // 3. �õ����߿ռ�λ��
        tool_pm[1] = -tool_pm[1];
        tool_pm[2] = -tool_pm[2];
        tool_pm[5] = -tool_pm[5];
        tool_pm[6] = -tool_pm[6];
        tool_pm[8] = -tool_pm[8];
        tool_pm[9] = -tool_pm[9];
        // 4. ���ݵ���z�������ת����
        double re[3]{aris::PI * (-1 + i * step_angle / 180), 0.0, 0.0};  // 313
        double rotate_tool_z_pm[16];
        aris::dynamic::s_re2pm(re, rotate_tool_z_pm);
        double target_tool_pm[16];
        aris::dynamic::s_pm_dot_pm(tool_pm, rotate_tool_z_pm, target_tool_pm);
        double target_tool_pe[6];
        aris::dynamic::s_pm2pe(target_tool_pm, target_tool_pe, "321");
        gm.setP(target_tool_pe);
        // 5. ����ĩ��λ�˲�����
        if (imp_->robot_model.solverPool().at(0).kinPos()) return;
        imp_->robot_model.forwardKinematics();
        aris::Size partSize = imp_->robot_model.partPool().size();
        vector<double> part_pq(partSize * 7);
        for (aris::Size i = 0; i < partSize; ++i) {
          imp_->robot_model.partPool().at(i).getPq(part_pq.data() + i * 7);
        }
        collision::CollidedObjectsCallback callback(
            &imp_->collision_calculator.collisionFilter());
        imp_->collision_calculator.updateLocation(part_pq.data());
        imp_->collision_calculator.hasCollisions(callback);
        if (callback.collidedObjectMap().size() != 0) {
          imp_->collision_result[i * pSize + j] = true;
          imp_->collided_objects_result[i * pSize + j] =
              callback.collidedObjectMap();
        }
      }
    }
  }
  // ����:
  // ����: ��ֱ�ڹ����˶���ƽ���еĸ�����б��
  else if (option == 1) {
    auto& gm = imp_->robot_model.generalMotionPool().at(0);
    double step_angle = 180.0 / resolution;
    for (aris::Size i = 0; i < resolution; ++i) {
      for (aris::Size j = 0; j < pSize; ++j) {
        // 1. �ӹ�������ϵ( normal tangent)
        double* point = points + j * 3;
        double* forward_vec = tangent + j * 3;
        double* normal_vec = normal + j * 3;
        double forward_angle = forward_angles[j];
        double tilt_angle = tilt_angles[j];
        double y_vec[3] = {
            normal_vec[1] * forward_vec[2] - forward_vec[1] * normal_vec[2],
            -normal_vec[0] * forward_vec[2] + forward_vec[0] * normal_vec[2],
            normal_vec[0] * forward_vec[1] - forward_vec[0] * normal_vec[1]};
        double tool_point_pm[16] = {forward_vec[0],
                                    y_vec[0],
                                    normal_vec[0],
                                    point[0],
                                    forward_vec[1],
                                    y_vec[1],
                                    normal_vec[1],
                                    point[1],
                                    forward_vec[2],
                                    y_vec[2],
                                    normal_vec[2],
                                    point[2],
                                    0.0,
                                    0.0,
                                    0.0,
                                    1.0};
        // 2. ����ǰ����ת�ӹ�������ϵ��normal tangent)
        double forward_re[3]{0.0, forward_angle / 180.0 * aris::PI,
                             0.0};  // 321
        double forward_pm[16];
        aris::dynamic::s_re2pm(forward_re, forward_pm, "321");
        double tool_pm[16];
        aris::dynamic::s_pm_dot_pm(tool_point_pm, forward_pm, tool_pm);
        // 3. �õ����߿ռ�λ��
        tool_pm[1] = -tool_pm[1];
        tool_pm[2] = -tool_pm[2];
        tool_pm[5] = -tool_pm[5];
        tool_pm[6] = -tool_pm[6];
        tool_pm[8] = -tool_pm[8];
        tool_pm[9] = -tool_pm[9];
        // 4. ���ݵ���ǰ������x�������ת����
        double theta = (step_angle * i - 1) * aris::PI / 2;
        double tilt_angle_pm[16]{1.0,
                                 0.0,
                                 0.0,
                                 0.0,
                                 0.0,
                                 std::cos(theta),
                                 -std::sin(theta),
                                 0.0,
                                 0.0,
                                 std::sin(theta),
                                 std::cos(theta),
                                 0.0,
                                 0.0,
                                 0.0,
                                 0.0,
                                 1.0};

        double target_tool_pm[16];
        aris::dynamic::s_pm_dot_pm(tool_pm, tilt_angle_pm, target_tool_pm);
        double target_tool_pe[6];
        aris::dynamic::s_pm2pe(target_tool_pm, target_tool_pe, "321");
        // 5. ����ĩ��λ�˲�����
        gm.setP(target_tool_pe);
        if (imp_->robot_model.solverPool().at(0).kinPos()) return;
        imp_->robot_model.forwardKinematics();
        aris::Size partSize = imp_->robot_model.partPool().size();
        vector<double> part_pq(partSize * 7);
        for (aris::Size i = 0; i < partSize; ++i) {
          imp_->robot_model.partPool().at(i).getPq(part_pq.data() + i * 7);
        }
        collision::CollidedObjectsCallback callback(
            &imp_->collision_calculator.collisionFilter());
        imp_->collision_calculator.updateLocation(part_pq.data());
        imp_->collision_calculator.hasCollisions(callback);
        if (callback.collidedObjectMap().size() != 0) {
          imp_->collision_result[i * pSize + j] = true;
          imp_->collided_objects_result[i * pSize + j] =
              callback.collidedObjectMap();
        }
      }
    }
  }
  return;
}

CamBackend::CamBackend() : imp_(new Imp) {}
CamBackend::~CamBackend(){};

}  // namespace sire::cam_backend