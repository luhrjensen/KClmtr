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
#include <vector>

namespace KClmtrBase {
namespace KClmtrNative {
/**
 * @ingroup Structs Structures
 * @brief The Settings for Flicker measurement
 */
struct FlickerSetting {
    struct Range {
        int start;		/**< the start of hz */
        int end;		/**< the end of the Hz */

        int numbers() const {
            return end - start + 1;
        }
        Range() {
            start = 0;
            end = 100;
        }
        Range(int _start, int _end) {
            start = _start;
            end = _end;
        }
        Range(const Range &other) {
            start = other.start;
            end = other.end;
        }
    };
    struct RangeCorrecionArray {
        Range rangeOfHz;
        CorrectionMode mode;
        std::vector<double> array;

        RangeCorrecionArray() {
            rangeOfHz = Range(0, 0);
            mode = CorrectionMode::ArrayFit;
        }
        RangeCorrecionArray(const RangeCorrecionArray &ra) {
            rangeOfHz = ra.rangeOfHz;
            mode = ra.mode;
            array = ra.array;
        }
    };

    int samples; 				/**< Must be a power of 2 between 64 and 2048 */
    int speed; 					/**< How many samples per 1 second */
    int numberOfPeaks;			/**< How many peaks to find */
    bool cosine;				/**< Cosine correction, should be on */
    bool smoothing; 			/**< Averaging, should be on */
    bool JETIADiscount_DB;		/**< JEITA Discount to be applied to dB. It's a human eye responds to the frequency */
    bool JEITADiscount_Percent;	/**< JEITA Discount to be applied to percent. It's a human eye reponds to the frequency */
    DecibelMode decibel; 		/**< The mode of DB */
    PercentMode percent;	 	/**< The mode of the percent */
    //Correction Matrix
    std::vector<RangeCorrecionArray> corrections; /**< The corrections to be used */

    FlickerSetting();
    FlickerSetting(const FlickerSetting &other);
    ~FlickerSetting();

    /**
     * @brief Get the Resolution of the flicker
     *
     * @return the resolution
     */
    double getResolution() const;
    /**
     * @brief Get correction at a Hz
     *
     * @return the correction factor at a hz
     */
    double getCorrection(double hz) const;

    //Array must have the same number as the Range
    /**
     * @brief Append a Correction Matrix to the Settings
     *
     * @param rangeOfHz the range of HZ it applies too
     * @param mode the mode of the correction, a polynomial fit, or a Array of numbers with 1Hz difference between points
     * @param array the array of numbers to be used in the correction
     *
     * @return A bool to tell either it was added or not
     */
    bool appendCorrection(const Range &rangeOfHz, const CorrectionMode &mode, const std::vector<double> &array);
    /**
     * @brief Append a Correction Matrix to the Settings
     *
     * @param rangeOfHz the range of HZ it applies too
     * @param mode the mode of the correction, a polynomial fit, or a Array of numbers with 1Hz difference between points
     * @param array the array of numbers to be used in the correction
     * @param size the size of the array
     *
     * @return A bool to tell either it was added or not
     */
    bool appendCorrection(const Range &rangeOfHz, const CorrectionMode &mode, const double *array, int size);

    /**
    * @brief the deletion of the whole correction
    */
    void deleteCorrection();
    /**
    * @brief to copy one setting to another
    */
    void copy(const FlickerSetting &other);
};

/**
 * @ingroup Structs Structures
 * @brief The Flicker measurement
 * @see KClmtr::getNextFlicker()
 * @see KClmtr::printFlicker()
 */
class Flicker {
    friend class KClmtr;
public:
    double getBigY() const;							/**< The corrected Y from XYZ  */
    MeasurementRange getRange() const;				/**< Range for Green aka for Y */
    const Matrix<double> &getPeakFrequencyPercent() const; /**< The top 3 frequency. HZ, percent*/
    const Matrix<double> &getPeakFrequencyDB() const;		/**< The top 3 frequency. HZ, dB*/
    const Matrix<double> &getFlickerPercent() const;		/**< The Percent value first element is HZ second data  */
    const Matrix<double> &getFlickerDB() const;			/**< The DB value first element is HZ second data  */
    const Matrix<double> &getCounts() const;				/**< The counts over Time */
    const Matrix<double> &getNits() const;					/**< The nits over Time */
    const Matrix<double> &getAmplitude() const;			/**< The amplitude */
	unsigned int getErrorCode() const;				/**< The error code whenever you are getting data  */
    const FlickerSetting &getSettings() const;				/**< The settings used to create the Flicker */
    double getFlickerIndex() const;

    Flicker();
    Flicker(unsigned int error);
    Flicker(const Flicker &f);
private:
    void four1(double data[], int nn, int isign);

    double bigY;
    MeasurementRange range;
    Matrix<double> peakfrequencyPercent;
    Matrix<double> peakfrequencyDB;
    Matrix<double> flickerPercent;
    Matrix<double> flickerDB;
    Matrix<double> counts;
    Matrix<double> nits;
    Matrix<double> amplitude;
	unsigned int errorcode;
    FlickerSetting settings;
    double flickerIndex;

    Flicker(const FlickerSetting &_settings, const double data[], int sectionOfFFT = 0, double nitsLast32Samples = -1);
};
}
}
