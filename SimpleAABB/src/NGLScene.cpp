#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Transformation.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>


//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the offset for full window mode mouse data
//----------------------------------------------------------------------------------------------------------------------
constexpr static int FULLOFFSET=4;

NGLScene::NGLScene()
{
  for(auto &panel : m_panelMouseInfo)
  {
    panel.m_spinXFace=0;
    panel.m_spinYFace=0;
    panel.m_rotate=false;
  }

  // now we need to set the scales for the ortho windos
  m_panelMouseInfo[static_cast<size_t>(Window::FRONT)].m_modelPos.set(0,0,1);
  m_panelMouseInfo[static_cast<size_t>(Window::SIDE)].m_modelPos.set(0,0,1);
  m_panelMouseInfo[static_cast<size_t>(Window::TOP)].m_modelPos.set(0,0,1);


  // mouse rotation values set to 0
  setTitle("Simple AABB Demo");
  m_width=this->size().width();
  m_height=this->size().height();
  std::cout<<m_width<<" "<<m_height<<"\n";

}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}



void NGLScene::resizeGL(int _w , int _h)
{
  // take into account device ratio later in the resize to be safe
  m_width=_w;
  m_height=_h;
}

void NGLScene::initializeGL()
{
  // we need to initialise the NGL lib which will load all of the OpenGL functions, this must
  // be done once we have a valid GL context but before we call any GL commands. If we dont do
  // this everything will crash
  ngl::NGLInit::instance();
  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  shader->setUniform("Colour",1.0f,1.0f,1.0f,1.0f);
  shader->setUniform("lightPos",1.0f,1.0f,1.0f);
  shader->setUniform("lightDiffuse",1.0f,1.0f,1.0f,1.0f);
  // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();
  prim->createLineGrid("grid",40.0f,40.0f,40.0f);
  // as re-size is not explicitly called we need to do this.
  glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
  // load a frag and vert shaders

  shader->createShaderProgram("TextureShader");

  shader->attachShader("TextureVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("TextureFragment",ngl::ShaderType::FRAGMENT);
  shader->loadShaderSource("TextureVertex","shaders/TextureVertex.glsl");
  shader->loadShaderSource("TextureFragment","shaders/TextureFragment.glsl");

  shader->compileShader("TextureVertex");
  shader->compileShader("TextureFragment");
  shader->attachShaderToProgram("TextureShader","TextureVertex");
  shader->attachShaderToProgram("TextureShader","TextureFragment");

  // link the shader no attributes are bound
  shader->linkProgramObject("TextureShader");
  (*shader)["TextureShader"]->use();



  (*shader)["nglColourShader"]->use();

  shader->setUniform("Colour",1.0f,1.0f,1.0f,1.0f);

  // first we create a mesh from an obj passing in the obj file and texture
  m_mesh.reset(  new ngl::Obj("models/Helix.obj","textures/helix_base.tif"));
  // now we need to create this as a VAO so we can draw it
  m_mesh->createVAO();
  m_mesh->calcBoundingSphere();
  m_meshAABB.reset(new MeshWithAABB(m_mesh.get()));
  startTimer(10);

}

void NGLScene::frameActive()
{
  size_t win = static_cast<size_t>(getActiveQuadrant());
  if(m_activeWindow != Window::ALL)
  {
    win= FULLOFFSET;
  }
  m_panelMouseInfo[win].m_modelPos.set(0,0,1);
}

void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("nglDiffuseShader");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_globalTransform.getMatrix();
  MV=  m_globalTransform.getMatrix()*m_view;
  MVP= M*m_view*m_projection;
  normalMatrix=MV;
  normalMatrix.inverse();
  shader->setUniform("MVP",MVP);
  shader->setUniform("normalMatrix",normalMatrix);
 }

void NGLScene::loadMatricesToTextureShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  shader->use("TextureShader");
  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat4 M;
  M=m_globalTransform.getMatrix();
  MV=  m_globalTransform.getMatrix()*m_view;
  MVP= m_transform.getMatrix()*M*m_view*m_projection;
  shader->setUniform("MVP",MVP);
 }

