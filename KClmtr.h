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

#include "string.h"

#include "SerialPort.h"
#include "BlackMatrix.h"
#include "Flicker.h"
#include "Measurement.h"
#include "Counts.h"
#include "WRGB.h"
#include "Matrix.h"
#include "Enums.h"

#ifdef  WIN32
#else
#include <pthread.h>
#endif

namespace KClmtrBase {
namespace KClmtrNative {
static const int polyDegree = 5; //Including 0th term

/**
* @brief The coefficient matrices
* @see KClmtr::getCoefficientTestMatrix()
* @see KClmtr::setTempCalFile()
*
*/
struct CorrectedCoefficient {
    Matrix<double> colorMatrix;   /**< The Color Coefficient Matrix */
    Matrix<double> rgbMatrix;     /**< The RGB Coefficient Matrix */
    unsigned int error;

    CorrectedCoefficient() {
        colorMatrix.initializeV(3, 3);
        rgbMatrix.initializeV(3, 3);
        error = 0;
    }
};
/**
 * @brief Object to control a Klein Device
 *
 */
class KClmtr {
public:
    /**
    * @brief constructor
    */
    KClmtr();
    /**
     * @brief closePort() and Destroys KClmtr
     */
    virtual ~KClmtr();

    //Properties
    /**
     * @brief Sets Device's name to be hooked up to.
     *
     * @param portName Full Device Name
     */
    void setPort(const std::string &portName);
    /**
     * @brief Gets the port Name which KClmtr is hooked up to, or about to hook up to.
     *
     * @return const string Full Device Name
     */
    std::string getPort() const;
    /**
     * @brief Returns true if the device is currently hooked up. Returns false if the device is not currently hooked up.
     *
     * @return bool
     */
    bool isPortOpen();	//Can't be const, for it needs to call API
    /**
     * @brief After KClmtr is open, returns the Serial Number of the Klein Device
     *
     * @return const string Serial Number
     */
    std::string getSerialNumber() const;
    /**
     * @brief After KClmtr is open, returns the Model Number of the Klein Device
     *
     * @return const string Model Number
     */
    std::string getModel() const;
    /**
     * @brief Turns on or off the aiming lights, if the Klein Device has a set of aiming lights. This gets over written if a measure or a flicker command sent
     *
     * @param onOff True turns on the Aiming Lights. False turns off the aiming lights
     */
    void setAimingLights(bool onOff);
    /**
     * @brief Set the range the Device needs to stay in
     *
     * @param range Ranges are 1 to 6\n
     * -1 will turn on AutoRange(by default AutoRange is on)\n
     * 0  will lock it in the current range
     */
    void setRange(int range);
    /**
     * @brief get the range the Device needs to stay in
     *
     * @return range Ranges are 1 to 6, -1 will turn on AutoRange(by default AutoRange is on)
     */
    int getRange() const;
    /**
     * @brief set the object to check for if measurment is in nosie range and return a zero\n
     * for negative values in the measurments. If the device reads less than accpable noise\n
     * level, it will return a warning to calibreate black levels.
     *
     * @param zeroNoise (by default zeroNoise is off)
     */
    void setZeroNoise(const bool zeroNoise);
    /**
     * @brief get the status of Zering out Negative Noise
     *
     * @return the current status of zeroNoise (by default zeroNoise is off)
     */
    bool getZeroNoise();
    //CalFiles
    /**
     * @brief Returns the calibration file's name that the KClmtr is currently using.
     *
     * @return const string Name of the calibration file
     */
    std::string getCalFileName() const;
    /**
     * @brief Sets the calibration file by the Calibration File's ID.
     * @see getCalFileList() to find the list of calibration files
     * @param calFileID	The index of the Cal File(or profile)
     */
    void setCalFileID(int calFileID);
    /**
     * @brief Returns the calibration file's name that the KClmtr is currently using.
     *
     * @return int ID of the calibration file
     */
    int getCalFileID() const;
    /**
     * @brief Returns the 3x3 matrix of the calibration file that the KClmtr is currently using.
     *
     * @return const matrix
     */
    Matrix<double> getCalMatrix() const;
    /**
     * @brief Returns the RGB 3x3 matrix of the clibartion file that the KClmtr is currently using
     *
     * @return const matrix
     */
    Matrix<double> getRGBMatrix() const;
    /**
    * @brief Gets the current gamut spec that the KClmtr is using to create the RGB Matrix, getcalMatrix().
    */
    GamutSpec getGamutSpec() const;
    /**
     * @brief Modifies the current gamut spec that the KClmtr is using to create the RGB Matrix, getcalMatrix().
     * @param gs - the new spec
     */
    void setGamutSpec(const GamutSpec &gs);
    /**
     * @brief Returns a string array of the calibration files on the KClmtr. There are 96 calibration files.
     *
     * @return const string array
     */
    std::vector<std::string> getCalFileList() const;
    /**
     * @brief Set a temporary calibration file with out saving it to the device, and will start using it. getCalFileID() will return 0, and getCalFileName() will return "Temporary Cal File".
     *
     * @param coeff
     */
    void setTempCalFile(const CorrectedCoefficient &coeff);

