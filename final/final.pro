# 
# CS123 Final Project Starter Code
# Adapted from starter code graciously provided by CS195-U: 3D Game Engines
#

QT += core gui opengl

TARGET = final
TEMPLATE = app

# If you add your own folders, add them to INCLUDEPATH and DEPENDPATH, e.g.
# INCLUDEPATH += folder1 folder2
# DEPENDPATH += folder1 folder2

INCLUDEPATH += ../shaders
DEPENDPATH += ../shaders

SOURCES += main.cpp \
    mainwindow.cpp \
    view.cpp \
    camera.cpp \
    cloudgenerator.cpp

HEADERS += mainwindow.h \
    view.h \
    vector.h \
    camera.h \
    cloudgenerator.h

FORMS += mainwindow.ui

OTHER_FILES += \
    ../shaders/lightscatter.frag \
    ../shaders/lightscatter.vert
