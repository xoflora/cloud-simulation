#include "view.h"

#include <iostream>
#include <QFileDialog>
#include <QGLFramebufferObject>
#include <QApplication>
#include <QKeyEvent>
#include <QList>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <QGLShader>
#include <iostream>
#include <numeric>

#define dimX 75
#define dimY 60
#define dimZ 75
#define SQUARE_SIZE 200
#define SQUARE_DISTRIBUTION 40

using namespace std;
class QGLShaderProgram;

View::View(QWidget *parent) : QGLWidget(parent), m_font("Verdana", 8, 4)
{
    // View needs all move events, not just mouse drag events
    setMouseTracking(true);

    // Hide the cursor since this is a fullscreen app
    //  setCursor(Qt::BlankCursor);

    // View needs keyboard focus
    setFocusPolicy(Qt::StrongFocus);

    m_camera.center = Vector3(0.f, 0.f, 0.f);
    m_camera.up = Vector3(0.f, 1.f, 0.f);
    m_camera.zoom = 3.5f;
    m_camera.theta = M_PI * 1.5f, m_camera.phi = 0.2f;
    m_camera.fovy = 60.f;

    // The game loop is implemented using a timer
    connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));

    m_cloudgen = new CloudGenerator();
    m_clouds = m_cloudgen->calcIntensity(dimX, dimY, dimZ);

}

View::~View()
{
    gluDeleteQuadric(m_quadric);
    delete(m_cloudgen);
}

void View::initializeGL()
{
    // All OpenGL initialization *MUST* be done during or after this
    // method. Before this method is called, there is no active OpenGL
    // context and all OpenGL calls have no effect.

    // Start a timer that will try to get 60 frames per second (the actual
    // frame rate depends on the operating system and other running programs)
    m_clock.start();
    timer.start(1000 / 60);

    // Center the mouse, which is explained more in mouseMoveEvent() below.
    // This needs to be done here because the mouse may be initially outside
    // the fullscreen window and will not automatically receive mouse move
    // events. This occurs if there are two monitors and the mouse is on the
    // secondary monitor.

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

    GLfloat ambientLight[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat diffuseLight[] = { 1.0f, 1.0f, 1.0, 1.0f };
    GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat position[] = { 2.0f, 2.0f, 2.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

//    glDisable(GL_DITHER);

    glDisable(GL_LIGHTING);
//    glShadeModel(GL_FLAT);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    initializeResources();
    m_quadric = gluNewQuadric();

    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));

    m_textureID = this->loadTexture("../textures/particle_cloud.png");

    glEnable(GL_ALPHA_TEST);

    paintGL();
}

/**
Create shader programs.
**/
void View::createShaderPrograms()
{
      const QGLContext *ctx = context();
      m_shaderPrograms["lightscatter"] = this->newFragShaderProgram(ctx, "../shaders/lightscatter.frag");
}

void View::initializeResources()
{

    m_skybox = loadSkybox();

    loadCubeMap();

    createShaderPrograms();

    createFramebufferObjects(width(), height());
}