    //FFT
    /**
     * @brief Set to use the cosine correction in the flicker output
     *
     * @param use
     */
    void setFFT_Cosine(bool use);
    /**
     * @brief Get the use of the cosine correction during flicker
     *
     * @return bool
     */
    bool getFFT_Cosine() const;
    /**
     * @brief Set to use smoothing in the output of flicker. Smooths out the line of flicker
     *
     * @param use
     */
    void setFFT_Smoothing(bool use);
    /**
     * @brief Get the use of smooth in the output of flicker.
     *
     * @return bool
     */
    bool getFFT_Smoothing() const;
	void setDeviceFlickerSpeed(bool use);
	bool getDeviceFlickerSpeed() const;
    /**
     * @brief Get how many samples need for flicker data needs before returning flicker
     *
     * @return samples 256 samples -  1  second\n
     *                128 samples - .5  seconds\n
     *                64  samples - .25 seconds\n
     */
    int getFFT_Samples() const;
    /**
     * @brief Set how many samples need for flicker data needs before returning flicker power of 2, between 64-2048
     *
     * @param samples 256 samples -  1  second\n
     *                128 samples - .5  seconds\n
     *                64  samples - .25 seconds\n
     */
    int setFFT_Samples(int samples);
    /**
     * @brief Get JEITA Discount for Percent
     *
     * @return True = On, False = Off
     */
    bool getFFT_PercentJEITA_Discount() const;
    /**
     * @brief Set JEITA Discount for Percent
     *
     * @param onOff True = on, False = off
     */
    void setFFT_PercentJEITA_Discount(bool onOff);
    /**
     * @brief Get JEITA Discount for dB
     *
     * @return True = On, False = Off
     */
    bool getFFT_DBJEITA_Discount() const;
    /**
     * @brief Set JEITA Discount for dB
     *
     * @param onOff True = on, False = off
     */
    void setFFT_DBJEITA_Discount(bool onOff);
    /**
     * @brief Get Percent's Calculation for FMA or normalized\n
     *       FMA		= which will 4 * AC/DC * 100, or Peak to peak\n
     *       Normalized = AC/DC * 100
     *
     * @return the mode it is in
     */
    PercentMode getFFT_PercentMode() const;
    /**
     * @brief Set Percent's Calculation for FMA or normalized\n
     *       FMA		= which will 4 * AC/DC * 100, or Peak to peak\n
     *       Normalized = AC/DC * 100
     *
     * @param mode the mode switch to set it too
     */
    void setFFT_PercentMode(PercentMode mode);
    /**
     * @brief Get dB's Calculation for VISA\n
     *        VISA = 20 * log10(2 * AC/DC) = JEITA + 3.01dB\n
     *       JETIA = 20 * log10(2^(1/2) * AC/DC)
     *
     * @return mode the mode it is in
     */
    DecibelMode getFFT_DBMode() const;
    /**
     * @brief Set dB's Calculation for VISA\n
     *        VISA = 20 * log10(2 * AC/DC) = JEITA + 3.01dB\n
     *       JETIA = 20 * log10(2^(1/2) * AC/DC)
     *
     * @param mode the mode switch to set it too
     */
    void setFFT_DBMode(DecibelMode mode);
    /**
    * @brief Set how many peaks to check for
    * @param numberOfPeaks the number of peaks
    */
    void setFFT_numberOfPeaks(int numberOfPeaks);
    /**
    * @brief Gets how many number of peaks to check
    */
    int getFFT_numberOfPeaks() const;
    //XYZ
    /**
     * @brief Set Max Averaging measurements for lowlight all measurements\n
     * NOTE: This will stop measurements. You will have to restart Measuring\n
     * Max value: 128 measurement, or 16 seconds\n
     * Min value:   1 measurement, or 1/8th of a second(no Average)\n
     * Defualt setting: 32 measurements, or 4 seconds
     *
     * @return it was set or not
     *
     */
    bool setMaxAverageCount(int maxAvg);
    /**
     * @brief get Max Averaging measurements
     *
     */
    int getMaxAverageCount() const;
    /**
    * @brief get the speed of the color measurments
    *
    * @see speedMode
    * return speedMode
    */
    SpeedMode getMeasureSpeedMode() const;
    /**
    * @brief sets the speed of the color measurements
    * @see speedMode
    */
    void setMeasureSpeedMode(SpeedMode value);
    /**
     * @brief Starts the Klein device to measure constantly.
     *
     */
    void startMeasuring();
    /**
     * @brief Stops the Klein device to stop measuring.
     *
     */
    void stopMeasuring();
    /**
     * @brief Returns true if the device is measuring mode. Returns false if it is not in measuring mode.
     *
     * @return bool
     */
    bool isMeasuring() const;
    /**
    * @brief Returns one measurement from the device that has been stored in the buffer of the class. You must use startMeasureing() to use this method.
    * @param m the Measurement struct that will be stored in to
    * @returns bool isFresh tells weither or not that the Measurement is something the program already grabbed or not
    */
    bool getMeasurement(Measurement &m);
    /**
     * @brief Returns one measurement from the device. Do not need to startMeasuring() to use this method.
     * @param n Average number of measurements togather to return one measurement\n
     * if less than 1 measure auto for black level AVERAGING_LOW_LIGHT
     * @see setMaxAverageCount
     * @return Measurement
     */
    Measurement getNextMeasurement(int n = -1);

