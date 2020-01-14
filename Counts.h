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
#include "Matrix.h"
#include "Enums.h"

namespace KClmtrBase {
namespace KClmtrNative {
class Counts {
    friend class KClmtr;
public:
    int getTh1() const;                     /**< Thermal count 1  */
    int getTh2() const;                     /**< Thermal count 2  */
    int getTherm() const;                   /**< Thermal count  */
    MeasurementRange getRedRange() const;   /**< Red range  */
    MeasurementRange getGreenRange() const; /**< Green range  */
    MeasurementRange getBlueRange() const;  /**< Blue range  */
    Matrix<int> getTheCounts() const;		/**< 2x3 matrix of the top and bottom counts */
	unsigned int getErrorCode() const;      /**< The error code whenever you are getting data  */

    Counts();
    Counts(const Counts &c);
    Counts(const std::string &s);
    Counts(unsigned int error);
private:
    int th1;
    int th2;
    int therm;
    MeasurementRange redrange;
    MeasurementRange greenrange;
    MeasurementRange bluerange;
    Matrix<int> theCounts;
	unsigned int errorcode;
};
}
}
