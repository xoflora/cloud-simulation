#ifndef VIEW_H
#define VIEW_H

#include <QGLWidget>
#include <QHash>
#include <QString>
#include <qgl.h>
#include <QTime>
#include <QTimer>
#include <QGLShaderProgram>
#include <QGLShader>

#include "camera.h"
#include "vector.h"
#include "cloudgenerator.h"

class QGLShaderProgram;
class QGLFramebufferObject;

class View : public QGLWidget
{
    Q_OBJECT

public:
    View(QWidget *parent);
    ~View();

private:
    QTime m_clock;
    QTimer timer;

    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

    void initializeResources();
    GLuint loadSkybox();
    void loadCubeMap();
    void applyOrthogonalCamera(float width, float height);
    void applyPerspectiveCamera(float width, float height);
    void createFramebufferObjects(int width, int height);
    void renderTexturedQuad(int width, int height);

    void paintText();

    GLuint loadTexture(const QString &path);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void createShaderPrograms();
    QGLShaderProgram* newShaderProgram(const QGLContext *context, QString vertShader, QString fragShader);
    QGLShaderProgram* newFragShaderProgram(const QGLContext *context, QString fragShader);
    void renderLightScatter(int width, int height);

    void renderBlackBox();
    void renderClouds(bool blackModeEnabled);
    void setSquareSize(float squareSize);

    int m_prevTime;
    double*** m_clouds;
    int m_num_squares;
    GLuint m_textureID1;
    GLuint m_textureID2;
    GLuint m_textureID3;
    GLuint m_textureID4;
    GLuint m_textureID5;
    GLuint m_textureID6;
    GLuint m_textureID7;
    GLuint m_textureID8;
    GLuint m_textureIDwhite;
    GLuint m_textureIDModeler;
    float m_squareSize;
    float m_squareDistribution;
    bool m_godRaysEnabled; // allows the user to toggle between using the god rays or not in the scene
    bool m_godModeEnabled; // allows the user to view JUST the god rays given by the shader
    bool m_modelerModeEnabled; // allows the user to view the particles without our beautiful textures
    float m_prevFps, m_fps;
    Vector2 m_prevMousePos;
    OrbitCamera m_camera;

    GLuint m_skybox;
    GLuint m_cubeMap;

    // Resources
    QHash<QString, QGLShaderProgram *> m_shaderPrograms; // hash map of all shader programs
    QHash<QString, QGLFramebufferObject *> m_framebufferObjects; // hash map of all framebuffer objects
    QFont m_font; // font for rendering text

    CloudGenerator* m_cloudgen;
    GLUquadric* m_quadric;

    int time;


private slots:
    void tick();
};

#endif // VIEW_H

