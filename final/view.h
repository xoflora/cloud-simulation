#ifndef VIEW_H
#define VIEW_H

#include <QGLWidget>
#include <QHash>
#include <QString>
#include <qgl.h>
#include <QTime>
#include <QTimer>

#include "camera.h"
#include "vector.h"
#include "cloudgenerator.h"

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

    int m_prevTime;
    double*** m_clouds;
    int m_num_squares;
    GLuint m_textureID;
    float m_prevFps, m_fps;
    Vector2 m_prevMousePos;

    GLuint m_skybox;
    GLuint m_cubeMap;
    QHash<QString, QGLFramebufferObject *> m_framebufferObjects; // hash map of all framebuffer objects
    OrbitCamera m_camera;

    QFont m_font; // font for rendering text

    CloudGenerator* m_cloudgen;
    GLUquadric* m_quadric;


private slots:
    void tick();
};

#endif // VIEW_H