    /**
     * @brief Getting the inverse sensitivity of the device compaired to the K-10 SF
     * @return
     */
    double modelSensitivityMultiplier();
    /**
     * @brief Getting the sensitivity of the device compaired to the K-10 SF
     * @return
     */
    double modelSensitivity();
    //Counts
    /**
    * @brief Starts the Klein device to get counts constantly.
    *
    */
    void startMeasureCounts();
    /**
    * @brief Stops the Klein device to stop getting counts.
    *
    */
    void stopMeasureCounts();
    /**
    * @brief Returns true if the device is counts mode. Returns false if it is not in counts mode.
    *
    * @return bool
    */
    bool isMeasureCounts() const;
    /**
    * @brief Returns one measurement from the device that has been stored in the buffer of the class. You must use startMeasureing() to use this method.
    * @param c the Measurement struct that will be stored in to
    * @returns bool isFresh tells weither or not that the Measurement is something the program already grabbed or not
    */
    bool getMeasureCounts(Counts &c);
    /**
    * @brief Returns one measurement from the device. Do not need to startCount() to use this method.
    * @return Counts
    */
    Counts getNextMeasureCount();
    //Setting up to Store CalFiles
    /**
     * @brief This will create a coefficient matrix from the reference device measurement and the KClmtr measurement. This will not store a calibration file into the Klein Device, but just return the values that would store it. This is good to used with setTempCaliFile()
     *
     * @param reference The reference device's measurement
     * @param kclmtr The KClmtr's measurement
     * @return CorrectedCoefficient The meatrix that would be stored to the KClmtr, but does not
     */
    CorrectedCoefficient getCoefficientTestMatrix(const WRGB &reference, const WRGB &kclmtr);
    /**
     * @brief Deletes a Calibration file with it's ID.
     *
     * @param calFileID
     * @return int Error code. 0 is Good
     */
    int deleteCalFile(int calFileID);

