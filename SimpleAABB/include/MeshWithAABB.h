#ifndef MESHWITHAABB_H__
#define MESHWITHAABB_H__
#include <array>
#include <memory>
#include <ngl/BBox.h>
#include <ngl/Transformation.h>
#include <ngl/Vec3.h>
#include <ngl/Obj.h>


class MeshWithAABB
{
  public :
    MeshWithAABB(std::shared_ptr<ngl::Obj> _mesh);
    void setTransform( ngl::Transformation &_t);
    void draw() const;
    void drawAABB() const;
    enum class Extents : char {LEFT,RIGHT,TOP,BOTTOM,BACK,FRONT};
  private :
    // this is the untransformed extents of the mesh (initial BBox)
    std::array<ngl::Vec4,6> m_defaultExtents;
    // this is the untransformed extents of the mesh (initial BBox)
    std::array<ngl::Vec4,6> m_transformedExtents;
    // the actual mesh used for drawing etc
    std::shared_ptr<ngl::Obj> m_mesh;
    // bounding box the size of the AABB
    std::shared_ptr<ngl::BBox> m_bbox;
    // calculate the extents of the tx and create bbox;
    void getExtents();

};


#endif