GLuint View::loadSkybox() {
    //loads skybox

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    // Be glad we wrote this for you...ugh.
    glBegin(GL_QUADS);
    float extent = 2000.f;
    glTexCoord3f( 1.0f, -1.0f, -1.0f); glVertex3f( extent, -extent, -extent);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-extent, -extent, -extent);
    glTexCoord3f(-1.0f,  1.0f, -1.0f); glVertex3f(-extent,  extent, -extent);
    glTexCoord3f( 1.0f,  1.0f, -1.0f); glVertex3f( extent,  extent, -extent);
    glTexCoord3f( 1.0f, -1.0f,  1.0f); glVertex3f( extent, -extent,  extent);
    glTexCoord3f( 1.0f, -1.0f, -1.0f); glVertex3f( extent, -extent, -extent);
    glTexCoord3f( 1.0f,  1.0f, -1.0f); glVertex3f( extent,  extent, -extent);
    glTexCoord3f( 1.0f,  1.0f,  1.0f); glVertex3f( extent,  extent,  extent);
    glTexCoord3f(-1.0f, -1.0f,  1.0f); glVertex3f(-extent, -extent,  extent);
    glTexCoord3f( 1.0f, -1.0f,  1.0f); glVertex3f( extent, -extent,  extent);
    glTexCoord3f( 1.0f,  1.0f,  1.0f); glVertex3f( extent,  extent,  extent);
    glTexCoord3f(-1.0f,  1.0f,  1.0f); glVertex3f(-extent,  extent,  extent);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-extent, -extent, -extent);
    glTexCoord3f(-1.0f, -1.0f,  1.0f); glVertex3f(-extent, -extent,  extent);
    glTexCoord3f(-1.0f,  1.0f,  1.0f); glVertex3f(-extent,  extent,  extent);
    glTexCoord3f(-1.0f,  1.0f, -1.0f); glVertex3f(-extent,  extent, -extent);
    glTexCoord3f(-1.0f,  1.0f, -1.0f); glVertex3f(-extent,  extent, -extent);
    glTexCoord3f(-1.0f,  1.0f,  1.0f); glVertex3f(-extent,  extent,  extent);
    glTexCoord3f( 1.0f,  1.0f,  1.0f); glVertex3f( extent,  extent,  extent);
    glTexCoord3f( 1.0f,  1.0f, -1.0f); glVertex3f( extent,  extent, -extent);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-extent, -extent, -extent);
    glTexCoord3f(-1.0f, -1.0f,  1.0f); glVertex3f(-extent, -extent,  extent);
    glTexCoord3f( 1.0f, -1.0f,  1.0f); glVertex3f( extent, -extent,  extent);
    glTexCoord3f( 1.0f, -1.0f, -1.0f); glVertex3f( extent, -extent, -extent);
    glEnd();
    glEndList();

    return id;
}

void View::loadCubeMap()
{

    QList<QFile *> fileList;
    fileList.append(new QFile("../textures/cloud_left.png"));
    fileList.append(new QFile("../textures/cloud_right.png"));
    fileList.append(new QFile("../textures/cloud_top.png"));
    fileList.append(new QFile("../textures/cloud_bottom.png"));
    fileList.append(new QFile("../textures/cloud_front.png"));
    fileList.append(new QFile("../textures/cloud_back.png"));


    Q_ASSERT(fileList.length() == 6);

    // Generate an ID
    GLuint id;
    glGenTextures(1, &id);

    // Bind the texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    // Load and build mipmaps for each image
    for (int i = 0; i < 6; ++i)
    {
        QImage image, texture;
        image.load(fileList[i]->fileName());
        image = image.mirrored(false, true);
        texture = QGLWidget::convertToGLFormat(image);
        texture = texture.scaledToWidth(1024, Qt::SmoothTransformation);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 3, 3, texture.width(), texture.height(), 0, GL_RGBA,GL_UNSIGNED_BYTE, texture.bits());
        gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 3, texture.width(), texture.height(), GL_RGBA, GL_UNSIGNED_BYTE, texture.bits());
    }

    // Set filter when pixel occupies more than one texture element
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    // Set filter when pixel smaller than one texture element
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_cubeMap = id;
}

