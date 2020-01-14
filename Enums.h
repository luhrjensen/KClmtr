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
#include <sstream>

#ifdef QT_CORE_LIB
#include <QString>
#endif

#ifdef __cplusplus_cli
#define ENUMKEYWORD(NAME, TYPE, ...) \
public enum class NAME : TYPE { \
	__VA_ARGS__ \
};
#else
#define ENUMKEYWORD(NAME, TYPE, ...) \
struct NAME { \
    enum e { __VA_ARGS__ }; \
    explicit NAME(TYPE v) : val(v) {} \
    NAME(e v) : val(v) {} \
    operator e() const { return e(val); } \
        NAME() : val(0) { } \
    private:\
        TYPE val; \
}
#endif
namespace KClmtrBase {
std::string intToString(int value);
void parsingRange(unsigned char range, int(&out)[3]);
ENUMKEYWORD(PercentMode, int,
            ContrastMethod,	/**< AC/DC * 100% * 4, or Peak to Peak */
            Normaized		/**< AC/DC * 100% */
           )
ENUMKEYWORD(DecibelMode, int,
            VESA,			/**< 20 * log10(2 * AC/DC), or JEITA + 3.01dB */
            JEITA			/**< 20 * log10(2^(1/2) * AC/DC) */
           )
ENUMKEYWORD(CorrectionMode, int,
            ArrayFit,
            PolyFit
           )
ENUMKEYWORD(MeasurementRange, int,
            range1 = 1,
            range2 = range1 + 1,
            range3 = range2 + 1,
            range4 = range3 + 1,
            range5 = range4 + 1,
            range6 = range5 + 1,

            range1B = range1,
            range1T = range2,
            range2B = range3,
            range2T = range4,
            range3B = range5,
            range3T = range6
           )
ENUMKEYWORD(GamutCode, int,
            NTSC,
            EBU,
            REC709,
            REC2020,
            SMPTE,
            DCIP3,
            USER_DEFINE,
            defaultGamut = REC709
           )
/**
* @brief
*/
ENUMKEYWORD(SpeedMode, int,
            SPEEDMODE_SLOWEST,/**< 2 Color Measurements per second, 2.0x more accurate than Normal*/
            SPEEDMODE_SLOW,   /**< 4 Color measurements per second, 1.5x more accurate than Normal*/
            SPEEDMODE_NORMAL, /**< Defualt: 8 Color measurements per second */
            SPEEDMODE_FAST    /**< 16 color measurements per second */
           )
/**
* @brief the Event codes from the device. Some events don't need any actions to fix
*
*/
#ifdef __cplusplus_cli
public ref class KleinsErrorCodes {
#else
class KleinsErrorCodes {
#endif
public:
    static const unsigned int NONE = 0x00000000; /**< No Events */
    //Serial Port
    static const unsigned int NOT_OPEN = 0x00000001; /**< Event: The Port isn't open.\n Fix: Use connect() to connect to a port. */
    static const unsigned int TIMED_OUT = 0x00000002; /**< Event: Did not receive the info from port.\n Fix: Try again, or reconnect the device.*/
    static const unsigned int LOST_CONNECTION = 0x00000004; /**< Event: The Device was unplugged during commucation.\n Fix: Please reconnect the device.*/

    //Measurement
    static const unsigned int BAD_VALUES = 0x00000008;
    static const unsigned int CONVERTED_NM = 0x00000010; /**< Event: Couldn't convert XYZ to NM.\n Fix: Nothing, for it's imposable to convert the current XYZ values to NM*/
    static const unsigned int KELVINS = 0x00000020; /**< Event: Couldn't convert XYZ to Kelvins.\n Fix: Nothing, for it's imposable to convert the current XYZ values to Kelvins*/
    static const unsigned int AIMING_LIGHTS = 0x00000040; /**< Event: The Aiming lights are on.\n Fix: Turn off the aimming lights with the Knob */
    static const unsigned int AVERAGING_LOW_LIGHT = 0x00000080; /**< Event: The Device needs more time to average because the light level is low.\n Fix: Nothing, have the device keep measuring to have a more accurate measurement.*/

    //Ranges
    static const unsigned int BOTTOM_UNDER_RANGE = 0x00000100; /**< Event: The device switched ranges.\n Fix: Nothing. */
    static const unsigned int TOP_OVER_RANGE = 0x00000200; /**< Event: The device switched ranges.\n Fix: Nothing. */
    static const unsigned int OVER_HIGH_RANGE = 0x00000400; /**< Event: The device switched ranges.\n Fix: Nothing */

    //Black Cals
    static const unsigned int BLACK_ZERO = 0x00000800;
    static const unsigned int BLACK_OVERDRIVE = 0x00001000;
    static const unsigned int BLACK_EXCESSIVE = 0x00002000;
    static const unsigned int BLACK_PARSING_ROM = 0x00004000; /**< Event: Creating Black cal into the device failed\n Fix: Rerun the BlackCal */
    static const unsigned int BLACK_STORING_ROM = 0x00008000; /**< Event: Storing the Black cal failed\n Fix: Rerun the BlackCal*/

    //CalFiles
    static const unsigned int CAL_WHITE_RGB = 0x00010000; /**< Event: There was a problem with converting the white spec and RGB\n Fix: Rerun Calibration*/
    static const unsigned int CAL_STORING = 0x00020000; /**< Event: Storing the CalFile failed\n Fix: Re-Store calibration*/
    static const unsigned int CAL_CONVERT_BINARY = 0x00040000; /**< Event: Couldn't convert the numbers to a string to be stored\n Fix: Rerun calibration*/

    //FFT
    static const unsigned int FFT_BAD_STRING = 0x00080000; /**< Event: FFT received a bad string\n Fix: Re-Start Flicker*/
    static const unsigned int FFT_RANGE_CAL = 0x00100000; /**< Event: FFT received a bad rang cal file\n Fix: Contact Klein Instruments*/
    static const unsigned int FFT_NO_XYZ = 0x00200000; /**< Event: There was no measurement in the FFT string\n Fix: Re-start Flicker*/
    static const unsigned int FFT_NO_RANGE = 0x00400000; /**< Event: There was no range in FFT\n Fix: Re-start Flicker*/
    static const unsigned int FFT_INSUFFICIENT_DATA = 0x00800000; /**< Event: There is not enough data to display FFT\n Fix: Continue gather Flicker data*/
    static const unsigned int FFT_PREVIOUS_RANGE = 0x01000000; /**< Event: The device is switching ranges\n Fix: Ingore this Flicker Data, and continue gather Flicker Data*/
    static const unsigned int FFT_NOT_SUPPORTED = 0x02000000; /**< Event: The device that you are currently hooked up to cannot do FFT\n Fix: Can't use this device on Flicker*/
    static const unsigned int FFT_BAD_SAMPLES = 0x04000000; /**< Event: FFT received a bad string\n Fix: Re-Start Flicker*/
    static const unsigned int FFT_OVER_SATURATED = 0x08000000; /**< Event: The counts are too high for the current range to get good data\n Fix: Skip this Flicker measurement, or change ranges if you have locked the range */

    //Miscellaneous
    static const unsigned int FIRMWARE = 0x10000000;
    static const unsigned int NEGATIVE_VALUES = 0x20000000;
    /**
    * @brief Infomational codes that reported and are generally to be ignored
    */
    static const unsigned int ingoreWarningList = KleinsErrorCodes::BOTTOM_UNDER_RANGE |     /*Range change*/
                                                  KleinsErrorCodes::TOP_OVER_RANGE |         /*Range change*/
                                                  KleinsErrorCodes::OVER_HIGH_RANGE |        /*Range change*/
                                                  KleinsErrorCodes::FFT_PREVIOUS_RANGE |     /*Range change*/
                                                  KleinsErrorCodes::FFT_INSUFFICIENT_DATA |  /*Not enough FFT data*/
                                                  KleinsErrorCodes::FFT_OVER_SATURATED |     /*Range changing soon*/
                                                  KleinsErrorCodes::CONVERTED_NM |           /*Can't use NM but everything else is good*/
                                                  KleinsErrorCodes::KELVINS |                /*Can't use Kleivens but everything else is good*/
                                                  KleinsErrorCodes::AVERAGING_LOW_LIGHT |    /*Averaging, just need to get more data*/
                                                  KleinsErrorCodes::NEGATIVE_VALUES;         /*Values from the device was negative and out of noise range*/
    /**
    * @brief To help change int errocodes to reable strings to output to the user
    */
#ifdef __cplusplus_cli
	static System::String^ errorCodesToString(unsigned int errorcode);
	static System::String^ errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode);
#elif QT_CORE_LIB
    static QString errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode = ingoreWarningList);
#elif NSAppKitVersionNumber10_0
    static NSString *errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode = ingoreWarningList);
#else
    static std::string errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode = ingoreWarningList);
#endif
    /**
    * @brief To help to know when to stop the measurement cycle, or keep going
    */
#ifdef __cplusplus_cli
    static bool shouldStopMeasuring(unsigned int errorcode);
    static bool shouldStopMeasuring(unsigned int errorcode, unsigned int ingoreErrorcode);
#else
    static bool shouldStopMeasuring(unsigned int errorcode, unsigned int ingoreErrorcode = ingoreWarningList);
#endif
    static unsigned int getErrorCodeFromKlein(std::string MyError);
};
}
