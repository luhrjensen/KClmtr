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
#include "Counts.h"
#include "Enums.h"
using namespace KClmtrBase;
using namespace KClmtrBase::KClmtrNative;

Counts::Counts() {
    th1 = 0;
    th2 = 0;
    therm = 0;
    theCounts.initializeV(2, 3);
    errorcode = 0;
}
Counts::Counts(const Counts &c) {
    th1 = c.th1;
    th2 = c.th2;
    therm = c.therm;
    redrange = c.redrange;
    greenrange = c.greenrange;
    bluerange = c.bluerange;
    theCounts = c.theCounts;
    errorcode = c.errorcode;
}
Counts::Counts(unsigned int error) {
    th1 = 0;
    th2 = 0;
    therm = 0;
    theCounts.initializeV(2, 3);
    errorcode = 0;
    errorcode = error;
}
Counts::Counts(const std::string &s) {
    th1 = 0;
    th2 = 0;
    therm = 0;
    theCounts.initializeV(2, 3);
    errorcode = 0;

    const unsigned char *myRead = (const unsigned char *)s.c_str();

    theCounts.v[0][0] = (int)myRead[0];
    theCounts.v[0][0] = theCounts.v[0][0] * 256 + (int)myRead[1];

    theCounts.v[0][1] = (int)myRead[2];
    theCounts.v[0][1] = theCounts.v[0][1] * 256 + (int)myRead[3];

    theCounts.v[0][2] = (int)myRead[4];
    theCounts.v[0][2] = theCounts.v[0][2] * 256 + (int)myRead[5];

    theCounts.v[1][0] = (int)myRead[6];
    theCounts.v[1][0] = theCounts.v[1][0] * 256 + (int)myRead[7];

    theCounts.v[1][1] = (int)myRead[8];
    theCounts.v[1][1] = theCounts.v[1][1] * 256 + (int)myRead[9];

    theCounts.v[1][2] = (int)myRead[10];
    theCounts.v[1][2] = theCounts.v[1][2] * 256 + (int)myRead[11];

    therm = (int)myRead[12];
    therm = therm * 256 + (int)myRead[13];

    th1 = (int)myRead[14];
    th2 = (int)myRead[15];

    //Error
    errorcode = KleinsErrorCodes::getErrorCodeFromKlein(s.substr(18, 1));

    //Ranges
    int ranges[3];
    parsingRange(myRead[16], ranges);
    redrange = (MeasurementRange)ranges[0];
    greenrange = (MeasurementRange)ranges[1];
    bluerange = (MeasurementRange)ranges[2];
}

int Counts::getTh1() const {
    return th1;
}
int Counts::getTh2() const {
    return th2;
}
int Counts::getTherm() const {
    return therm;
}
MeasurementRange Counts::getRedRange() const {
    return redrange;
}
MeasurementRange Counts::getGreenRange() const {
    return greenrange;
}
MeasurementRange Counts::getBlueRange() const {
    return bluerange;
}
Matrix<int> Counts::getTheCounts() const {
    return theCounts;
}
unsigned int Counts::getErrorCode() const {
    return errorcode;
}