void View::applyOrthogonalCamera(float width, float height)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.f, width, 0.f, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void View::applyPerspectiveCamera(float width, float height)
{
    float ratio = ((float) width) / height;
    Vector3 dir(-Vector3::fromAngles(m_camera.theta, m_camera.phi));
    Vector3 eye(m_camera.center - dir * m_camera.zoom);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(m_camera.fovy, ratio, 0.1f, 7000.f);
    gluLookAt(eye.x, eye.y, eye.z, eye.x + dir.x, eye.y + dir.y, eye.z + dir.z,
              m_camera.up.x, m_camera.up.y, m_camera.up.z);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void View::createFramebufferObjects(int width, int height)
{
    // Allocate the main framebuffer object for rendering the scene to
    // This needs a depth attachment
    m_framebufferObjects["fbo_0"] = new QGLFramebufferObject(width, height, QGLFramebufferObject::Depth,
                                                             GL_TEXTURE_2D, GL_RGB16F_ARB);
    m_framebufferObjects["fbo_0"]->format().setSamples(16);
    // Allocate the secondary framebuffer obejcts for rendering textures to (post process effects)
    // These do not require depth attachments
    m_framebufferObjects["fbo_1"] = new QGLFramebufferObject(width, height, QGLFramebufferObject::NoAttachment,
                                                             GL_TEXTURE_2D, GL_RGB16F_ARB);

    m_framebufferObjects["fbo_2"] = new QGLFramebufferObject(width, height, QGLFramebufferObject::NoAttachment,
                                                             GL_TEXTURE_2D, GL_RGB16F_ARB);
}

void View::renderLightScatter(int width, int height)
{
    float exposure = 0.2;
    float decay = 0.2;
    float density = 0.5;
    float weight = 0.9;

    GLfloat lightPositionInWorld[4];
    lightPositionInWorld[0] = -1998.;
    lightPositionInWorld[1] = 800.;
    lightPositionInWorld[2] = -1998.;
    lightPositionInWorld[3] = 0.;

//    double lightPositionInWorld[4];
//    lightPositionInWorld[0] = -1998.;
//    lightPositionInWorld[1] = 800.;
//    lightPositionInWorld[2] = -1998.;
//    lightPositionInWorld[3] = 0.;

//    double modelView[16];
//    double projection[16];
//    double viewport[4];
//    double depthRange[2];

//    glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
//    glGetDoublev(GL_PROJECTION_MATRIX, projection);
//    glGetDoublev(GL_VIEWPORT, viewport);
//    glGetDoublev(GL_DEPTH_RANGE, depthRange);

//    // Compose the matrices into a single row-major transformation
//    double T[4][4];
//    int r, c, i;
//    for (r = 0; r < 4; ++r)
//    {
//     for (c = 0; c < 4; ++c)
//     {
//       T[r][c] = 0;
//       for (i = 0; i < 4; ++i)
//       {
//         // OpenGL matrices are column major
//          T[r][c] += projection[r + i * 4] * modelView[i + c * 4];
//       }
//     }
//    }

//    // Transform the vertex
//    double result[4];
//    for (r = 0; r < 4; ++r)
//    {
//     result[r] += (T[r][0]*lightPositionInWorld[0]) + (T[r][1]*lightPositionInWorld[1]) + (T[r][2]*lightPositionInWorld[2]) + (T[r][3]*lightPositionInWorld[3]);
//    }

//    // Homogeneous divide
//    const double rhw = 1 / result[3];

//    GLfloat lightPositionOnScreen[2];
//    lightPositionOnScreen[0] = (1 + result[0] * rhw) * viewport[2] / 2 + viewport[0];
//    lightPositionOnScreen[1] = (1 - result[1] * rhw) * viewport[3] / 2 + viewport[1];

//    return Vector4(
//    (1 + result.x * rhw) * viewport[2] / 2 + viewport[0],
//    (1 - result.y * rhw) * viewport[3] / 2 + viewport[1],
//    (result.z * rhw) * (depthRange[1] - depthRange[0]) + depthRange[0],
//    rhw);

    //do this in vert
    //gl_ModelViewProjectionMatrix*lightPositionInWorld;

//    //TODO : calculate light position on screen
//    GLfloat lightPositionOnScreen[2];
//    lightPositionOnScreen[0] = 50;
//    lightPositionOnScreen[1] = 100;

    m_framebufferObjects["fbo_1"]->bind();
    m_shaderPrograms["lightscatter"]->bind();

    glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_2"]->texture());

    m_shaderPrograms["lightscatter"]->setUniformValue("exposure", exposure);
    m_shaderPrograms["lightscatter"]->setUniformValue("decay", decay);
    m_shaderPrograms["lightscatter"]->setUniformValue("density", density);
    m_shaderPrograms["lightscatter"]->setUniformValue("weight", weight);
    m_shaderPrograms["lightscatter"]->setUniformValueArray("lightPositionInWorld", lightPositionInWorld, 1, 4);

    renderTexturedQuad(width , height);
    m_shaderPrograms["lightscatter"]->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    m_framebufferObjects["fbo_1"]->release();
}

void View::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int time = m_clock.elapsed();
    m_fps = 1000.f / (time - m_prevTime);
    m_prevTime = time;
    int width = this->width();
    int height = this->height();

    // Render the scene to a framebuffer
    m_framebufferObjects["fbo_0"]->bind();
    applyPerspectiveCamera(width, height);

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Enable cube maps and draw the skybox
    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMap);
    glCallList(m_skybox);

    glBindTexture(GL_TEXTURE_CUBE_MAP,0);
    glDisable(GL_TEXTURE_CUBE_MAP);

    // Enable culling (back) faces for rendering the dragon
    glEnable(GL_CULL_FACE);

    Vector3 startPoint(-2000, -1000, -2000);

    // calculate the angle and axis about which the squares should be rotated to match
    // the camera's rotation for billboarding
    Vector3 dir(-Vector3::fromAngles(m_camera.theta, m_camera.phi));
    Vector3 faceNormal = Vector3(0,0,-1);
    Vector3 axis = dir.cross(faceNormal);
    axis.normalize();
    double angle = acos(dir.dot(faceNormal) / dir.length() / faceNormal.length());

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(-1500, 800, -1500);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gluSphere(m_quadric, 400.f, 20.f, 20.f);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexEnvf(GL_TEXTURE_2D,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_num_squares = 0;

    for (int i=0; i < dimX; i++)
    {
        for (int j=0; j < dimY; j++)
        {
            for (int k=0; k < dimZ; k++)
            {
                float intensity = m_clouds[i][j][k];

                if (intensity > 0.4)
                {
                    m_num_squares++;
                    glMatrixMode(GL_MODELVIEW);
                    glPushMatrix();
                    glTranslatef(startPoint.x+(SQUARE_DISTRIBUTION*i), startPoint.y+(SQUARE_DISTRIBUTION*j), startPoint.z+(SQUARE_DISTRIBUTION*k));
                    glRotatef((-angle/M_PI)*180, axis.x, axis.y, axis.z);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.1f*intensity*intensity);
                    renderTexturedQuad(SQUARE_SIZE, SQUARE_SIZE);
                    glPopMatrix();
                }
            }
        }
    }

    cout << "Squares painted: " << m_num_squares << endl;

    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    m_framebufferObjects["fbo_0"]->release();

    // Copy the rendered scene into framebuffer 1
    m_framebufferObjects["fbo_0"]->blitFramebuffer(m_framebufferObjects["fbo_1"],
                                                   QRect(0, 0, width, height), m_framebufferObjects["fbo_0"],
                                                   QRect(0, 0, width, height), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    applyOrthogonalCamera(width, height);
    glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_1"]->texture());
    renderTexturedQuad(width, height);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_framebufferObjects["fbo_2"]->bind();
    glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_1"]->texture());
    renderTexturedQuad(width, height);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_framebufferObjects["fbo_2"]->release();

    this->renderLightScatter(width, height);

    glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_1"]->texture());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Enable alpha blending and render the texture to the screen
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    renderTexturedQuad(width, height);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);

    paintText();
}

