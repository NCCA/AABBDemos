#include "MeshWithAABB.h"
#include <ngl/BBox.h>
#include <iostream>
MeshWithAABB::MeshWithAABB( ngl::Obj *_mesh)
{
  m_mesh=_mesh;
  ngl::BBox box=m_mesh->getBBox();
/*  m_defaultExtents[static_cast<int>(Extents::LEFT)].set(box.minX(),0.0f,0.0f,1.0f);
  m_defaultExtents[static_cast<int>(Extents::RIGHT)].set(box.maxX(),0.0f,0.0f,1.0f);
  m_defaultExtents[static_cast<int>(Extents::TOP)].set(0.0f,box.maxY(),0.0f,1.0f);
  m_defaultExtents[static_cast<int>(Extents::BOTTOM)].set(0.0f,box.minY(),0.0f,1.0f);
  m_defaultExtents[static_cast<int>(Extents::FRONT)].set(0.0f,0.0f,box.maxZ(),1.0f);
  m_defaultExtents[static_cast<int>(Extents::BACK)].set(0.0f,0.0f,box.minZ(),1.0f);
  */
  // top
  m_defaultExtents[0].set(box.minX(),box.maxY(),box.maxZ());
  m_defaultExtents[1].set(box.maxX(),box.maxY(),box.maxZ());
  m_defaultExtents[2].set(box.maxX(),box.maxY(),box.minZ());
  m_defaultExtents[3].set(box.minX(),box.maxY(),box.minZ());

  m_defaultExtents[4].set(box.minX(),box.minY(),box.maxZ());
  m_defaultExtents[5].set(box.maxX(),box.minY(),box.maxZ());
  m_defaultExtents[6].set(box.maxX(),box.minY(),box.minZ());
  m_defaultExtents[7].set(box.minX(),box.minY(),box.minZ());

  ngl::Transformation t;
  setTransform(t);

}

void MeshWithAABB::setTransform( ngl::Transformation &_t)
{
  auto m_transformedExtents=m_defaultExtents;
  ngl::Mat4 tx=_t.getMatrix();

  for(auto &v : m_transformedExtents)
  {
    v=v*tx;
  }
  ngl::Real maxX=0.0f;
  ngl::Real minX=0.0f;
  ngl::Real maxY=0.0f;
  ngl::Real minY=0.0f;
  ngl::Real maxZ=0.0f;
  ngl::Real minZ=0.0f;

  for(auto v : m_transformedExtents)
  {
    if     (v.m_x >maxX) { maxX=v.m_x; }
    else if(v.m_x <minX) { minX=v.m_x; }
    if     (v.m_y >maxY) { maxY=v.m_y; }
    else if(v.m_y <minY) { minY=v.m_y; }
    if     (v.m_z >maxZ) { maxZ=v.m_z; }
    else if(v.m_z <minZ) { minZ=v.m_z; }
  }
  m_bbox.reset(new ngl::BBox(minX,maxX,minY,maxY,minZ,maxZ));
}


void MeshWithAABB::draw() const
{
  m_mesh->draw();
}

void MeshWithAABB::drawAABB() const
{
 m_bbox->draw();

}