    //Storing the CalFile
    /**
     * @brief This will store a Calibration file into the device, based on the reference device's measurement and the KClmtr's measurement
     *
     * @param id The location which the Calibration file will be stored
     * @param name The name of the Calibration file
     * @param reference The Reference Device's measurement
     * @param kclmtr The KClmtr's measurement
     * @return int The error code. 0 is good.
     */
    int storeMatrices(int id, const std::string &name, const WRGB &reference, const WRGB &kclmtr);
    /**
     * @brief storeMatrices This will store a Calibration file into the device, based on the reference device's measurement and the KClmtr's measurement
     * @param id The location which the calibration file will be stored
     * @param name The name of the calibreation file will be stored
     * @param correctedXYZ The XYZ 3x3 matrix
     * @return The error code. 0 is good
     */
    int storeMatrices(int id, const std::string &name, const Matrix<double> &correctedXYZ);
    /**
     * @brief storeMatrices This will store a Calibration file into the device, based on the reference device's measurement and the KClmtr's measurement
     * @param id The location which the calibration file will be stored
     * @param name The name of the calibreation file will be stored
     * @param correctionMatrix The XYZ 3x3 matrix and The RGB 3x3 matrix, this is obsolete if using chromaSurf or this SDK. Now using GamaSpect
     * @return The error code. 0 is good
     */
    int storeMatrices(int id, const std::string &name, const CorrectedCoefficient &correctionMatrix);
    //BlackCal - Cold
    /**
     * @brief Takes reading of counts, if passes saves to flash/ram
     *
     * @return BlackMatrix
     */
    BlackMatrix captureBlackLevel();
    /**
     * @brief Reads matrix of saved counts from flash
     *
     * @return BlackMatrix
     */
    BlackMatrix getFlashMatrix();

    //BlackCal - Hot
    /**
     * @brief Reads matrix of saved counts from RAM
     *
     * @return BlackMatrix
     */
    BlackMatrix getRAMMatrix();
    /**
     * @brief Reads matrix of black thermal correction coefficents
     *
     * @return BlackMatrix
     */
    BlackMatrix getCoefficientMatrix();

    //FFT
    /**
     * @brief Returns true if the device is in flicker mode. Returns false if the device is not in flicker mode
     *
     * @return bool
     */
    bool isFlickering() const;
    /**
     * @brief Starts the device in flicker mode.
     * @return int Errorcode, 0 is good
     */
    int startFlicker();
    /**
    * @brief Returns one Flicker from the device that has been stored in the buffer of the class. You must use startFlicker() to use this method.
    * @param f the flicker struct that will be stored in to
    * @returns bool isFresh tells weither or not that the flicker is something the program already grabbed or not
    */
    bool getFlicker(Flicker &f);
    /**
     * @brief Grabs and returns one flicker measurement. Do not use startFlicker() method witht his. The speed which this returns is based on the getFFT_Samples()
     * @return Flicker
     */
    Flicker getNextFlicker();
    /**
     * @brief Stops the device from being in flicker mode.
     */
    void stopFlicker();

    //Setup/Close
    /**
     * @brief This will open the device to the getPortName() you have set.
     *
     * @return bool A user should always have this true. This is used for internal use.
     */
    bool connect();

    /**
    * @brief Connects to a Klein device
    * @param portName The name of the port to connect
    * @returns bool it connected or not
    */

    bool connect(const std::string &portName);
    /**
     * @brief Closes the port and reset all the propeties of the KClmtr object
     *
     */
    void closePort();

    /**
    * @brief Tests a port to see if it would connect to a Klein device
    * @param portName The name of the port to test
    * @param model If the port is a Klein Device, it will reaturn the Model Number of the device
    * @param SN If the port is a Klein Device, it will reaturn the Serial Number of the device
    * @returns bool it can or cannot be connected
    */
    static bool testConnection(const std::string &portName, std::string &model, std::string &SN);

    //Measurement thread
    /**
     * @brief If startMeasurment() has been called, this will be called when a full measurement has been returned from the Klein device
    	 *  @details You must inherit KClmtr class into your class and then override this function
    	 *  @details Here is an example:
    	 *  @details Header
    	 *  @snippet NativeKClmtrExample.cpp measure
    	 *   Source
    	 *  @snippet NativeKClmtrExample.cpp measure
     */
    virtual void printMeasure(Measurement) {}
    /**
     * @brief If startFlicker(true) has been called, this will be called when a full flicker has been returned from the Klein device
    	 *  @details You must inherit KClmtr class into your class and then override this function
    	 *  @details Here is an example:
    	 *  @details Header
    	 *  @snippet NativeKClmtrExample.cpp flicker
    	 *   Source
    	 *  @snippet NativeKClmtrExample.cpp flicker
     */
    virtual void printFlicker(Flicker) {}
    /**
    * @brief If startCounts(true) has been called, this will be called when a full counts has been returned from the Klein device
    *  @details You must inherit KClmtr class into your class and then override this function
    *  @details Here is an example:
    *  @details Header
    *  @snippet NativeKClmtrExample.cpp flicker
    *   Source
    *  @snippet NativeKClmtrExample.cpp flicker
    */
    virtual void printCounts(Counts) {}
protected:
    struct command {
        const std::string commandString;
        const int expected;
        const int timeout;
    };