void View::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

GLuint View::loadTexture(const QString &path)
{
    QFile file(path);

    QImage image, texture;
    if(!file.exists()) return -1;
    image.load(file.fileName());
    texture = QGLWidget::convertToGLFormat(image);

    //Put your code here
    glEnable(GL_TEXTURE_2D);

    GLuint id = 0;
    glGenTextures(1, &id);

    // Make the texture we just created is the new active texture
    glBindTexture(GL_TEXTURE_2D, id);

    // Copy the image data into the OpenGL texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

    // Set filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return id; /* return something meaningful */
}

void View::renderTexturedQuad(int width, int height) {
    // Clamp value to edge of texture when texture index is out of bounds
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Draw the  quad
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(width, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(width, height);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, height);
    glEnd();
}

void View::mousePressEvent(QMouseEvent *event)
{
    m_prevMousePos.x = event->x();
    m_prevMousePos.y = event->y();
}

void View::wheelEvent(QWheelEvent *event)
{
    if (event->orientation() == Qt::Vertical)
    {
        m_camera.mouseWheel(event->delta());
    }
}

void View::mouseMoveEvent(QMouseEvent *event)
{
    // This starter code implements mouse capture, which gives the change in
    // mouse position since the last mouse movement. The mouse needs to be
    // recentered after every movement because it might otherwise run into
    // the edge of the screen, which would stop the user from moving further
    // in that direction. Note that it is important to check that deltaX and
    // deltaY are not zero before recentering the mouse, otherwise there will
    // be an infinite loop of mouse move events.
    int deltaX = event->x() - width() / 2;
    int deltaY = event->y() - height() / 2;
    if (!deltaX && !deltaY) return;
//    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));

    // TODO: Handle mouse movements here

    Vector2 pos(event->x(), event->y());
    if (event->buttons() & Qt::LeftButton || event->buttons() & Qt::RightButton)
    {
        m_camera.mouseMove(pos - m_prevMousePos);
    }
    m_prevMousePos = pos;

}

