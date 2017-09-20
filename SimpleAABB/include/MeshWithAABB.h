#ifndef MESHWITHAABB_H_
#define MESHWITHAABB_H_
#include <array>
#include <memory>
#include <ngl/BBox.h>
#include <ngl/Transformation.h>
#include <ngl/Vec3.h>
#include <ngl/Obj.h>
#include <ngl/AbstractVAO.h>

class MeshWithAABB
{
  public :
    MeshWithAABB( ngl::Obj *_mesh);
    void setTransform( ngl::Transformation &_t);
    void draw() const;
    void drawAABB() const;
    enum class Extents : char {LEFT,RIGHT,TOP,BOTTOM,BACK,FRONT};
  private :
    // this is the untransformed extents of the mesh (initial BBox)
    std::array<ngl::Vec4,8> m_defaultExtents;
    // the actual mesh used for drawing etc
    ngl::Obj *m_mesh;
    // bounding box the size of the AABB
    std::shared_ptr<ngl::BBox> m_bbox;
    // calculate the extents of the tx and create bbox;
    void getExtents();
    void setBBox();
 //   ngl::AbstractVAO *m_vao=nullptr;

};


#endif