void NGLScene::top(Mode _m)
{
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  // Rotation based on the mouse position for our global transform
  auto win=FULLOFFSET;
  if(_m == Mode::PANEL)
  {
    win=static_cast<size_t>(Window::TOP);
  }


  // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();

  // first draw a top  persp // front //side
  ngl::Vec3 from(0,5,0);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,0,-1);
  m_globalTransform.reset();
  {
    /// a top view (left upper)
    m_view=ngl::lookAt(from,to,up);
    m_projection=ngl::ortho(-5,5,-5,5, 0.1f, 500.0f);
    // x,y w/h
    if(_m==Mode::PANEL)

    {
      glViewport (0,(m_height/2)*devicePixelRatio(), (m_width/2)*devicePixelRatio(), (m_height/2)*devicePixelRatio());
    }
    else
    {
      glViewport(0,0,m_width*devicePixelRatio(),m_height*devicePixelRatio());
    }
      // draw
    // in this case set to an identity
    ngl::Vec3 p=m_panelMouseInfo[win].m_modelPos;
    m_globalTransform.setPosition(p.m_x,0,-p.m_y);
    m_globalTransform.setScale(p.m_z,p.m_z,p.m_z);

    loadMatricesToTextureShader();
    m_meshAABB->draw();
    // draw the mesh bounding box
    (*shader)["nglColourShader"]->use();
    //loadMatricesToShader();
    shader->setUniform("MVP",m_view*m_projection);
    shader->setUniform("Colour",0.0f,0.0f,1.0f,1.0f);
    m_meshAABB->drawAABB();

    m_globalTransform.addPosition(0,-1,0);
    loadMatricesToShader();
    prim->draw("grid");
  }

}

void NGLScene::side(Mode _m)
{
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  // Rotation based on the mouse position for our global transform
  int win=FULLOFFSET;
  if(_m == Mode::PANEL)
  {
    win=static_cast<size_t>(Window::SIDE);
  }

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();

  // first draw a top  persp // front //side
  ngl::Vec3 from(0,5,0);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,0,-1);
  /// a side view (bottom right)
  m_globalTransform.reset();
  {
    from.set(5,0,0);
    up.set(0,1,0);
    m_view=ngl::lookAt(from,to,up);
    m_projection=ngl::ortho(-5,5,-5,5, 0.1f, 100.0f);
    // x,y w/h
    if(_m==Mode::PANEL)
    {
      glViewport ((m_width/2)*devicePixelRatio(), 0, (m_width/2)*devicePixelRatio(), (m_height/2)*devicePixelRatio());
    }
    else
    {
      glViewport(0,0,m_width*devicePixelRatio(),m_height*devicePixelRatio());
    }

    // draw
    ngl::Vec3 p=m_panelMouseInfo[win].m_modelPos;
    m_globalTransform.setPosition(0,p.m_y,-p.m_x);
    m_globalTransform.setScale(p.m_z,p.m_z,p.m_z);


    loadMatricesToTextureShader();
    m_meshAABB->draw();
    // draw the mesh bounding box
    (*shader)["nglColourShader"]->use();
    //loadMatricesToShader();
    shader->setUniform("MVP",m_view*m_projection);
    shader->setUniform("Colour",0.0f,0.0f,1.0f,1.0f);
    m_meshAABB->drawAABB();

    m_globalTransform.setRotation(90,90,0);
    m_globalTransform.addPosition(0,0,2);
    loadMatricesToShader();
    prim->draw("grid");
  }



}
void NGLScene::persp(Mode _m)
{
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  // Rotation based on the mouse position for our global transform
  // 4 is the panel full screen mode
  size_t win=FULLOFFSET;
  if(_m == Mode::PANEL)
  {
    win=static_cast<size_t>(Window::PERSP);
  }
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_panelMouseInfo[win].m_spinXFace);
  rotY.rotateY(m_panelMouseInfo[win].m_spinYFace);
  // multiply the rotations
  ngl::Mat4 final=rotY*rotX;
  // add the translations
  final.m_m[3][0] = m_panelMouseInfo[win].m_modelPos.m_x;
  final.m_m[3][1] = m_panelMouseInfo[win].m_modelPos.m_y;
  final.m_m[3][2] = m_panelMouseInfo[win].m_modelPos.m_z;
  // set this in the TX stack

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();

  // first draw a top  persp // front //side
  ngl::Vec3 from(0,5,0);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,0,-1);
  m_globalTransform.reset();
  {
    /// a perspective view (right upper)
    //first set the global mouse rotation for this one
    m_globalTransform.setMatrix(final);
    from.set(0,5,5);
    up.set(0,1,0);
    m_view=ngl::lookAt(from,to,up);
    m_projection=ngl::perspective(45,float(m_width/m_height),0.01,100);
    // x,y w/h
    if(_m==Mode::PANEL)
    {
      glViewport ((m_width/2)*devicePixelRatio(), (m_height/2)*devicePixelRatio(), (m_width/2)*devicePixelRatio(), (m_height/2)*devicePixelRatio());
    }
    else
    {
      glViewport(0,0,m_width*devicePixelRatio(),m_height*devicePixelRatio());
    }

    // draw
    loadMatricesToTextureShader();
    m_meshAABB->draw();
    // draw the mesh bounding box
    (*shader)["nglColourShader"]->use();
    shader->setUniform("MVP",m_view*m_projection);
    shader->setUniform("Colour",0.0f,0.0f,1.0f,1.0f);
    loadMatricesToShader();
    m_meshAABB->drawAABB();

    // now we need to add an offset to the y position to draw the grid,
    // easiest way is to just modify the matrix directly
    final.m_m[3][1]-=0.8f;
    m_globalTransform.setMatrix(final);

    //m_globalTransform.setPosition(0,-0.55,0);
    loadMatricesToShader();
    prim->draw("grid");
  }

}
void NGLScene::front(Mode _m)
{
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["nglDiffuseShader"]->use();

  size_t win=FULLOFFSET;
  if(_m == Mode::PANEL)
  {
    win=static_cast<size_t>(Window::FRONT);
  }

   // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives *prim=ngl::VAOPrimitives::instance();

  // first draw a top  persp // front //side
  ngl::Vec3 from(0,5,0);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,0,-1);
  /// a front view (bottom left)
  m_globalTransform.reset();
  {
    from.set(0,0,5);
    up.set(0,1,0);
    m_view=ngl::lookAt(from,to,up);
    m_projection=ngl::ortho(-5,5,-5,5, 0.01f, 200.0f);
    // x,y w/h
    if(_m==Mode::PANEL)
    {
      glViewport (0,0, (m_width/2)*devicePixelRatio(), (m_height/2)*devicePixelRatio());
    }
    else
    {
      glViewport(0,0,m_width*devicePixelRatio(),m_height*devicePixelRatio());
    }
    // draw
    // in this case set to an identity
    ngl::Vec3 p=m_panelMouseInfo[win].m_modelPos;
    m_globalTransform.setPosition(p.m_x,p.m_y,0);
    m_globalTransform.setScale(p.m_z,p.m_z,p.m_z);
    loadMatricesToTextureShader();
    m_meshAABB->draw();
    // draw the mesh bounding box
    (*shader)["nglColourShader"]->use();
    //loadMatricesToShader();
    shader->setUniform("MVP",m_view*m_projection);
    shader->setUniform("Colour",0.0f,0.0f,1.0f,1.0f);
    m_meshAABB->drawAABB();
    m_globalTransform.setRotation(90,0,0);
    m_globalTransform.addPosition(0,0,-1);
    loadMatricesToShader();
    prim->draw("grid");
  }

}


