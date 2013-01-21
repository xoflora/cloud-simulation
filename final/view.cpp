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

#define dimX 50
#define dimY 25
#define dimZ 50
#define EXTENT 500.
#define SUN_RADIUS 35
#define SUNX -EXTENT+(2*SUN_RADIUS)
#define SUNY (2*EXTENT)/3
#define SUNZ -EXTENT+(2*SUN_RADIUS)

using namespace std;
class QGLShaderProgram;

View::View(QWidget *parent) : QGLWidget(parent), m_font("Verdana", 8, 4)
{
    // View needs all move events, not just mouse drag events
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_camera.center = Vector3(0.f, 0.f, 0.f);
    m_camera.up = Vector3(0.f, 1.f, 0.f);
    m_camera.zoom = 3.5f;
    m_camera.theta = M_PI * 1.5f, m_camera.phi = 0.2f;
    m_camera.fovy = 60.f;

    // The game loop is implemented using a timer
    connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));

    //initialize settings for our program
    this->setSquareSize(100);
    m_godRaysEnabled = true;
    m_godModeEnabled = false;
    m_modelerModeEnabled = false;
    m_cloudgen = new CloudGenerator();
    m_clouds = m_cloudgen->calcIntensity(dimX, dimY, dimZ);
}

View::~View()
{
    gluDeleteQuadric(m_quadric);
    delete(m_cloudgen);
    delete m_framebufferObjects["fbo_0"];
    delete m_framebufferObjects["fbo_1"];
    delete m_framebufferObjects["fbo_2"];
    delete m_framebufferObjects["fbo_3"];
}

/**
  A mutator for the square size (container size). The distribution of our cloud depends on this.
  */
void View::setSquareSize(float squareSize)
{
    m_squareSize = squareSize;
    m_squareDistribution = m_squareSize / 5.0;
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

    glDisable(GL_LIGHTING);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    initializeResources();
    m_quadric = gluNewQuadric();

    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));

    //loads the textures used for the cloud particles
    m_textureIDwhite = this->loadTexture("../textures/particle_cloud.png");
    m_textureIDModeler = this->loadTexture("../textures/particle_cloud_gradient.png");
    m_textureID1 = this->loadTexture("../textures/particle_cloud1.png");
    m_textureID2 = this->loadTexture("../textures/particle_cloud2.png");
    m_textureID3 = this->loadTexture("../textures/particle_cloud3.png");
    m_textureID4 = this->loadTexture("../textures/particle_cloud4.png");
    m_textureID5 = this->loadTexture("../textures/particle_cloud5.png");
    m_textureID6 = this->loadTexture("../textures/particle_cloud6.png");
    m_textureID7 = this->loadTexture("../textures/particle_cloud7.png");
    m_textureID8 = this->loadTexture("../textures/particle_cloud8.png");

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

/**
  loadSkybox: initializes and draws the sky box used for our cloud display (separate one for the occlusion pass, see below)
  */

GLuint View::loadSkybox() {
    //loads skybox

    GLuint id = glGenLists(1);
    glNewList(id, GL_COMPILE);

    // Be glad we wrote this for you...ugh.
    glBegin(GL_QUADS);
    float extent = EXTENT;
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
    m_framebufferObjects["fbo_3"] = new QGLFramebufferObject(width, height, QGLFramebufferObject::NoAttachment,
                                                             GL_TEXTURE_2D, GL_RGB16F_ARB);
}

/**
  renderLightScatter: does pre-processing prior to passing our scene to the shader for god rays
  */

