#ifndef CLOUDGENERATOR_H
#define CLOUDGENERATOR_H

class CloudGenerator
{

public:
    CloudGenerator();
    ~CloudGenerator();
    double*** calcIntensity(int dimX, int dimY, int dimZ);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y, double z);
private:
    double*** m_intensity;
    int m_dimX;
    int m_dimY;
    int m_dimZ;

};

#endif // CLOUDGENERATOR_H