void View::mouseReleaseEvent(QMouseEvent *event)
{
}

void View::keyReleaseEvent(QKeyEvent *event)
{
}

void View::tick()
{
    // Get the number of seconds since the last tick (variable update rate)
    float seconds = m_clock.restart() * 0.001f;

    // TODO: Implement the demo update here

    // Flag this view for repainting (Qt will call paintGL() soon after)
    update();
}

void View::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) QApplication::quit();

//    //we adjust how we move by what angle we're currently facing
//    double cx = cos(-m_angleX * M_PI/180);
//    double sx = sin(-m_angleX * M_PI/180);
//    if(event->key() == Qt::Key_W)
//    {
//        m_zDiff -= 0.025f * cx;
//        m_xDiff -= 0.025f * sx;
//        this->updateCamera();
//        this->update();
//    }
//    else if(event->key() == Qt::Key_S)
//    {
//        m_zDiff += 0.025f * cx;
//        m_xDiff += 0.025f * sx;
//        this->updateCamera();
//        this->update();
//    }
//    else if(event->key() == Qt::Key_D)
//    {
//        m_zDiff += 0.025f * -sx;
//        m_xDiff += 0.025f * cx;
//        this->updateCamera();
//        this->update();
//    }
//    else if(event->key() == Qt::Key_A)
//    {
//        m_zDiff -= 0.025f * -sx;
//        m_xDiff -= 0.025f * cx;
//        this->updateCamera();
//        this->update();
//    }
//    else if(event->key() == Qt::Key_Escape)
//    {
//        m_firstPersonMode = false;
//        m_originalMouseX = -1;
//        m_originalMouseY = -1;
//    }
}

/**
    Creates a shader program from both vert and frag shaders
  **/
QGLShaderProgram* View::newShaderProgram(const QGLContext *context, QString vertShader, QString fragShader)
{
    QGLShaderProgram *program = new QGLShaderProgram(context);
    program->addShaderFromSourceFile(QGLShader::Vertex, vertShader);
    program->addShaderFromSourceFile(QGLShader::Fragment, fragShader);
    program->link();
    return program;
}

/**
    Creates a new shader program from a frag shader only
  **/
QGLShaderProgram* View::newFragShaderProgram(const QGLContext *context, QString fragShader)
{
    QGLShaderProgram *program = new QGLShaderProgram(context);
    program->addShaderFromSourceFile(QGLShader::Fragment, fragShader);
    program->link();
    return program;
}

void View::paintText()
{
    glColor3f(1.f, 1.f, 1.f);

    // Combine the previous and current framerate
    if (m_fps >= 0 && m_fps < 1000)
    {
       m_prevFps *= 0.95f;
       m_prevFps += m_fps * 0.05f;
    }

    // QGLWidget's renderText takes xy coordinates, a string, and a font
    renderText(10, 20, "FPS: " + QString::number((int) (m_prevFps)), m_font);
    renderText(10, 35, "S: Save screenshot", m_font);
}