void View::renderLightScatter(int width, int height)
{
    float exposure = 0.8; //brightness of the rays compared to the rest of the scene
    float decay = 0.95; //determines the fall-off of the rays from the light source
    float density = 0.9; //frequency of samples between the pixel and the light source
    float weight = 1.0; //scales the decay

    Vector3 dir(-Vector3::fromAngles(m_camera.theta, m_camera.phi));

    Vector3 lightVector(-SUNX, -SUNY,-SUNZ);
    lightVector.normalize();

    float dotLightLook = lightVector.dot(dir);

    double lightPositionInWorld[4];
    lightPositionInWorld[0] = SUNX;
    lightPositionInWorld[1] = SUNY;
    lightPositionInWorld[2] = SUNZ;
    lightPositionInWorld[3] = 1.;

    double modelView[16];
    double projection[16];
    double viewPort[4];
    double depthRange[2];

    //copies the matrices of the camera to our arrays

    glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_VIEWPORT, viewPort);
    glGetDoublev(GL_DEPTH_RANGE, depthRange);

    // Compose the matrices into a single row-major transformation

    double T[4][4];
    int r, c, i;
    for (r = 0; r < 4; ++r)
    {
     for (c = 0; c < 4; ++c)
     {
       T[r][c] = 0;
       for (i = 0; i < 4; ++i)
       {
          T[r][c] += projection[r + i * 4] * modelView[i + c * 4];
       }
     }
    }

    double result[4];
    for (r = 0; r < 4; ++r)
    {
        result[r] = (T[r][0]*lightPositionInWorld[0]) + (T[r][1]*lightPositionInWorld[1]) + (T[r][2]*lightPositionInWorld[2]) + (T[r][3]*lightPositionInWorld[3]);
    }

    const double rhw = 1 / result[3];

    GLfloat lightPositionOnScreen[2];
    lightPositionOnScreen[0] = (1 + result[0] * rhw) * viewPort[2] / 2 + viewPort[0];
    lightPositionOnScreen[1] = (1 - result[1] * rhw) * viewPort[3] / 2 + viewPort[1];

    lightPositionOnScreen[0] = lightPositionOnScreen[0]/((double)this->width());
    lightPositionOnScreen[1] = 1-(lightPositionOnScreen[1]/((double)this->height()));


    m_framebufferObjects["fbo_1"]->bind();
    m_shaderPrograms["lightscatter"]->bind();

    glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_2"]->texture());

    m_shaderPrograms["lightscatter"]->setUniformValue("exposure", exposure);
    m_shaderPrograms["lightscatter"]->setUniformValue("decay", decay);
    m_shaderPrograms["lightscatter"]->setUniformValue("density", density);
    m_shaderPrograms["lightscatter"]->setUniformValue("weight", weight);
    m_shaderPrograms["lightscatter"]->setUniformValue("dotLightLook", dotLightLook);
    m_shaderPrograms["lightscatter"]->setUniformValueArray("lightPositionOnScreen", lightPositionOnScreen, 1, 2);
    applyOrthogonalCamera(width, height);

    renderTexturedQuad(width , height);
    m_shaderPrograms["lightscatter"]->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    m_framebufferObjects["fbo_1"]->release();
}

void View::renderBlackBox()
{
    /* change the color of the skybox based on whether we're rending just the god rays or multiplying
     * them with the original image (this optimization was made so that image of just the god rays
     * would be visible on even on low contrast monitors/projectors */
    if(m_godModeEnabled)
    {
        glColor3f(0.97f,0.97f,0.97f);
    } else
    {
        glColor3f(0.99f,0.99f,0.99f);
    }

    glBegin(GL_QUADS);
    float extent = EXTENT;
    glVertex3f( extent, -extent, -extent);
    glVertex3f(-extent, -extent, -extent);
    glVertex3f(-extent,  extent, -extent);
    glVertex3f( extent,  extent, -extent);
    glVertex3f( extent, -extent,  extent);
    glVertex3f( extent, -extent, -extent);
    glVertex3f( extent,  extent, -extent);
    glVertex3f( extent,  extent,  extent);
    glVertex3f(-extent, -extent,  extent);
    glVertex3f( extent, -extent,  extent);
    glVertex3f( extent,  extent,  extent);
    glVertex3f(-extent,  extent,  extent);
    glVertex3f(-extent, -extent, -extent);
    glVertex3f(-extent, -extent,  extent);
    glVertex3f(-extent,  extent,  extent);
    glVertex3f(-extent,  extent, -extent);
    glVertex3f(-extent,  extent, -extent);
    glVertex3f(-extent,  extent,  extent);
    glVertex3f( extent,  extent,  extent);
    glVertex3f( extent,  extent, -extent);
    glVertex3f(-extent, -extent, -extent);
    glVertex3f(-extent, -extent,  extent);
    glVertex3f( extent, -extent,  extent);
    glVertex3f( extent, -extent, -extent);
    glEnd();
}

