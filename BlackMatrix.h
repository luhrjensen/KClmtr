/*
KClmtr Object to communicate with Klein K-10/8/1

Copyright (c) 2017 Klein Instruments Inc.

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
#include <string>
#include "Enums.h"
namespace KClmtrBase {
namespace KClmtrNative {
/**
 * @ingroup Structs Structures
 */
/**
 * @brief Used for any of the Black Calibration Methods in KClmtr
 * @see KClmtr::captureBlackLevel()
 * @see KClmtr::recallFlashMatrix()
 * @see KClmtr::recallRAMMatrix()
 * @see KClmtr::recallCoefficientMatrix()
 */
struct BlackMatrix {
    double range[6][3];      /**< The Black Matrix for xyz in all 6 ranges */
    double therm;			 /**< The tempaure of the Black Matrix */
    unsigned int errorcode;           /**< The error code whenever you are getting data  */

    BlackMatrix() {
        for(int i = 0; i < 6; ++i) {
            for(int j = 0; j < 3; ++j) {
                range[i][j] = 0;
            }
        }
        therm = 0;
        errorcode = 0;
    }
    BlackMatrix(unsigned int error) {
        for(int i = 0; i < 6; ++i) {
            for(int j = 0; j < 3; ++j) {
                range[i][j] = 0;
            }
        }
        therm = 0;
        errorcode = error;
    }
    BlackMatrix(double matrix[19], unsigned int  error) {
        fromMatrix(matrix);
        errorcode = 0;
        errorcode = error;
    }
    BlackMatrix(const std::string &read, unsigned int  error) {
        double matrix[19];
        const unsigned char *myRead = (const unsigned char *)read.c_str();
        for(int i = 0; i < 19; ++i) {
            matrix[i] = (int)myRead[(i + 1) * 2];
            matrix[i] = matrix[i] * 256 + (int)myRead[(i + 1) * 2 + 1];
            if(matrix[i] < 500 || matrix[i] > 2500) {
                i = 19;
                errorcode |= KleinsErrorCodes::BLACK_PARSING_ROM;
                for(int j = 0; j < 19; ++j) {
                    matrix[j] = 0;
                }
            }
        }
        fromMatrix(matrix);
        errorcode = error | KleinsErrorCodes::getErrorCodeFromKlein(read.substr(41, 1));
    }
    void fromMatrix(double matrix[19]) {
        for(int i = 0; i < 3; ++i) {
            range[5][i] = matrix[i + 0];
            range[4][i] = matrix[i + 3];
            range[3][i] = matrix[i + 6];
            range[2][i] = matrix[i + 9];
            range[1][i] = matrix[i + 12];
            range[0][i] = matrix[i + 15];
        }
        therm = matrix[18];
    }

    void toMatrix(double matrix[19]) {
        for(int i = 0; i < 3; ++i) {
            matrix[i + 0] = range[5][i];
            matrix[i + 3] = range[4][i];
            matrix[i + 6] = range[3][i];
            matrix[i + 9] = range[2][i];
            matrix[i + 12] = range[1][i];
            matrix[i + 15] = range[0][i];
        }
        matrix[18] = therm;
    }
};
}
}

