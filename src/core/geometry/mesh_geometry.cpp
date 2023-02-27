#include "sire/core/geometry/mesh_geometry.hpp"

#include <string>

#include <aris/core/reflection.hpp>
#include <aris/dynamic/model.hpp>
#include <aris/server/control_server.hpp>

namespace sire::geometry {
SIRE_DEFINE_TO_JSON_HEAD(MeshGeometry) {
  GeometryOnPart::to_json(j);
  j["shape_type"] = shapeType();
  j["resource_path"] = resourcePath();
}

MeshGeometry::MeshGeometry(string resource_path)
    : GeometryOnPart(), MeshShape(resource_path) {}

MeshGeometry::~MeshGeometry() = default;

ARIS_DEFINE_BIG_FOUR_CPP(MeshGeometry)

// �������ڲ���from_json to_json���壬
// ʹ�ú궨���������json����ת����from_json to_json�ķ�������
SIRE_DEFINE_JSON_OUTER_TWO(MeshGeometry)

ARIS_REGISTRATION {
  aris::core::class_<MeshGeometry>("MeshGeometry")
      .inherit<GeometryOnPart>()
      .inherit<MeshShape>();
}
}  // namespace sire::geometry