void View::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int width = this->width();
    int height = this->height();

    int time = m_clock.elapsed();
    m_fps = 1000.f / (time - m_prevTime);
    m_prevTime = time;

    if(this->m_godRaysEnabled || this->m_godModeEnabled)
    {
        m_framebufferObjects["fbo_0"]->bind();
        applyPerspectiveCamera(width, height);

        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        this->renderBlackBox();

        glEnable(GL_CULL_FACE);


        //draws the sun for god rays
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(SUNX, SUNY, SUNZ);
        glColor4f(0.0f, 0.0f, 0.0f, 0.f);
        gluSphere(m_quadric, SUN_RADIUS, 20, 20);
        glPopMatrix();

        glDisable(GL_CULL_FACE);

        glEnable(GL_CULL_FACE);

        this->renderClouds(true);

        glDisable(GL_CULL_FACE);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        m_framebufferObjects["fbo_0"]->release();

        // Copy the rendered scene into framebuffer 1
        m_framebufferObjects["fbo_0"]->blitFramebuffer(m_framebufferObjects["fbo_1"],
                                                       QRect(0, 0, width, height), m_framebufferObjects["fbo_0"],
                                                       QRect(0, 0, width, height), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    //START RENDERING REAL SKYBOX
    if(!this->m_godModeEnabled)
    {
        // Render the scene to a framebuffer
        m_framebufferObjects["fbo_0"]->bind();
        applyPerspectiveCamera(width, height);

        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Enable cube maps and draw the skybox
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMap);
        glCallList(m_skybox); //renders the skybox

        glBindTexture(GL_TEXTURE_CUBE_MAP,0);
        glDisable(GL_TEXTURE_CUBE_MAP);

        // Enable culling (back) faces for rendering the dragon
        glEnable(GL_CULL_FACE);

        this->renderClouds(false);

        glDisable(GL_CULL_FACE);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        m_framebufferObjects["fbo_0"]->release();
    }

    //END REAL SKYBOX

    // Copy the rendered scene into framebuffer 1
    m_framebufferObjects["fbo_0"]->blitFramebuffer(m_framebufferObjects["fbo_3"],
                                                   QRect(0, 0, width, height), m_framebufferObjects["fbo_0"],
                                                   QRect(0, 0, width, height), GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // render the scene if not in god mode
    if(!m_godModeEnabled)
    {
        applyOrthogonalCamera(width, height);
        glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_3"]->texture());
        renderTexturedQuad(width, height);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // check if the user specified that god rays should be calculated on the gpu
    if(m_godRaysEnabled || m_godModeEnabled)
    {
        // copy what's in FBO 1 to FBO 2 for renderLightScatter shader stuff
        applyOrthogonalCamera(width, height);
        m_framebufferObjects["fbo_2"]->bind();
        glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_1"]->texture());
        renderTexturedQuad(width, height);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_framebufferObjects["fbo_2"]->release();

        // Enable alpha blending and render the texture from the GPU to the screen
        applyPerspectiveCamera(width, height);
        this->renderLightScatter(width, height);
        applyOrthogonalCamera(width, height);
        glBindTexture(GL_TEXTURE_2D, m_framebufferObjects["fbo_1"]->texture());

        //blend if we're using god rays
        if(!m_godModeEnabled)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
        }

        renderTexturedQuad(width, height);

        if(!m_godModeEnabled)
        {
            glDisable(GL_BLEND);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    paintText();
}

/**
  Called to draw the cloud particles on the screen (used for both god mode and normal node)
  */

void View::renderClouds(bool renderGreyMode) {

    //start point is determined by our sky box size
    Vector3 startPoint(-EXTENT, -EXTENT+(2*SUN_RADIUS), -EXTENT);

    // calculate the angle and axis about which the squares should be rotated to match
    // the camera's rotation for billboarding
    Vector3 dir(-Vector3::fromAngles(m_camera.theta, m_camera.phi));
    Vector3 faceNormal = Vector3(0,0,-1);
    Vector3 axis = dir.cross(faceNormal);
    axis.normalize();
    double angle = acos(dir.dot(faceNormal) / dir.length() / faceNormal.length());

    //if we're rending the grey occlusion mode, use white cloud particles
    if (renderGreyMode)
    {
        glBindTexture(GL_TEXTURE_2D, m_textureIDwhite);
    // use white gradient particle if we're in modeler mode
    } else if(m_modelerModeEnabled)
    {
        glBindTexture(GL_TEXTURE_2D, m_textureIDModeler);
    }

    glTexEnvf(GL_TEXTURE_2D,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND_SRC);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_num_squares = 0;

    //light vector indicates direction in which the suns rays are going
    Vector3 lightVector(-SUNX, -SUNY, -SUNZ);
    lightVector.normalize();

    double particleSunAngle;

    //populate the cloud lattice
    for (int i=0; i < dimX; i++)
    {
        for (int j=0; j < dimY; j++)
        {
            for (int k=0; k < dimZ; k++)
            {
                //intensity is the value given in the corresponding perlin 3d array, also incorporating vertical fall-off
                float intensity = m_clouds[i][j][k]*(1.0 - (j/((float) dimY)));
                //threshold for rendering a particle
                if (intensity > 0.1)
                {
                    //use various particle colors depending on intensity and lighting scheme
                    if (!(renderGreyMode || m_modelerModeEnabled))
                    {
                        //vector from the sun to the current particle
                        Vector3 toParticle(startPoint.x+(m_squareDistribution*i)-SUNX, startPoint.y+(m_squareDistribution*j)-SUNY, startPoint.z+(m_squareDistribution*k)-SUNZ);
                        toParticle.normalize();

                        //cos of the angle from sun to particle, mapping to range [0,1]
                        particleSunAngle = ((lightVector.dot(toParticle))/2.)+0.5;

                        //factor is greater when particles are in more direct view of the sun
                        double factor = ((1.8-(particleSunAngle))*m_clouds[i][j][k]);

                        if (factor >= 0.0 && factor <= 0.125)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID8);
                        }
                        else if (factor > 0.125 && factor <= 0.18)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID7);
                        }
                        else if (factor > 0.18 && factor <= 0.25)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID6);
                        }
                        else if (factor > 0.25 && factor <= 0.31)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID5);
                        }
                        else if (factor > 0.31 && factor <= 0.4)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID4);
                        }
                        else if (factor > 0.4 && factor <= 0.5)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID3);
                        }
                        else if (factor > 0.5 && factor <= 0.6)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID2);
                        }
                        else if (factor > 0.6 && factor <= 1.0)
                        {
                            glBindTexture(GL_TEXTURE_2D, m_textureID1);
                        }
                    }

                    m_num_squares++;
                    glMatrixMode(GL_MODELVIEW);
                    glPushMatrix();
                    glTranslatef(startPoint.x+(m_squareDistribution*i), startPoint.y+(m_squareDistribution*j), startPoint.z+(m_squareDistribution*k));
                    glRotatef((-angle/M_PI)*180, axis.x, axis.y, axis.z);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
                    renderTexturedQuad(m_squareSize, m_squareSize);

                    if (!renderGreyMode && !m_modelerModeEnabled)
                    {
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                    glPopMatrix();
                }
            }
        }
    }

    if (renderGreyMode || m_modelerModeEnabled)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}


