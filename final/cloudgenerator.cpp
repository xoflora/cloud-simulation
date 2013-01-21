#include "cloudgenerator.h"
#include <qgl.h>
#include <math.h>
#include <iostream>

#define SIZE 512 //defines the cube size

using namespace std;

CloudGenerator::CloudGenerator()
{
}

CloudGenerator::~CloudGenerator()
{
    for (int x=0; x<m_dimX; x++)
    {
        for (int y=0; y<m_dimY; y++)
        {
            delete[] m_intensity[x][y];
        }

        delete[] m_intensity[x];
    }
    delete m_intensity;
}

double*** CloudGenerator::calcIntensity(int dimX, int dimY, int dimZ)
{
    m_dimX = dimX;
    m_dimY = dimY;
    m_dimZ = dimZ;

    //makes the 3d array structure
    m_intensity = new double**[dimX];
    for (int x=0; x<dimX; x++)
    {
        m_intensity[x] = new double*[dimY];

        for (int y=0; y<dimY; y++)
        {
            m_intensity[x][y] = new double[dimZ];
        }
    }

    double numCubes = 4; //affects the size of the cube
    int numPasses = 4; //number of passes we make (how many perlin functions we accumulate)

    //our psuedo-random number array used for perlin generation
    int randomNums[512] = {151,160,137,91,90,15,
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

    for (int q=0; q<numPasses; q++)
    {
        for (int i=0; i<dimX; i++)
        {
            for (int j=0; j<dimY; j++)
            {
                for (int k=0; k<dimZ; k++)
                {

                    //gets unit cube coordinates for the cube our pixel is in
                    int X = (int)(floor(j/(dimX/(numCubes*(pow(2, q))))))&255;
                    int Y = (int)(floor(i/(dimY/(numCubes*(pow(2, q))))))&255;
                    int Z = (int)(floor(k/(dimZ/(numCubes*(pow(2, q))))))&255;;

                    //offset of the pixel we're looking at within its unit cube
                    double x = (j/(dimX/(numCubes*(pow(2, q)))))-floor(j/(dimX/(numCubes*(pow(2, q)))));
                    double y = (i/(dimY/(numCubes*(pow(2, q)))))-floor(i/(dimY/(numCubes*(pow(2, q)))));
                    double z = (k/(dimZ/(numCubes*(pow(2, q)))))-floor(k/(dimZ/(numCubes*(pow(2, q)))));

                    double u = ((x)*(x)*(x)*((x)*((x)*6.-15.)+10.));
                    double v = ((y)*(y)*(y)*((y)*((y)*6.-15.)+10.));
                    double w = ((z)*(z)*(z)*((z)*((z)*6.-15.)+10.));

                    //makes "random" vectors for each of the corners
                    int A = randomNums[X]+Y;
                    int B = randomNums[X+1]+Y;
                    int AA = randomNums[A]+Z;
                    int AB = randomNums[A+1]+Z;
                    int BA = randomNums[B]+Z;
                    int BB = randomNums[B+1]+Z;

                    double color = lerp(w, lerp(v, lerp(u, grad(randomNums[AA], x  , y  , z   ),
                                                        grad(randomNums[BA], x-1, y  , z   )),
                                                lerp(u, grad(randomNums[AB], x  , y-1, z   ),
                                                        grad(randomNums[BB], x-1, y-1, z   ))),
                                        lerp(v, lerp(u, grad(randomNums[AA+1], x  , y  , z-1 ),
                                                        grad(randomNums[BA+1], x-1, y  , z-1 )),
                                                lerp(u, grad(randomNums[AB+1], x  , y-1, z-1 ),
                                                        grad(randomNums[BB+1], x-1, y-1, z-1 ))));

                    if (q == 0) {
                        //first pass
                        m_intensity[i][j][k] = (double)(min(1., max(0., color)));
                    }
                    else {
                        //weigh each pass depending on the number of cubes it used for its grid
                        m_intensity[i][j][k] += (double)((min(1., max(0., color)))/(pow(2, q)));
                    }
                    if (q==numPasses-1) {
                        //cap intensities at 1
                        m_intensity[i][j][k] = (min(1., max(0., m_intensity[i][j][k])));
                    }
                }
            }
        }
    }

    return m_intensity;
}

/**
  Linear interpolation
  */
double CloudGenerator::lerp(double t, double a, double b)
{
    return max(0., a+(t*(b-a)));
}

/**
  Gradient
  */
double CloudGenerator::grad(int hash, double x, double y, double z)
{
    int h = hash & 15;
    double u = (h < 8) ? x : y;
    double v = (h < 4) ? y : (h == 12||h == 14) ? x : z;

    return (((h & 1) == 0 ? u : -u) + (((h & 2) == 0 ? v: -v)));
}
