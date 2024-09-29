#pragma once

#include "Math/Vector3.h"
#include <iostream>

class HDRData
{
public:
    HDRData() : width(0), height(0), cols(nullptr), marginalDistData(nullptr), conditionalDistData(nullptr)
    {
    }

    ~HDRData()
    {
        delete cols;
        delete marginalDistData;
        delete conditionalDistData;
    }

    int width, height;
    // each pixel takes 3 float32, each component can be of any value...
    float *cols;
    Vector3f *marginalDistData;    // y component holds the pdf
    Vector3f *conditionalDistData; // y component holds the pdf
};

HDRData *LoadHDR(const char *fileName);