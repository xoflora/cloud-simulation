#include "cloudgenerator.h"
#include <qgl.h>
#include <math.h>
#include <iostream>

#define SIZE 512

using namespace std;

CloudGenerator::CloudGenerator()
{
}

CloudGenerator::~CloudGenerator() {

}

double*** CloudGenerator::calcIntensity(int dimX, int dimY, int dimZ)
{
    m_dimX = dimX;
    m_dimY = dimY;
    m_dimZ = dimZ;

    m_intensity = new double**[dimX];
    for (int x=0; x<dimX; x++) {
        m_intensity[x] = new double*[dimY];
        for (int y=0; y<dimY; y++) {
            m_intensity[x][y] = new double[dimZ];
        }
    }

//    double temp[dimX][dimY][dimZ];

    double numCubes = 4.;
    int numPasses = 4;

    int permutation[512] = {151,160,137,91,90,15,
                    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
                    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
                    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
                    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
                    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
                    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
                    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
                    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
                    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
                    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
                    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
                    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

    int p[512];

    for (int k=0; k<256; k++) {
        p[k] = permutation[k];
        p[256+k] = permutation[k];
    }


    for (int q=0; q<numPasses; q++) {
        for (int i=0; i<dimX; i++) {
            for (int j=0; j<dimY; j++) {
                for (int k=0; k<dimZ; k++) {

                    int X = (int)(floor(j/(dimX/(numCubes*(pow(2, q))))))&255;
                    int Y = (int)(floor(i/(dimY/(numCubes*(pow(2, q))))))&255;
                    int Z = (int)(floor(k/(dimZ/(numCubes*(pow(2, q))))))&255;;

                    double x = (j/(dimX/(numCubes*(pow(2, q)))))-floor(j/(dimX/(numCubes*(pow(2, q)))));
                    double y = (i/(dimY/(numCubes*(pow(2, q)))))-floor(i/(dimY/(numCubes*(pow(2, q)))));
                    double z = (k/(dimZ/(numCubes*(pow(2, q)))))-floor(k/(dimZ/(numCubes*(pow(2, q)))));

                    double u = ((x)*(x)*(x)*((x)*((x)*6.-15.)+10.));
                    double v = ((y)*(y)*(y)*((y)*((y)*6.-15.)+10.));
                    double w = ((z)*(z)*(z)*((z)*((z)*6.-15.)+10.));

                    int A = p[X]+Y;
                    int B = p[X+1]+Y;
                    int AA = p[A]+Z;
                    int AB = p[A+1]+Z;
                    int BA = p[B]+Z;
                    int BB = p[B+1]+Z;

    //                double color = lerp(v, lerp(u, grad(p[A], x, y), grad(p[B], x-1, y)), lerp(u, grad(p[A+1], x, y-1), grad(p[B+1], x-1, y-1)));

                    double color = lerp(w, lerp(v, lerp(u, grad(p[AA], x  , y  , z   ),
                                                        grad(p[BA], x-1, y  , z   )),
                                                lerp(u, grad(p[AB], x  , y-1, z   ),
                                                        grad(p[BB], x-1, y-1, z   ))),
                                        lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ),
                                                        grad(p[BA+1], x-1, y  , z-1 )),
                                                lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
                                                        grad(p[BB+1], x-1, y-1, z-1 ))));

                    if (color < 0) {
                        cout << "negative" << endl;
                    }
                    if (q == 0) {
                        m_intensity[i][j][k] = (double)(min(1., max(0., color)));
                    }
                    else {
                        m_intensity[i][j][k] += (double)((min(1., max(0., color)))/(pow(2, q)));
                    }
                    if (q==numPasses-1) {
                        m_intensity[i][j][k] = (min(1., max(0., m_intensity[i][j][k])));
                    }
                }
            }
        }
    }

    return m_intensity;
}


double CloudGenerator::lerp(double t, double a, double b) {
    //might not need this max thing.... look into this.
//    if (a+(t*(b-a)) < 0.) {

//    }
    return max(0., a+(t*(b-a)));
}

double CloudGenerator::grad(int hash, double x, double y, double z) {
    int h = hash&15;
    double u = (h<8) ? x : y;
    double v = (h<4) ? y : (h==12||h==14) ? x : z;

    return (((h&1) == 0 ? u : -u) + (((h&2) == 0 ? v: -v)));
}