NGLScene::Window NGLScene::getActiveQuadrant() const
{
  // find where the  mouse is and set quadrant
  if( (m_mouseX <m_width/2) && (m_mouseY<m_height/2) )
  {
    return Window::TOP;
  }
  else if( (m_mouseX >=m_width/2) && (m_mouseY<m_height/2) )
  {
    return Window::PERSP;
  }
  else if( (m_mouseX <=m_width/2) && (m_mouseY>m_height/2) )
  {
    return Window::FRONT;
  }
  else if( (m_mouseX >=m_width/2) && (m_mouseY>m_height/2) )
  {
    return Window::SIDE;
  }
  else // should never get here but stops warning
  {
    return Window::ALL;
  }
}


void NGLScene::toggleWindow()
{
  if(m_activeWindow ==Window::ALL)
  {
    m_activeWindow=getActiveQuadrant();
    //store mouse info as we have gone fullscreen
    m_panelMouseInfo[FULLOFFSET].m_modelPos = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_modelPos;
    m_panelMouseInfo[FULLOFFSET].m_origX = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origX;
    m_panelMouseInfo[FULLOFFSET].m_origY = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origY;

    m_panelMouseInfo[FULLOFFSET].m_spinXFace = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_spinXFace;
    m_panelMouseInfo[FULLOFFSET].m_spinYFace = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_spinYFace;
    m_panelMouseInfo[FULLOFFSET].m_origXPos = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origXPos;
    m_panelMouseInfo[FULLOFFSET].m_origYPos = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origYPos;
    m_panelMouseInfo[FULLOFFSET].m_rotate = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_rotate;
    m_panelMouseInfo[FULLOFFSET].m_translate = m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_translate;
  }
  else
  {
    // store info as we are minimised
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_modelPos = m_panelMouseInfo[FULLOFFSET].m_modelPos;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origX = m_panelMouseInfo[FULLOFFSET].m_origX;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origY = m_panelMouseInfo[FULLOFFSET].m_origY;

    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_spinXFace = m_panelMouseInfo[FULLOFFSET].m_spinXFace;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_spinYFace = m_panelMouseInfo[FULLOFFSET].m_spinYFace;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origXPos = m_panelMouseInfo[FULLOFFSET].m_origXPos;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_origYPos = m_panelMouseInfo[FULLOFFSET].m_origYPos;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_rotate = m_panelMouseInfo[FULLOFFSET].m_rotate;
    m_panelMouseInfo[static_cast<size_t>(m_activeWindow)].m_translate = m_panelMouseInfo[FULLOFFSET].m_translate;
    m_activeWindow=Window::ALL;
  }

}