    static command BLACKCAL_RAM;
    static command BLACKCAL_STORE;
    static command BLACKCAL_FLASH;
    static command BLACKCAL_NEW_RAM;
    static command BLACKCAL_COEFFICIENTMATRIX;
    static command CALFILE_INCOMING;
    static command CALFILE_FILELIST;
    static command CALFILE_SAVING;
    static command AIMINGLIGHTS_OFF;
    static command AIMINGLIGHTS_ON;
    static command COUNTS_4PERSECOND;
    static command COLOR_2PERSECOND;
    static command COLOR_4PERSECOND;
    static command COLOR_8PERSECOND;
    static command COLOR_16PERSECOND;
    static command DEVICE_INFO;
    static command FLICKER_INFO;
    static command FLICKER_256PERSECOND;
    static command FLICKER_384PERSECOND;
    static command RANGE_FIX1;
    static command RANGE_FIX2;
    static command RANGE_FIX3;
    static command RANGE_FIX4;
    static command RANGE_FIX5;
    static command RANGE_FIX6;
    static command RANGE_FIXCURRENT;
    static command RANGE_AUTO;
    static command DUMMY;

    /**
    * @brief sendMessageToKColorimeter To send a predefind message that you don't care about the return
    * @param m The message
    * @return errorcode
    */
    unsigned int sendMessageToKColorimeter(const command &m);
    /**
    * @brief sendMessageToKColorimeter To send a predefind message
    * @param m The message
    * @param readString The return message from the device
    * @return errorcode
    */
    unsigned int sendMessageToKColorimeter(const command &m, std::string &readString);
    /**
    * @brief sendMessageToKColorimeter To send a string to the device that you don't care about the return
    * @param strMsg The message
    * @param expected The expected number of chars coming back
    * @param timeOut_Sec the amount of time it should give up
    * @return errorcode
    */
    unsigned int sendMessageToKColorimeter(const std::string &strMsg, int expected, int timeOut_Sec);
    /**
     * @brief sendMessageToKColorimeter To send a string to the device
     * @param strMsg The message
     * @param expected The expected number of chars coming back
     * @param timeOut_Sec the amount of time it should give up
     * @param readString The return message from the device
     * @return errorcode
     */
    unsigned int sendMessageToKColorimeter(const std::string &strMsg, int expected, int timeOut_Sec, std::string &readString);
    /**
     * @brief readFromKColorimeter To just read from the device
     * @param expected The expected number of chars coming back
     * @param timeOut_Sec the amount of time it should give up
     * @param readString The return message from the device
     * @return errorcode
     */
    unsigned int readFromKColorimeter(int expected, long timeOut_Sec, std::string &readString);
private:
    //Objects
    SerialPort m_CommPort;
    bool m_isOpen;
#ifdef WIN32
    DWORD threadId;
    HANDLE threadH;
#else
    pthread_t threadId;
#endif

    //Variables
    //The Serial Number of the K10/8
    std::string m_SerialNumber;
    //The Model Number
    std::string m_Model;
    //The firmware
    std::string m_firmware;

    //Sending and receiving
    //N5 Constant measuring flag
    bool m_MeasuringN5;
    //M6 Constant measuring flag
    bool m_MeasuringM6;

    //CalFiles
    //List of the CalFiles, ID number and Name
    std::string m_CalFileList[99];
    //The name of the CalFile currently loaded or to set
    std::string m_CalFileName;
    //The ID of the CalFile currently loadded or to set
    int m_CalFileID;
    //The Cal Matrix from the K10/8
    Matrix<double> m_CalMatrix;
    //The index of the array to set load
    int m_Calindex;
    //The RGB Matrix when we download the calfile
    //matrix m_CalRGBMatrix;
    GamutSpec _gs;

