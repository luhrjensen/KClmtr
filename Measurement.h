/*
KClmtr Object to communicate with Klein K-101

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

#define ABS(v) ((v)<0 ? -(v) : (v))

namespace KClmtrBase {
namespace KClmtrNative {
/**
* @brief Use to make a Gamut from the primaries, and white to calculate RGB and Hue and Saturation
* @see Measurement
*/
class GamutSpec {
public:
    /**
    * @brief To be used in fromCode to quickly get static values of the gamut spect
    * @see fromCode
    */

    GamutSpec();
    GamutSpec(const GamutSpec &gs);
    ~GamutSpec() {}
    /**
    * @brief Creates gamut out of a full spec
    * @param redX - in CIE 1931 space
    * @param redY - in CIE 1931 space
    * @param greenX - in CIE 1931 space
    * @param greenY - in CIE 1931 space
    * @param blueX - in CIE 1931 space
    * @param blueY - in CIE 1931 space
    * @param whiteX - in CIE 1931 space
    * @param whiteY - in CIE 1931 space
    * @param whiteBigY - in nits
    */
    GamutSpec(double redX, double redY,
              double greenX, double greenY,
              double blueX, double blueY,
              double whiteX, double whiteY, double whiteBigY);
    /**
    * @brief Creates a gamut out of predefined gamutSpecs
    */
    static GamutSpec fromCode(GamutCode code, double whiteBigY = 100.0);

    /**
    * @brief returns code if predefined
    */
    GamutCode getCodeSpec();

    /**
    * @brief To set Red primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    */
    void setRed(double x, double y);
    /**
    * @brief To set Green primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    */
    void setGreen(double x, double y);
    /**
    * @brief To set Blue primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    */
    void setBlue(double x, double y);
    /**
    * @brief To set White primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    * @param bigY - in nits
    */
    void setWhite(double x, double y, double bigY);

    /**
    * @brief To get White primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    * @param bigY - in nits
    */
    void getWhite(double &x, double &y, double &bigY) const;
    /**
    * @brief To get Red primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    * @param bigY - in nits
    */
    void getRed(double &x, double &y, double &bigY) const;
    /**
    * @brief To get Green primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    * @param bigY - in nits
    */
    void getGreen(double &x, double &y, double &bigY) const;
    /**
    * @brief To get Blue primary
    * @param x - in CIE 1931 space
    * @param y - in CIE 1931 space
    * @param bigY - in nits
    */
    void getBlue(double &x, double &y, double &bigY) const;

    /**
    * @brief gets the 3x3 matrix to convert RGB percents (0.0 - 1.0) to XYZ values
    */
    Matrix<double> getRGBtoXYZ() const;
    /**
    * @brief gets the 3x3 matrix to convert XYZ values to RGB percents (0.0 - 1.0)
    */
    Matrix<double> getXYZtoRGB() const;

private:
    double _redX;
    double _redY;
    double _redBigY;
    double _greenX;
    double _greenY;
    double _greenBigY;
    double _blueX;
    double _blueY;
    double _blueBigY;
    double _whiteX;
    double _whiteY;
    double _whiteBigY;

    GamutCode _code;

    void updateMatrixes();
    void checkGamutCode();
    void reduceRow(double **mat, int n, int lenght);
    Matrix<double> RGBtoXYZ;
    Matrix<double> XYZtoRGB;

    bool operator ==(const GamutSpec &other);
};
/**
* @brief To know which the range for the XYZ filters
*/

/**
 * @brief Every unit for a measurement
 * @see KClmtr::getNextMeasurement()
 * @see KClmtr::printMeasure()
 */
class Measurement {
    friend class KClmtr;
public:
    Measurement();
    Measurement(const Measurement &m);