void NGLScene::paintGL()
{
  // clear the screen and depth buffer
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   switch(m_activeWindow)
   {
     case Window::ALL :
     {
       front(Mode::PANEL);
       side(Mode::PANEL);
       top(Mode::PANEL);
       persp(Mode::PANEL);
       break;
     }
     case Window::FRONT : {front(Mode::FULLSCREEN); break; }
     case Window::SIDE : {side(Mode::FULLSCREEN); break; }
     case Window::TOP : {top(Mode::FULLSCREEN); break; }
     case Window::PERSP : {persp(Mode::FULLSCREEN); break; }

   }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  m_mouseX=_event->x();
  m_mouseY=_event->y();
  size_t win;
  if(m_activeWindow ==Window::ALL)
    win=static_cast<size_t>(getActiveQuadrant());
  else
    win=FULLOFFSET;
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_panelMouseInfo[win].m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_panelMouseInfo[win].m_origX;
    int diffy=_event->y()-m_panelMouseInfo[win].m_origY;
    m_panelMouseInfo[win].m_spinXFace += (float) 0.5f * diffy;
    m_panelMouseInfo[win].m_spinYFace += (float) 0.5f * diffx;
    m_panelMouseInfo[win].m_origX = _event->x();
    m_panelMouseInfo[win].m_origY = _event->y();
    update();

	}
	// right mouse translate code
	else if(m_panelMouseInfo[win].m_translate && _event->buttons() == Qt::RightButton)
	{
		int diffX = (int)(_event->x() - m_panelMouseInfo[win].m_origXPos);
		int diffY = (int)(_event->y() - m_panelMouseInfo[win].m_origYPos);
		m_panelMouseInfo[win].m_origXPos=_event->x();
		m_panelMouseInfo[win].m_origYPos=_event->y();
		m_panelMouseInfo[win].m_modelPos.m_x += INCREMENT * diffX;
		m_panelMouseInfo[win].m_modelPos.m_y -= INCREMENT * diffY;
		update();

	}
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  size_t win;
	if(m_activeWindow ==Window::ALL)
    win=static_cast<size_t>(getActiveQuadrant());
	else
		win=FULLOFFSET;

  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_panelMouseInfo[win].m_origX = _event->x();
    m_panelMouseInfo[win].m_origY = _event->y();
    m_panelMouseInfo[win].m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_panelMouseInfo[win].m_origXPos = _event->x();
    m_panelMouseInfo[win].m_origYPos = _event->y();
    m_panelMouseInfo[win].m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  size_t win;
  if(m_activeWindow ==Window::ALL)
    win=static_cast<size_t>(getActiveQuadrant());
  else
    win=FULLOFFSET;
  if (_event->button() == Qt::LeftButton)
  {
    m_panelMouseInfo[win].m_rotate=false;
  }
  // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_panelMouseInfo[win].m_translate=false;
  }
 }

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

  size_t win;
	if(m_activeWindow ==Window::ALL)
    win=static_cast<size_t>(getActiveQuadrant());
	else
		win=FULLOFFSET;
	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_panelMouseInfo[win].m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_panelMouseInfo[win].m_modelPos.m_z-=ZOOM;
	}
	update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the NGLScene
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : frameActive(); break;
  // show windowed
  case Qt::Key_Space : toggleWindow(); break;
  case Qt::Key_0 : m_active^=true; break;
  case Qt::Key_1 : m_rotMode=RotMode::XROT; break;
  case Qt::Key_2 : m_rotMode=RotMode::YROT; break;
  case Qt::Key_3 : m_rotMode=RotMode::ZROT; break;
  case Qt::Key_4 : m_rotMode=RotMode::ALL; break;

  default : break;
  }
    update();
}

void NGLScene::timerEvent(QTimerEvent *)
{
  // draw the mesh
  static float rotXX=0.0f;
  if(!m_active)
    return;
  ++rotXX;
  switch(m_rotMode )
  {
  case RotMode::XROT :
    m_transform.setRotation(rotXX,0,0);
  break;
  case RotMode::YROT :
    m_transform.setRotation(0,rotXX,0);
  break;
  case RotMode::ZROT :
    m_transform.setRotation(0,0,rotXX);
  break;
  case RotMode::ALL :
    m_transform.setRotation(rotXX,rotXX,rotXX);
  break;
  }
  m_meshAABB->setTransform(m_transform);
  update();
}