    // Average for low light
    double *m_AvgX;
    double *m_AvgY;
    double *m_AvgZ;
    // Last index for average
    int m_AvgLast;
    // Max Avg
    int m_MaxAvgNumber;
    //Speed mode for color measurements
    SpeedMode m_speedMode;
    //Check noise
    bool m_ZeroNoise;

    //Freash Data for Measure and flicker
    bool m_isFlickerfresh;
    bool m_isMeasurefresh;
    bool m_isCountsfresh;
    Flicker m_flicker;
    Measurement m_measure;
    Counts m_counts;

	bool m_DeviceFlickerSpeed;

	
    //Flicker
    //Flicker measuring
    bool m_Flickering;
    //Next Flicker measuring
    bool m_Flickering2;
    //The flicker Count
    int m_fft_numPass;
    //The setting for Flicker
    FlickerSetting m_flickerSettings;
    int m_fft_lastRange;
    double *m_ParsedOutRippleArray;
    double m_FFTCalRangeCoeff1[3][71];
    double m_FFTCalRangeCoeff2[3][polyDegree];
    int m_fft_PreviousRange;
    int m_fft_count;
    int m_range;

    //XYZ - Parsing
    double speedModeSamples();
    double speedModeMultiplier();
    double multiplierForAveraging();
    int boxCarAvg(double(&data)[3], double(&minMax)[3][2], bool autoAvg, int &error);
    Measurement parseAndPrintXYZ(std::string ReadString, bool autoAvg = true);
    double unpackK_float(std::string PartString);
    double parseK_float(std::string MyString);
    const command &getColorMeasurmentCommand() const;

    //CalFiles
    //void reLoadRGB();
    void setCalFileList(const std::string &CalFileList);
    void loadedCalFile(const std::string &CalFileString);
    double unpackCalMan_float(std::string PartString);
    void correctXYZCalFile(double inX, double inY, double inZ, double &outX, double &outY, double &outZ);

    //setting up to store calfile
    void makeCorrectedXYZ(const Matrix<double> &target, const Matrix<double> &kclmtr, Matrix<double> &correctedXYZ);

    //Store the calfile
    std::string packUserMatrix(std::string name, CorrectedCoefficient corrected, unsigned int &error);
    unsigned char *convertNumberToBinaryStr(double v, unsigned int &error);

    //BlackCal
    int parsingColdHot(const std::string &read, double coldHot[19]);
    bool checkTH1andTH2(const Counts &counts);

    //FFT
    unsigned int printStartupString(const std::string &read);
    unsigned int loadFFTRangeCal(const std::string &readString);
    bool checkCoef();
    unsigned int checkThreeFFT_Validity(double array[][129]);
    bool isFlickerNew();
    void caluclateCoef(double array[][129]);
    void endFlicker();

    //FFT - Parsing
    Flicker parseAndPrintFFT(std::string &read);
    void resetFlicker();
    bool verify_FFTString(const std::string &FFTString);
    unsigned int parseSignal_from_FFT_str(const std::string &FFTString);
    unsigned int parseN5Command(const std::string &FFTString, double &outX, double &outY, double &outZ, MeasurementRange &outRange);
    int startFlicker(bool grabConstanly);


    //Sending/Receiving
    static unsigned int sendMessageToSerialPort(SerialPort &comPort, const command &m, std::string &readString);
    static unsigned int sendMessageToSerialPort(SerialPort &comPort, const std::string &strMsg, int expected, int timeOut_Sec, std::string &readString);
    static unsigned int readFromSerialPort(SerialPort &comPort, int expected, long timeOut_Sec, std::string &readString);

    //Setup/Close
    static bool setSerialNumberValues(const std::string &read, std::string &model, std::string &SN);
    static bool getModelSN(SerialPort &comPort, std::string &model, std::string &SN);

    //Measurement thread
    enum _ThreadMode {
        NOT_RUNNING,
        RUN,
        STOP
    };
    enum _measureMode {
        MEASURE,
        FLICKER,
        COUNTS
    };
    void startThread2(_measureMode m);
    void stopThread2();
    void endThread();
    static void threadStuff(void *args);
    _ThreadMode threadModeParent;
    _ThreadMode threadModeChild;
    _measureMode measureMode;
    void stopFlicker2();
    void stopMeasuring2();
    void stopMeasureCounts2();
    void startFlicker2();
    void startMeasuring2();
    void startMeasureCounts2();
};
}
}