    double getCIE1931_x() const;
    double getCIE1931_y() const;
    double getBigX() const;
    double getBigY() const;
    double getBigZ() const;
    double getBigXRaw() const;
    double getBigYRaw() const;
    double getBigZRaw() const;
    double getRGB_Red() const;
    double getRGB_Green() const;
    double getRGB_Blue() const;
    double getCIE1974_u() const;
    double getCIE1974_v() const;
    double getWavelength_nm() const;
    double getWavelength_duv() const;
    double getLab_L()  const;
    double getLab_a() const;
    double getLab_b() const;
    double getLCh_L()  const;
    double getLCh_C() const;
    double getLCh_h() const;
    double getHSV_hue() const;
    double getHSV_saturation() const;
    double getHSV_value() const;
    MeasurementRange getRedRange() const;
    MeasurementRange getGreenRange() const;
    MeasurementRange getBlueRange() const;
    double getColorTemputure_K() const;
    double getColorTemputure_duv() const;	/**< The Color Temputers distance off the black body curve */
    unsigned int getErrorCode() const;		/**< The number of measurements that was averaged togather */
    int getAveragingby() const;				/**< The number of measurements that was averaged togather	*/
    double getMinX() const;					/**< The min X in the XYZ, from all the measurements that was averaged togather*/
    double getMaxX() const;					/**< The max X in the XYZ, from all the measurements that was averaged togather*/
    double getMinY() const;					/**< The min Y in the XYZ, from all the measurements that was averaged togather*/
    double getMaxY() const;					/**< The max Y in the XYZ, from all the measurements that was averaged togather*/
    double getMinZ() const;					/**< The min z in the XYZ, from all the measurements that was averaged togather*/
    double getMaxZ() const;					/**< The max Z in the XYZ, from all the measurements that was averaged togather*/
    GamutSpec getGamutSpec() const;			/**< brief get the gamutSpec to be used to calculate the RGB and Hue/Saturation values */
    /**
    * @brief set the gamutSpec to be used to calculate the RGB and Hue/Saturation values
    * @param gs - the spec
    */
    void setGamutSpec(const GamutSpec &gs);
    /**
    * @brief returns the DeltaE of 1976
    * @param spec - the measurement strut it is comparing it to.
    */
    double deltaE1976(const Measurement &spec) const;
    /**
    * @brief returns the DeltaE of 1994
    * @param spec - the measurement strut it is comparing it to.
    */
    double deltaE1994(const Measurement &spec) const;
    /**
    * @brief returns the DeltaE of 2000
    * @param spec - the measurement strut it is comparing it to.
    */
    double deltaE2000(const Measurement &spec) const;
    /**
    * @brief To return a bad Measurement in the ErrorCode
    */
    static Measurement fromError(int error);
    /**
    * @brief Create a Measurement Structure out of XYZ
    *
    * @param X
    * @param Y
    * @param Z
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromXYZ(double X, double Y, double Z, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief Create a Measurement Structure out of xyL
    *
    * @param x
    * @param y
    * @param Y
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromxyY(double x, double y, double Y, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief Create a Measurement Structure out of u'v'L
    *
    * @param u
    * @param v
    * @param Y
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromuvprimeY(double u, double v, double Y, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief Create a Measurement Structure out of temp duv
    *
    * @param _temp
    * @param _tempduv
    * @param Y
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromTempduvY(double _temp, double _tempduv, double Y, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief Create a Measurement Structure out of nm du'v'
    *
    * @param _nm
    * @param _nmduv
    * @param Y
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromnmduvY(double _nm, double _nmduv, double Y, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief Create a Measurement Structure out of XYZ
    *
    * @param red
    * @param green
    * @param blue
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromRGB(double red, double green, double blue, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief Create a Measurement Structure out of XYZ
    *
    * @param hue
    * @param saturation
    * @param value
    * @param gs to correctly get the RGB and HueSat values
	* @param error the error to be added to the measurement
    */
    static Measurement fromHSV(double hue, double saturation, double value, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
     * @brief fromLab
     * @param L
     * @param a
     * @param b
     * @param gs
	 * @param error the error to be added to the measurement
     */
    static Measurement fromLab(double L, double a, double b, const GamutSpec &gs = GamutSpec::fromCode(GamutCode::defaultGamut), int error = 0);
    /**
    * @brief converts Foot Lamberts into nits
    * @param FootLamberts
    */
    static double toNits(double FootLamberts);
    /**
    * @brief converts nits into Foot Lamberts
    * @param nits
    */
    static double toFootLamberts(double nits);

private:
    double x;
    double y;
    double bigx;
    double bigy;
    double bigz;
    double red;
    double green;
    double blue;
    double u;
    double v;
    double nm;
    double nmduv;
    double L;
    double a;
    double b;
    double C;
    double h;
    double hue;
    double saturation;
    double value;
    double temp;
    double tempduv;
    GamutSpec gs;

    //KClmtr can set this
    double bigxraw;
    double bigyraw;
    double bigzraw;
	unsigned int errorcode;
    MeasurementRange redrange;
    MeasurementRange greenrange;
    MeasurementRange bluerange;
    int averagingby;
    double minX;
    double maxX;
    double minY;
    double maxY;
    double minZ;
    double maxZ;

    //Main function to change XYZ to all others
    void computeDerivativeData(double _bigX, double _bigY, double _bigZ, const GamutSpec &_gs);

    static void projectOntoCurve(double u, double v, double whiteU, double whiteV, const double curve[][8], int points, double &out, double &outduv, unsigned int &errorcode, unsigned int errorflag);
    static bool findOnCurve(double value, const double curve[][8], int points, double &u, double &v, double &du, double &dv);
    static void unitVector(double &x, double &y);
    static double labF(double t);
    static double invertLabF(double t);
};
}
}