void View::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    delete m_framebufferObjects["fbo_0"];
    delete m_framebufferObjects["fbo_1"];
    delete m_framebufferObjects["fbo_2"];
    delete m_framebufferObjects["fbo_3"];
    createFramebufferObjects(w, h);
}

GLuint View::loadTexture(const QString &path)
{
    QFile file(path);

    QImage image, texture;
    if(!file.exists()) return -1;
    image.load(file.fileName());
    texture = QGLWidget::convertToGLFormat(image);

    glEnable(GL_TEXTURE_2D);

    GLuint id = 0;
    glGenTextures(1, &id);

    // Make the texture we just created is the new active texture
    glBindTexture(GL_TEXTURE_2D, id);

    // Copy the image data into the OpenGL texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

    // Set filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return id; /* return something meaningful */
}

void View::renderTexturedQuad(int width, int height)
{
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

    if (event->key() == Qt::Key_G)
    {
       m_godRaysEnabled = !m_godRaysEnabled && !m_modelerModeEnabled;
    }

    if (event->key() == Qt::Key_B)
    {
       m_godModeEnabled = !m_godModeEnabled;
    }

    if (event->key() == Qt::Key_Q)
    {
        this->setSquareSize(min(m_squareSize + 5, 100));
    }

    if (event->key() == Qt::Key_W)
    {
       this->setSquareSize(max(m_squareSize - 5, 0));
    }

    if (event->key() == Qt::Key_M)
    {
       m_modelerModeEnabled = !m_modelerModeEnabled;
       m_godRaysEnabled = false;
    }
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
    renderText(10, 20, "G: Toggle God Rays", m_font);
    renderText(10, 35, "B: Toggle God Ray Pass", m_font);
    renderText(10, 50, "M: Toggle Modeler Mode", m_font);
    renderText(10, 65, "Q/W: Increase/Decrease Container Size", m_font);
}

