/*
KClmtr Object to communicate with Klein K-10/8/1

Copyright (c) 2016 Klein Instruments Inc.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
namespace KClmtrBase {
namespace KClmtrNative {
/**
 * @ingroup Structs Structures
 */
/**
 * @brief Used to store all the XYZ values from a measurement to use in create a calibration file
 * @see KClmtr::getCoefficientTestMatrix()
 * @see KClmtr::storeMatrices()
 */
struct WRGB {
    //white, red, green, blue
    //x, y, z
    double v[4][3]; /**<The matrix is: [white, red, green, blue][X,Y,Z] */
    WRGB() {
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 3; ++j) {
                v[i][j] = 0;
            }
        }
    }
    WRGB(const Measurement m[4]) {
        for(int i = 0; i < 4; ++i) {
            v[i][0] = m[i].getBigX();
            v[i][1] = m[i].getBigY();
            v[i][2] = m[i].getBigZ();
        }
    }
    WRGB(const WRGB &other) {
        for(int i = 0; i < 4; ++i) {
            v[i][0] = other.v[i][0];
            v[i][1] = other.v[i][1];
            v[i][2] = other.v[i][2];
        }
    }
};
}
}