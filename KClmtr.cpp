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
#include "KClmtr.h"

#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include <cmath>
#include <cstring>

using namespace std;

using namespace KClmtrBase;
using namespace KClmtrBase::KClmtrNative;

KClmtr::command KClmtr::BLACKCAL_RAM				= {"B4",   43, 2}; //Gets Ram Black Cal
KClmtr::command KClmtr::BLACKCAL_STORE				= {"B7",    2, 1}; //Store the RAM into FLASH, need password
KClmtr::command KClmtr::BLACKCAL_FLASH				= {"B8",   43, 2}; //Gets Flash Black Cal
KClmtr::command KClmtr::BLACKCAL_NEW_RAM			= {"B9",   43, 8}; //Set a new state for RAM, Start up, RAM = FLASH
KClmtr::command KClmtr::BLACKCAL_COEFFICIENTMATRIX  = {"S0",  133, 2}; //Counts Black Cal Matrix
KClmtr::command KClmtr::CALFILE_INCOMING			= {"D1",    2, 2}; //Give heads up for Cal retrival
KClmtr::command KClmtr::CALFILE_FILELIST			= {"D7", 1925, 6}; //Cal Files List
KClmtr::command KClmtr::CALFILE_SAVING				= {"D9",    2, 2}; //Givs heads up for storing cal files
KClmtr::command KClmtr::AIMINGLIGHTS_OFF			= {"L0",    0, 0}; //Lights Off
KClmtr::command KClmtr::AIMINGLIGHTS_ON				= {"L1",    0, 0}; //Lights On
KClmtr::command KClmtr::COUNTS_4PERSECOND			= {"M6",   20, 2}; //Count Measurement
KClmtr::command KClmtr::COLOR_2PERSECOND            = {"N7",   15, 1}; //Slowest Color Measurement
KClmtr::command KClmtr::COLOR_4PERSECOND            = {"N6",   15, 1}; //Slow Color Measurement
KClmtr::command KClmtr::COLOR_8PERSECOND			= {"N5",   15, 1}; //Color Measurement
KClmtr::command KClmtr::COLOR_16PERSECOND			= {"N4",   15, 1}; //Fast Color Measurement
KClmtr::command KClmtr::DEVICE_INFO					= {"P0",   21, 1}; //Serial/Model
KClmtr::command KClmtr::FLICKER_INFO				= {"P4",  613, 5}; //Flicker Cal, Firmware
KClmtr::command KClmtr::FLICKER_256PERSECOND		= {"T2",   -1, 2}; //Flicker
KClmtr::command KClmtr::FLICKER_384PERSECOND		= {"T1",   -1, 2}; //Faster Flicker
KClmtr::command KClmtr::RANGE_FIX1					= {"J1",    1, 2}; //Set Range to 1
KClmtr::command KClmtr::RANGE_FIX2					= {"J2",    1, 2}; //Set Range to 2
KClmtr::command KClmtr::RANGE_FIX3					= {"J3",    1, 2}; //Set Range to 3
KClmtr::command KClmtr::RANGE_FIX4					= {"J4",    1, 2}; //Set Range to 4
KClmtr::command KClmtr::RANGE_FIX5					= {"J5",    1, 2}; //Set Range to 5
KClmtr::command KClmtr::RANGE_FIX6					= {"J6",    1, 2}; //Set Range to 6
KClmtr::command KClmtr::RANGE_FIXCURRENT			= {"J7",    1, 2}; //Stops AutoRanging
KClmtr::command KClmtr::RANGE_AUTO					= {"J8",    1, 2}; //Start AutoRanging
KClmtr::command KClmtr::DUMMY						= {"X9",   -1, 1 }; //To stop flicker, and other dummy things

static void sleep(int ms) {
#ifdef WIN32
    Sleep((DWORD)ms);
#else
    usleep(ms * 1000);
#endif
}

static string trim(const string &s) {
    size_t begin = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r") + 1;

    return s.substr(begin, end - begin);
}

//Polyfit - came from http://www.ngdc.noaa.gov/geomag/geom_util/polyfit.shtml
//data X/Y data going downwards: column 0 = X, 1 = Y
static void polyFit(const Matrix<double> &data, unsigned int degree, double consts[]) {
    Matrix<double> X(data.getRow(), degree);
    Matrix<double> Y(data.getRow(), 1);
    for(unsigned int i = 0; i < data.getRow(); ++i) {
        Y.v[i][0] = data.v[i][1];
        for(unsigned int j = 0; j < degree; ++j) {
            X.v[i][j] = pow(data.v[i][0], j);
        }
    }
    Matrix<double> TX = X.transpose();
    Matrix<double> XSquare = TX * X;

    //Setting up P to be a unity matrix
    Matrix<double> P = Matrix<double>::Unity(degree);

    //Getting L,U Matrixes, and updating P
    Matrix<double> L, U;
    if(XSquare.LUDecomposition(L, U, P)) {
        //Got the L,U,P Matrixes now
        Matrix<double> b = P * (TX * Y);
        Matrix<double> Final = Matrix<double>::LUSolve(L, U, b);

        for(unsigned int i = 0; i < degree; i++) {
            consts[i] = Final.v[i][0];
        }
    }
}

KClmtr::KClmtr() {
    //Objects
    m_isOpen = false;
    threadId = 0;
    //Variables
    //The Serial Number of the K10/8
    m_SerialNumber = "";
    //The Model Number
    m_Model = "";
    //The Firmware
    m_firmware = "";
    //Check noise
    m_ZeroNoise = false;

    //Sending and receiving
    //N5 Constant measuring flag
    m_MeasuringN5 = false;
    m_speedMode = SpeedMode::SPEEDMODE_NORMAL;
    //M6 Constant measuring flag
    m_MeasuringM6 = false;

	m_DeviceFlickerSpeed = true;

    //CalFiles
    //List of the CalFiles, ID number and Name
    for(int i = 0; i < 99; ++i) {
        m_CalFileList[i] = "";
    }
    //The name of the CalFile currently loaded or to set
    m_CalFileName = "Factory Cal File";
    //The ID of the CalFile currently loadded or to set
    m_CalFileID = 0;
    //The index of the array to set load
    m_Calindex = 0;
    m_CalMatrix.initializeV(3, 3);
    _gs = GamutSpec::fromCode(GamutCode::defaultGamut);

    // Low light average index
    m_AvgLast = 0;


    //Flicker
    //Flicker measuring
    m_Flickering = false;
    m_Flickering2 = false;

    m_flickerSettings = FlickerSetting();

    for(int i = 0; i <= 70; ++i) {
        m_FFTCalRangeCoeff1[0][i] = -1;
        m_FFTCalRangeCoeff1[1][i] = -1;
        m_FFTCalRangeCoeff1[2][i] = -1;
    }

    for(int i = 0; i < polyDegree; ++i) {
        m_FFTCalRangeCoeff2[0][i] = -1;
        m_FFTCalRangeCoeff2[1][i] = -1;
        m_FFTCalRangeCoeff2[2][i] = -1;
    }

    m_ParsedOutRippleArray = NULL;

    m_fft_numPass = 0;
    m_fft_PreviousRange = 0;

    threadModeParent = NOT_RUNNING;
    threadModeChild = NOT_RUNNING;
#ifdef WIN32
    threadH = 0;
#endif

    m_AvgX = NULL;
    m_AvgY = NULL;
    m_AvgZ = NULL;
    m_MaxAvgNumber = 32;
}

KClmtr::~KClmtr() {
    closePort();
}
void KClmtr::setPort(const string &portName) {
    m_CommPort.portName = trim(portName);
}
string KClmtr::getPort() const {
    return m_CommPort.portName;
}

bool KClmtr::isPortOpen() {
    bool isOpen = m_CommPort.isOpen();
    if(m_isOpen && !isOpen) {
        closePort();
    }
    return isOpen;
}
string KClmtr::getSerialNumber() const {
    return m_SerialNumber;
}
string KClmtr::getModel() const {
    return m_Model;
}
void KClmtr::setAimingLights(bool onOff) {
    if(m_Model.find("K-10") == 0 ||
            m_Model.find("KV-10") == 0 ||
            m_Model.find("K-80") == 0) {
        sendMessageToKColorimeter(onOff ? AIMINGLIGHTS_ON : AIMINGLIGHTS_OFF);
    }
}
int KClmtr::getRange() const {
    return m_range;
}

void KClmtr::setRange(int range) {
    m_range = range;
    switch(m_range) {
        case 0:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            break;
        case 1:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            sendMessageToKColorimeter(RANGE_FIX1);
            break;
        case 2:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            sendMessageToKColorimeter(RANGE_FIX2);
            break;
        case 3:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            sendMessageToKColorimeter(RANGE_FIX3);
            break;
        case 4:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            sendMessageToKColorimeter(RANGE_FIX4);
            break;
        case 5:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            sendMessageToKColorimeter(RANGE_FIX5);
            break;
        case 6:
            sendMessageToKColorimeter(RANGE_FIXCURRENT);
            sendMessageToKColorimeter(RANGE_FIX6);
            break;
        case -1:
        default:
            sendMessageToKColorimeter(RANGE_AUTO);
            break;
    }
}
void KClmtr::setZeroNoise(const bool zeroNoise) {
    m_ZeroNoise = zeroNoise;
}

bool KClmtr::getZeroNoise() {
    return m_ZeroNoise;
}
string KClmtr::getCalFileName() const {
    //Returns The CalFileName when we set it after a load
    return m_CalFileName;
}
void KClmtr::setCalFileID(int calFileID) {
    //Find a ID matching the user ID
    if(calFileID < 96) {
        //Save the counter to index
        m_Calindex = calFileID;
    }

    //Making sure it is not the Factory Cal File
    if(m_CalFileList[m_Calindex] == "Blank" || m_Calindex == 0 || m_CalFileList[m_Calindex] == "Factory Cal File") {
        loadedCalFile("");
    } else {
        string returnString;
        //Goes to load cal file
        if(sendMessageToKColorimeter(CALFILE_INCOMING, returnString) == 0) {
            if(returnString == "D1") {
                string CalFile;
                //After find which ID we are using
                CalFile = m_Calindex;
                if(sendMessageToKColorimeter(CalFile, 131, 2, CalFile) == 0) {
                    loadedCalFile(CalFile);
                }
            }
        }
    }
}
int KClmtr::getCalFileID() const {
    return m_CalFileID;
}
Matrix<double> KClmtr::getCalMatrix() const {
    return m_CalMatrix;
}
Matrix<double> KClmtr::getRGBMatrix() const {
    return _gs.getXYZtoRGB();
}
GamutSpec KClmtr::getGamutSpec() const {
    return _gs;
}
void KClmtr::setGamutSpec(const GamutSpec &gs) {
    _gs = gs;
}

vector<string> KClmtr::getCalFileList() const {
    vector<string> CalList;
    for(int i = 0; i < 97; ++i) {
        CalList.push_back(intToString(i) + ": " + m_CalFileList[i]);
    }
    //Returns the CalFileList with ID : Name;
    return CalList;
}

void KClmtr::setFFT_Cosine(bool use) {
    m_flickerSettings.cosine = use;
}
bool KClmtr::getFFT_Cosine() const {
    return m_flickerSettings.cosine;
}
void KClmtr::setFFT_Smoothing(bool use) {
    m_flickerSettings.smoothing = use;
}
bool KClmtr::getFFT_Smoothing() const {
    return m_flickerSettings.smoothing;
}

int KClmtr::setFFT_Samples(int samples) {
    if(64 <= samples && samples <= 2048 && ((samples & ~(samples - 1)) == samples)) {   //Power of 2, and between 64 and 2048
        if(samples != m_flickerSettings.samples) {
            m_flickerSettings.samples = samples;
            if(m_Flickering) {
                this->resetFlicker();
                return this->startFlicker(!m_Flickering2);
            }
        }
        return (int)KleinsErrorCodes::NONE;
    } else {
        return (int)KleinsErrorCodes::FFT_BAD_SAMPLES;
    }
}
int KClmtr::getFFT_Samples() const {
    return m_flickerSettings.samples;
}
bool KClmtr::getFFT_PercentJEITA_Discount() const {
    return m_flickerSettings.JEITADiscount_Percent;
}
void KClmtr::setFFT_PercentJEITA_Discount(bool onOff) {
    m_flickerSettings.JEITADiscount_Percent = onOff;
}

bool KClmtr::getFFT_DBJEITA_Discount() const {
    return m_flickerSettings.JETIADiscount_DB;
}
void KClmtr::setFFT_DBJEITA_Discount(bool onOff) {
    m_flickerSettings.JETIADiscount_DB = onOff;
}

PercentMode KClmtr::getFFT_PercentMode() const {
    return m_flickerSettings.percent;
}
void KClmtr::setFFT_PercentMode(PercentMode mode) {
    m_flickerSettings.percent = mode;
}

DecibelMode KClmtr::getFFT_DBMode() const {
    return m_flickerSettings.decibel;
}
void KClmtr::setFFT_DBMode(DecibelMode mode) {
    m_flickerSettings.decibel = mode;
}
void KClmtr::setFFT_numberOfPeaks(int numberOfPeaks) {
    m_flickerSettings.numberOfPeaks = numberOfPeaks;
}
int KClmtr::getFFT_numberOfPeaks() const {
    return m_flickerSettings.numberOfPeaks;
}
void KClmtr::threadStuff(void *args) {
    KClmtr *k = (KClmtr *)args;
#ifdef WIN32
    k->threadId = GetCurrentThreadId();
#endif


    k->threadModeChild = RUN;

    if(k->measureMode == FLICKER) {
        if(k->isFlickerNew()) {
            k->m_flickerSettings.speed = 384;
            k->sendMessageToKColorimeter(FLICKER_384PERSECOND);
            sleep(5);
            k->m_CommPort.setSetting(9600 * 2, 8, 'n', 10000);
        } else {
            k->m_flickerSettings.speed = 256;
            k->sendMessageToKColorimeter(FLICKER_256PERSECOND);
        }
    }
    string FFTString;
    while(k->threadModeParent == RUN) {
        if(k->measureMode == MEASURE) {
            string measure;
            int error = k->sendMessageToKColorimeter(k->getColorMeasurmentCommand(), measure);
            if(error == 0 && k->threadModeParent == RUN) {
                k->m_measure = k->parseAndPrintXYZ(measure);

                k->m_isMeasurefresh = true;
                k->printMeasure(k->m_measure);
            } else if(error) {
                k->m_measure = Measurement::fromError(error);
                k->threadModeParent = STOP;

                k->m_isMeasurefresh = true;
                k->printMeasure(k->m_measure);
            }
        } else if(k->measureMode == FLICKER) {
            int error = 0;
            if(error == 0 && FFTString.length() > 96) {
                //Runs through FFT
                if(!k->verify_FFTString(FFTString)) {
                    //Make it pass the next time
                    size_t nextT = FFTString.find('T', 1);
                    if(nextT == FFTString.npos) {
                        FFTString = "";
                    } else {
                        FFTString = FFTString.substr(nextT);
                    }
                } else {
                    k->m_flicker = k->parseAndPrintFFT(FFTString);
                    if(k->threadModeParent == RUN) {
                        if(k->m_flicker.errorcode & ~((int)KleinsErrorCodes::FFT_PREVIOUS_RANGE | (int)KleinsErrorCodes::FFT_INSUFFICIENT_DATA | (int)KleinsErrorCodes::FFT_OVER_SATURATED)) {
                            k->threadModeParent = STOP;
                        }
                        k->m_isFlickerfresh = true;
                        k->printFlicker(k->m_flicker);
                    }
                }
            } else {
                //Reads more data to get above 96
                error = k->readFromKColorimeter(96, 2, FFTString);
                if(error != 0) {
                    k->m_flicker = Flicker();
                    k->m_flicker.errorcode = error;

                    k->m_isFlickerfresh = true;
                    k->threadModeParent = STOP;
                    k->printFlicker(k->m_flicker);
                }
            }
        } else if(k->measureMode == COUNTS) {
            string counts;
            int error = k->sendMessageToKColorimeter(COUNTS_4PERSECOND, counts);
            if(error == 0 && k->threadModeParent == RUN) {
                k->m_counts = Counts(counts);

                k->m_isCountsfresh = true;
                k->printCounts(k->m_counts);
            } else if(error) {
                k->m_counts = Counts();
                k->m_counts.errorcode = error;

                k->threadModeParent = STOP;

                k->m_isMeasurefresh = true;
                k->printCounts(k->m_counts);
            }
        }
    }
    k->endThread();
}
void KClmtr::endThread() {
    if(measureMode == FLICKER) {
        endFlicker();
    }
    threadModeChild = NOT_RUNNING;
    threadId = 0;
}
void KClmtr::stopThread2() {
    if(threadModeChild == NOT_RUNNING) {
        return;
    }
    threadModeParent = STOP;
    //Making sure the thread has ended
#ifdef WIN32
    DWORD currentThreadID = GetCurrentThreadId();
    if(threadId != currentThreadID) {
#else
    pthread_t currentThreadID = pthread_self();
    if(threadId != currentThreadID) {
#endif
        while(threadModeChild != NOT_RUNNING) {
            sleep(1);
        }
#ifdef WIN32
        WaitForSingleObject(threadH, INFINITE);
#else
        pthread_join(threadId, NULL);
#endif
    }
     threadModeParent = NOT_RUNNING;
}
void KClmtr::startThread2(_measureMode m) {
    stopThread2();
    threadId = 0;
    measureMode = m;
    threadModeParent = RUN;
    threadModeChild = NOT_RUNNING;
#ifdef WIN32
    threadH = (HANDLE)_beginthread(KClmtr::threadStuff, 0, this);
    if(threadH == 0) {
#else
    int stacksize = 0;
    pthread_attr_t  attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stacksize);
    pthread_create(&threadId, &attr, (void * ( *)(void *))KClmtr::threadStuff, (void *)this);
    if(threadId == 0) {
#endif
        if(isMeasuring()) {
            stopMeasuring();
        }
        if(isFlickering()) {
            stopFlicker();
        }
        if(isMeasureCounts()) {
            stopMeasureCounts();
        }
    }
    while(threadModeChild == NOT_RUNNING) {
        sleep(1);
    }
}
void KClmtr::stopFlicker2() {
    stopThread2();
}
void KClmtr::stopMeasuring2() {
    stopThread2();
}
void KClmtr::stopMeasureCounts2() {
    stopThread2();
}
void KClmtr::startFlicker2() {
    startThread2(FLICKER);
}
void KClmtr::startMeasuring2() {
    startThread2(MEASURE);
}
void KClmtr::startMeasureCounts2() {
    startThread2(COUNTS);
}
//XYZ
bool KClmtr::setMaxAverageCount(int maxAvg) {
    if(m_MaxAvgNumber != maxAvg &&
            maxAvg <= 128 &&
            maxAvg >= 1) {
        stopMeasuring();
        m_MaxAvgNumber = maxAvg;
        return true;
    }
    return false;
}

int KClmtr::getMaxAverageCount() const {
    return m_MaxAvgNumber;
}
SpeedMode KClmtr::getMeasureSpeedMode() const {
    return m_speedMode;
}
void KClmtr::setMeasureSpeedMode(SpeedMode value) {
    if(isMeasuring()) {
        stopMeasuring();
    }
    m_speedMode = value;
}
const KClmtr::command &KClmtr::getColorMeasurmentCommand() const {
    switch(m_speedMode) {
        case SpeedMode::SPEEDMODE_SLOWEST:
            return KClmtr::COLOR_2PERSECOND;
        case SpeedMode::SPEEDMODE_SLOW:
            return KClmtr::COLOR_4PERSECOND;
        case SpeedMode::SPEEDMODE_FAST:
            return KClmtr::COLOR_16PERSECOND;
        case SpeedMode::SPEEDMODE_NORMAL:
        default:
            return KClmtr::COLOR_8PERSECOND;
    }
}
void KClmtr::startMeasuring() {
    stopFlicker();
    stopMeasureCounts();
    m_MeasuringN5 = true;
    m_AvgLast = 0;
    m_AvgX = new double[m_MaxAvgNumber];
    m_AvgY = new double[m_MaxAvgNumber];
    m_AvgZ = new double[m_MaxAvgNumber];
    startMeasuring2();
}
void KClmtr::stopMeasuring() {
    stopMeasuring2();
    m_AvgLast = 0;
    m_MeasuringN5 = false;
    if(m_AvgX != NULL) {
        delete[] m_AvgX;
        delete[] m_AvgY;
        delete[] m_AvgZ;
        m_AvgX = NULL;
        m_AvgY = NULL;
        m_AvgZ = NULL;
    }
}
bool KClmtr::isMeasuring() const {
    return m_MeasuringN5;
}
bool KClmtr::getMeasurement(Measurement &m) {
    m = m_measure;
    bool temp = m_isMeasurefresh;
    m_isMeasurefresh = false;
    return temp;
}
void KClmtr::correctXYZCalFile(double inX, double inY, double inZ, double &outX, double &outY, double &outZ) {
    //No need to change xyY with factory cal
    if(m_CalFileID != 0) {
        outX = inX * m_CalMatrix.v[0][0] +
               inY * m_CalMatrix.v[0][1] +
               inZ * m_CalMatrix.v[0][2];
        outY = inX * m_CalMatrix.v[1][0] +
               inY * m_CalMatrix.v[1][1] +
               inZ * m_CalMatrix.v[1][2];
        outZ = inX * m_CalMatrix.v[2][0] +
               inY * m_CalMatrix.v[2][1] +
               inZ * m_CalMatrix.v[2][2];
    } else {
        outX = inX;
        outY = inY;
        outZ = inZ;
    }
}

Measurement KClmtr::getNextMeasurement(int n) {
    //Clearing the buffer
    unsigned int error = sendMessageToKColorimeter(getColorMeasurmentCommand());
    if(error != KleinsErrorCodes::NONE) {
        return Measurement::fromError(error);
    }
    Measurement m;
    int tempMaxAvg = m_MaxAvgNumber;    //Save this value, to reset the max avg, so that boxAvg is correct
    m_MaxAvgNumber = n < 1 ? m_MaxAvgNumber : n;
    m_AvgLast = 0;
    m_AvgX = new double[m_MaxAvgNumber];
    m_AvgY = new double[m_MaxAvgNumber];
    m_AvgZ = new double[m_MaxAvgNumber];

    int i = 0;
    do {
        string mstring;
        error = sendMessageToKColorimeter(getColorMeasurmentCommand(), mstring);
        if(error != KleinsErrorCodes::NONE) {
            return Measurement::fromError(error);
        }
        m = parseAndPrintXYZ(mstring, n < 1);

        ++i;
    } while(
        !(!(m.errorcode & (int)KleinsErrorCodes::AVERAGING_LOW_LIGHT) && n < 1) && //Hit auto range light level
        !(i >= m_MaxAvgNumber));							    //Hit the maxAvg

    m_AvgLast = 0;
    m_MaxAvgNumber = tempMaxAvg;
    delete[] m_AvgX;
    delete[] m_AvgY;
    delete[] m_AvgZ;
    m_AvgX = NULL;
    m_AvgY = NULL;
    m_AvgZ = NULL;

    return m;
}
//XYZ - Parsing
double KClmtr::speedModeSamples() {
    return 1 / speedModeMultiplier() * 32.0; //32 samples per second
}
double KClmtr::speedModeMultiplier() {
    switch(m_speedMode) {
        case SpeedMode::SPEEDMODE_SLOWEST:
            return .25; // Need 1/4 times the infomation for  2 mps(Measurements per second)
        case SpeedMode::SPEEDMODE_SLOW:
            return .5;  // Need 1/2 times the infomation for  4 mps
        case SpeedMode::SPEEDMODE_FAST:
            return 2;  // Need 2    times the infomation for 16 mps
        case SpeedMode::SPEEDMODE_NORMAL:
        default:
            return 1;  // Needs 1   times the infomation for 8 mps
    }
}
double KClmtr::modelSensitivityMultiplier() {
    return 1.0 / modelSensitivity();
}
double KClmtr::modelSensitivity() {
    if(m_Model == "K-1") {
        return 30.0;
    } else if(m_Model == "K-10-A"
              || m_Model == "K-10A") {
        return 3.0;
    } else if(m_Model == "K-8") {
        return 1.0 / 8.0;
    } else if(m_Model == "K-80") {
        return 2.0;
    } else {          //K-10 or others
        return 1.0;
    }
}
double KClmtr::multiplierForAveraging() {
    return modelSensitivityMultiplier() * speedModeMultiplier();
}
int KClmtr::boxCarAvg(double(&data)[3], double(&minMax)[3][2], bool autoAvg, int &error) {
    int AvgLastLocal = m_AvgLast % m_MaxAvgNumber;
    //Setting the values in the Avg
    m_AvgX[AvgLastLocal] = data[0];
    m_AvgY[AvgLastLocal] = data[1];
    m_AvgZ[AvgLastLocal] = data[2];

    //resetting Data, Min and Max Values
    for(int i = 0; i < 3; ++i) {
        minMax[i][0] = 10000;
        minMax[i][1] = -10000;
        data[i] = 0;
    }

    //Avging
    bool thresholdMet = false;
    int avgNumber;
    int avgLastI;
    for(avgNumber = 1,                  //how much we are avging by
            avgLastI = m_AvgLast;           //is the unit to avg with, but we mod it with maxAvgNumber
            avgNumber <= m_MaxAvgNumber &&   //Checking to make sure we don't go out of MaxAvgNumber
            0 <= avgLastI;                  //Checking to make sure we don't go back in time we don't have data for
            ++avgNumber,                   //Adding one to Avging by
            --avgLastI) {                   //Subtracting to go back in time to grab next freshest data

        int AvgLastLocal = avgLastI % m_MaxAvgNumber;
        double tempData[3];
        tempData[0] = m_AvgX[AvgLastLocal];
        tempData[1] = m_AvgY[AvgLastLocal];
        tempData[2] = m_AvgZ[AvgLastLocal];

        //Checking the running avg vs the current number
        //to see if it's too far out of spec
        if(avgNumber != 1) {	//Can't really do an avg on the first mesurement
            double sum = tempData[0] + tempData[1] + tempData[2];
            double runningSum = (data[0] + data[1] + data[2]) / (avgNumber - 1);
            if((ABS(runningSum - sum) / (runningSum + 1)) > (.01 * 3)) {
                error |= (int)KleinsErrorCodes::AVERAGING_LOW_LIGHT;
                break;
            }
        }

        data[0] += tempData[0];
        data[1] += tempData[1];
        data[2] += tempData[2];

        //Getting Min and Max Values
        for(int i = 0; i < 3; ++i) {
            //Checking Min
            if(tempData[i] < minMax[i][0]) {
                minMax[i][0] = tempData[i];
            }
            //Checking Max
            if(tempData[i] > minMax[i][1]) {
                minMax[i][1] = tempData[i];
            }
        }
        //Checking if the threshold is met on Auto
        if(autoAvg) {
            double sum = data[0] + data[1] + data[2];
            double threshold = avgNumber * 3.0 * 16 / (avgNumber + 1.0);
                   threshold *= multiplierForAveraging();
            if(sum > threshold) {
                thresholdMet = true;
                ++avgNumber;	//Stupid hax, for it skips the +1
                break;
            }
        }

    }
    --avgNumber; //Went past, need to go back one.
    //Setting the Data
    for(int i = 0; i < 3; ++i) {
        data[i] /= avgNumber;
    }
    //Checking if Theshold has been Met, or if we hit Max Avg
    if(!(thresholdMet || avgNumber == m_MaxAvgNumber)) {
        error |= (int)KleinsErrorCodes::AVERAGING_LOW_LIGHT;
    }

    /*//Setting unstable light
    //Checking Min and max - data > 1% nits
    double sum = data[0] + data[1] + data[2];
    double sumMin = minMax[0][0] + minMax[1][0] + minMax[2][0];
    double sumMax = minMax[0][1] + minMax[1][1] + minMax[2][1];
    if (((sumMax - sumMin) / (sum + 1)) > (.01 * 3)) {
    	m_errorcode |= AVERAGING_LOW_LIGHT;
    } */

    return avgNumber;
}

Measurement KClmtr::parseAndPrintXYZ(string ReadString, bool autoAvg) {
    string SubString;
    string MyError;
    int error = 0;
    double data[3];
    //ReadString = "" + (char)78 + (char)53 + (char)93 + (char)90 + (char)7 + (char)102 + (char)173 + (char)7 + (char)77 + (char)42 + (char)7 + (char)128 + (char)60 + (char)48 + (char)62;

    //Getting X
    SubString = ReadString.substr(2, 3);
    data[0] = parseK_float(SubString);

    //Getting Big Y OR L
    SubString = ReadString.substr(5, 3);
    data[1] = parseK_float(SubString);

    //Getting Z
    SubString = ReadString.substr(8, 3);
    data[2] = parseK_float(SubString);

    //Getting(range)
    SubString = ReadString.substr(11, 1);
    int ranges[3];
    parsingRange(SubString.c_str()[0], ranges);

    //Getting error
    MyError = ReadString.substr(13, 1);
    error = KleinsErrorCodes::getErrorCodeFromKlein(MyError);

    //Run the Avg
    double minMax[3][2];

    int avgNumber = boxCarAvg(data, minMax, autoAvg, error);
    ++m_AvgLast;

    //Correcting with Calfile
    double bigx, bigy, bigz;

    //Checking Noise
    for(int i = 0; i < 3; ++i) {
        //Number of Samples
        double samples = speedModeSamples() * avgNumber;
        //Max out the sample noise
        if(samples > 512){
            samples = 512;
        }
        //Models sensitivity and the samples
        double muiti = modelSensitivityMultiplier() * 1.0 / sqrt(samples/32.0);
        //Red and Blue are less sentive than Green
        if(i != 1) {
            muiti *= 3.0;
        }
        //Threshold for a Neg value could be
        double theshold = -.02 * muiti;
        //Checking if the data is less than 0, but geater than threshold
        if(m_ZeroNoise && data[i] < 0 && data[i] > theshold) {
            data[i] = 0;
        } else if(data[i] <= theshold) { //Lower than the Threshold
            error |= KleinsErrorCodes::NEGATIVE_VALUES;
        }
    }

    //Correcting with Calfile
    correctXYZCalFile(data[0], data[1], data[2],  //in XYZ
                      bigx,    bigy,    bigz);    //out XYZ

    /*******************/
    /*Setting veraibles*/
    /*******************/
    //Calucating from XYZ
    Measurement measurement = Measurement::fromXYZ(bigx, bigy, bigz, _gs, error);

    //Setting ranges
    measurement.redrange = (MeasurementRange)ranges[0];
    measurement.greenrange = (MeasurementRange)ranges[1];
    measurement.bluerange = (MeasurementRange)ranges[2];

    //Setting Raw values
    measurement.bigxraw = data[0];
    measurement.bigyraw = data[1];
    measurement.bigzraw = data[2];

    //Set Avgby
    measurement.averagingby = avgNumber;

    //Setting Min and  Max
    measurement.minX = minMax[0][0];
    measurement.maxX = minMax[0][1];
    measurement.minY = minMax[1][0];
    measurement.maxY = minMax[1][1];
    measurement.minZ = minMax[2][0];
    measurement.maxZ = minMax[2][1];

    return measurement;
}
double KClmtr::unpackK_float(string PartString) {
    int chr1, chr2, chr3, NegBit;
    double K_float;
    const unsigned char *myRead = (const unsigned char *)PartString.c_str();
    //sign and fractions MSB
    chr1 = (int)myRead[0];
    //fractions LSB
    chr2 = (int)myRead[1];
    //the signed exponent
    chr3 = (int)myRead[2];

    if(chr1 >= 128) {
        chr1 = chr1 - 128;
        NegBit = -1;
    } else {
        NegBit = 1;
    }

    //chr3 is stored as 2's complement
    if(chr3 > 128) {
        chr3 = chr3 - 256;
    }
    //128*256=32768
    K_float = chr1 * 256;
    K_float = K_float + chr2;
    K_float = NegBit * K_float;
    K_float = K_float / 32768;
    //exp(..) = 2^(float3)
    K_float = K_float * exp(chr3 * log(2.0));
    return K_float;
}
double KClmtr::parseK_float(string MyString) {
    //this is an XYZ response K_float, may be 2x different from matrix K_float
    int MyInt1, MyInt2, MyInt3, NegFlag;
    double MyFraction, K_float;
    const unsigned char *myRead = (const unsigned char *)MyString.c_str();
    //MyString is 3 chrs.. sign bit, 15 bit fraction, and 1 byte 2's exponent in 2's complement format
    MyInt1 = (int)myRead[0];
    MyInt2 = (int)myRead[1];
    MyInt3 = (int)myRead[2];
    //first bit is negative flag
    if(MyInt1 > 127) {
        MyInt1 = MyInt1 - 128;
        NegFlag = -1;
    } else {
        NegFlag = 1;
    }
    MyFraction = MyInt1 * 256 + MyInt2;
    MyFraction = NegFlag * MyFraction / 256;
    //The K_Float was off by a factor of 2, therefore it is 65536 and not 32768
    MyFraction = MyFraction / 256;

    //2's complement exponent
    if(MyInt3 > 128) {
        MyInt3 = MyInt3 - 256;
    }

    K_float = MyFraction * pow(2.0, MyInt3);

    return K_float;
}

//CalFiles
//void KClmtr::reLoadRGB() {
//    double RGBPercent[3];
//    RGBPercent[0] = m_rawCalRGBMatrix[0] * m_WhiteSpec.x + m_rawCalRGBMatrix[1] * m_WhiteSpec.y + m_rawCalRGBMatrix[2] * m_WhiteSpec.z;
//    RGBPercent[1] = m_rawCalRGBMatrix[3] * m_WhiteSpec.x + m_rawCalRGBMatrix[4] * m_WhiteSpec.y + m_rawCalRGBMatrix[5] * m_WhiteSpec.z;
//    RGBPercent[2] = m_rawCalRGBMatrix[6] * m_WhiteSpec.x + m_rawCalRGBMatrix[7] * m_WhiteSpec.y + m_rawCalRGBMatrix[8] * m_WhiteSpec.z;

//    int c = 0;
//    for(int i = 0; i < 3; ++i) {
//        for(int j = 0; j < 3; ++j) {
//            //Saving the RGB Matrix
//            m_CalRGBMatrix.v[i][j] = m_rawCalRGBMatrix[c] * 100 / RGBPercent[i];
//            c = c + 1;
//        }
//    }
//}
void KClmtr::setCalFileList(const string &CalFileList) {
    string theSubString;
    int Bitval;
    Bitval = 3;
    //And it is the Factory Cal File
    m_CalFileList[0] = "Factory Cal File";
    for(int i = 1; i < 97; ++i) {
        theSubString = CalFileList.substr(Bitval - 1, 20);
        const unsigned char *myRead = reinterpret_cast<const unsigned char *>(theSubString.c_str());
        const unsigned char testBlank = myRead[10];
        //If we find no CalFile we call it a Blank File
        if(testBlank == 255) {
            theSubString = ("Blank");
        }
        //Removing all the Spacing after the name
        theSubString = trim(theSubString);
        //Setting the name to the ID
        m_CalFileList[i] = theSubString;
        //Going to the next CalFile in the List
        Bitval = Bitval + 20;
    }
}

void KClmtr::loadedCalFile(const string &CalFileString) {
    //Getting the Matix out of the ReadString
    string PartString;
    double CalMatrix[9];
    //0 = red, 1 = green, 2 = blue
    //double RGBPercent[3];

    //Making sure it is not the Factory Cal File
    if(m_CalFileList[m_Calindex] == "Blank" || m_Calindex == 0 || m_CalFileList[m_Calindex] == "Factory Cal File") {
        //Saving the CalFileID
        m_CalFileID = 0;
        //And the Name of it after it has been loaded
        m_CalFileName = "Factory Cal File";
        //Making the identity matrix
        CalMatrix[0] = 1;
        CalMatrix[1] = 0;
        CalMatrix[2] = 0;
        CalMatrix[3] = 0;
        CalMatrix[4] = 1;
        CalMatrix[5] = 0;
        CalMatrix[6] = 0;
        CalMatrix[7] = 0;
        CalMatrix[8] = 1;

        //        m_rawCalRGBMatrix[0] = 3.85266;
        //        m_rawCalRGBMatrix[1] = -1.7702026;
        //        m_rawCalRGBMatrix[2] = -0.59213258;
        //        m_rawCalRGBMatrix[3] = -1.09594727;
        //        m_rawCalRGBMatrix[4] = 2.027709961;
        //        m_rawCalRGBMatrix[5] = 0.020358086;
        //        m_rawCalRGBMatrix[6] = 0.05136680603;
        //        m_rawCalRGBMatrix[7] = -0.190895080566;
        //        m_rawCalRGBMatrix[8] = 0.9453125;

        //        m_defaultWhiteSpec.x = 76.01171875;
        //        m_defaultWhiteSpec.y = 80;
        //        m_defaultWhiteSpec.z = 87.07421875;
        //        m_defaultWhiteSpec.xyVar = 0.005;
        //        m_defaultWhiteSpec.yVar = 5;

        //        if(!m_UseCustomWhiteSpec) {
        //            m_WhiteSpec = m_defaultWhiteSpec;
        //        }
    } else {
        //Saving the CalFileID
        m_CalFileID = m_Calindex;
        //And the Name of it after it has been loaded
        m_CalFileName = m_CalFileList[m_Calindex];

        bool useCalMan = false;

        if(CalFileString.substr(21, 1) == "C") {
            useCalMan = true;
        }

        //if we are are not using a Coustom whitesepec and that we do not have a calMan file, then we can parse whitespec out
        //        if(!m_UseCustomWhiteSpec) {
        //            if(useCalMan) {
        //                m_defaultWhiteSpec.x = 76.01171875;
        //                m_defaultWhiteSpec.y = 80;
        //                m_defaultWhiteSpec.z = 87.07421875;
        //                m_defaultWhiteSpec.xyVar = 0.005;
        //                m_defaultWhiteSpec.yVar = 5;
        //            } else {
        //                for(int i = 0; i < 6; ++i) {
        //                    PartString = CalFileString.substr((60 + i * 3) - 1, 3);
        //                    if(i == 0) {
        //                        m_defaultWhiteSpec.x = unpackK_float(PartString);
        //                    } else if(i == 1) {
        //                        m_defaultWhiteSpec.y = unpackK_float(PartString);
        //                    } else if(i == 2) {
        //                        m_defaultWhiteSpec.z = unpackK_float(PartString);
        //                    } else if(i == 3) {
        //                        m_defaultWhiteSpec.xyVar = unpackK_float(PartString);
        //                    } else if(i == 4) {
        //                        m_defaultWhiteSpec.yVar = unpackK_float(PartString);
        //                    }
        //                }
        //            }
        //        }

        if(useCalMan) {
            //Calman does not use RGBMatrix, so using default
            //            m_rawCalRGBMatrix[0] = 3.85266;
            //            m_rawCalRGBMatrix[1] = -1.7702026;
            //            m_rawCalRGBMatrix[2] = -0.59213258;
            //            m_rawCalRGBMatrix[3] = -1.09594727;
            //            m_rawCalRGBMatrix[4] = 2.027709961;
            //            m_rawCalRGBMatrix[5] = 0.020358086;
            //            m_rawCalRGBMatrix[6] = 0.05136680603;
            //            m_rawCalRGBMatrix[7] = -0.190895080566;
            //            m_rawCalRGBMatrix[8] = 0.9453125;

            //Cal mans is in a different location and needs to be unpacked a different way
            for(int i = 0; i < 9; ++i) {
                //Only part of the ReadString will be read
                PartString = CalFileString.substr(24 + (i * 8), 8);
                //After finding the float then we store it
                CalMatrix[i] = unpackCalMan_float(PartString);
            }
        } else {
            //            for(int i = 0; i < 9; ++i) {
            //                PartString = CalFileString.substr((75 + (i) * 3) - 1, 3);
            //                m_rawCalRGBMatrix[i] = unpackK_float(PartString);
            //            }
            for(int i = 0; i < 9; ++i) {
                //Only part of the ReadString will be read
                PartString = CalFileString.substr((102 + i * 3) - 1, 3);
                //After finding the float then we store it
                CalMatrix[i] = unpackK_float(PartString);
            }
        }
    }

    //    if(!m_UseCustomWhiteSpec) {
    //        m_WhiteSpec = m_defaultWhiteSpec;
    //    }

    //    RGBPercent[0] = m_rawCalRGBMatrix[0] * m_WhiteSpec.x + m_rawCalRGBMatrix[1] * m_WhiteSpec.y + m_rawCalRGBMatrix[2] * m_WhiteSpec.z;
    //    RGBPercent[1] = m_rawCalRGBMatrix[3] * m_WhiteSpec.x + m_rawCalRGBMatrix[4] * m_WhiteSpec.y + m_rawCalRGBMatrix[5] * m_WhiteSpec.z;
    //    RGBPercent[2] = m_rawCalRGBMatrix[6] * m_WhiteSpec.x + m_rawCalRGBMatrix[7] * m_WhiteSpec.y + m_rawCalRGBMatrix[8] * m_WhiteSpec.z;

    //    for(int i = 0; i < 3; ++i) {
    //        if(RGBPercent[i] == 0) {
    //            RGBPercent[i] = 0.0001;
    //        }
    //    }

    int c = 0;
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            //Saving the CalMatrix
            m_CalMatrix.v[i][j] = CalMatrix[c];
            //            //Saving the RGB Matrix
            //            m_CalRGBMatrix.v[i][j] = m_rawCalRGBMatrix[c] * 100 / RGBPercent[i];
            ++c;
        }
    }
}
void KClmtr::setTempCalFile(const CorrectedCoefficient &coeff) {
    m_CalFileID = 0;
    m_CalFileName = "Temporary Cal File";

    delete m_CalMatrix.v;
    m_CalMatrix.initializeV(3, 3);

    int c = 0;
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            //m_rawCalRGBMatrix[c] = coeff.rgbMatrix[i][j];
            m_CalMatrix.v[i][j] = coeff.colorMatrix.v[i][j];
            ++c;
        }
    }
    //    m_defaultWhiteSpec = whiteSpec;

    //    if(!m_UseCustomWhiteSpec) {
    //        m_WhiteSpec = m_defaultWhiteSpec;
    //    }

    //    reLoadRGB();
}
double KClmtr::unpackCalMan_float(string PartString) {
    double MyBigDoub[8];
    const unsigned char *myRead = (const unsigned char *)PartString.c_str();
    for(int i = 0; i < 8; ++i) {
        MyBigDoub[i] = myRead[i];
    }

    int MySign;
    double MyExp, MyFraction, MyNumber;
    if(MyBigDoub[7] >= 128) {
        MySign = -1;
        MyExp = MyBigDoub[7] - 128;
    } else {
        MySign = 1;
        MyExp = MyBigDoub[7];
    }

    //make room for the 4 LSB bits
    MyExp = MyExp * 16;
    //append the left 4 bits of MyBigDoub(6). #1022 offset is strange but true
    MyExp = (MyExp + (int)(MyBigDoub[6] / 16)) - 1022;

    //MSB, append a '1'
    MyFraction = ((MyBigDoub[6] / 16) - (int)(MyBigDoub[6] / 16)) * 16 + 16;
    MyFraction = MyFraction * 256 + MyBigDoub[5];
    //more than this is silly resolution
    MyFraction = MyFraction * 256 + MyBigDoub[4];
    //this resolution is silly and there are 3 more we are not even looking at
    MyFraction = MyFraction * 256 + MyBigDoub[3];
    MyFraction = MyFraction / 256 / 256 / 256 / 32;

    MyNumber = exp(MyExp * log(2.0)) * MyFraction;

    MyNumber = MyNumber * MySign;

    //index varies from 0 to 8
    return MyNumber;
}
//CalFiles - setting up to store
static double getDeterminant(double v1, double v2, double v3,
                             double v4, double v5, double v6,
                             double v7, double v8, double v9) {
    double Determinant;
    Determinant = v1 * v8 * v6 +
                  v4 * v2 * v9 +
                  v7 * v5 * v3 -
                  v3 * v4 * v8 -
                  v2 * v6 * v7 -
                  v1 * v5 * v9;
    return Determinant;
}
static unsigned int distruibuteWhiteToRGB(const WRGB &ref, Matrix<double> &distributed) {
    distributed.initializeV(3, 3);
    //convert XYZ to xyz values and use them
    double R[10];
    double Rt, Gt, Bt, Denom;
    double OldRt, oldGt, oldBt;
    OldRt = ref.v[1][0] + ref.v[1][1] + ref.v[1][2];
    //convert Red XYZ to Red xyz
    R[1] = ref.v[1][0] / OldRt;
    R[2] = ref.v[1][1] / OldRt;
    R[3] = ref.v[1][2] / OldRt;

    oldGt = ref.v[2][0] + ref.v[2][1] + ref.v[2][2];
    //convert Green XYZ to Green xyz
    R[4] = ref.v[2][0] / oldGt;
    R[5] = ref.v[2][1] / oldGt;
    R[6] = ref.v[2][2] / oldGt;

    oldBt = ref.v[3][0] + ref.v[3][1] + ref.v[3][2];
    //convert Blue XYZ to Blue xyz
    R[7] = ref.v[3][0] / oldBt;
    R[8] = ref.v[3][1] / oldBt;
    R[9] = ref.v[3][2] / oldBt;

    Denom = getDeterminant(R[1], R[2], R[3], R[4], R[5], R[6], R[7], R[8], R[9]);

    if(Denom == 0) {
        //<-Next number
        return KleinsErrorCodes::CAL_WHITE_RGB;
    } else {
        //solve for determinant values by substituting white XYZ into matrices
        Rt = getDeterminant(ref.v[0][0], ref.v[0][1], ref.v[0][2], R[4], R[5], R[6], R[7], R[8], R[9]) / Denom;
        Gt = getDeterminant(R[1], R[2], R[3], ref.v[0][0], ref.v[0][1], ref.v[0][2], R[7], R[8], R[9]) / Denom;
        Bt = getDeterminant(R[1], R[2], R[3], R[4], R[5], R[6], ref.v[0][0], ref.v[0][1], ref.v[0][2]) / Denom;
        // Rt, Gt, Bt values are nearly equal to one? or what?
        //double myval;
        //myval = Ref.v[1][0] + Ref.v[1][1] + Ref.v[1][2];

        //NEW.. sacrifice some white xy to get back some RGB Y truth
        Rt = (2 * Rt + 1 * OldRt) / 3;
        Gt = (2 * Gt + 1 * oldGt) / 3;
        Bt = (2 * Bt + 1 * oldBt) / 3;

        distributed.v[0][0] = R[1] * Rt;
        distributed.v[0][1] = R[2] * Rt;
        distributed.v[0][2] = R[3] * Rt;
        distributed.v[1][0] = R[4] * Gt;
        distributed.v[1][1] = R[5] * Gt;
        distributed.v[1][2] = R[6] * Gt;
        distributed.v[2][0] = R[7] * Bt;
        distributed.v[2][1] = R[8] * Bt;
        distributed.v[2][2] = R[9] * Bt;

        return KleinsErrorCodes::NONE;
    }
}

CorrectedCoefficient KClmtr::getCoefficientTestMatrix(const WRGB &reference, const WRGB &kclmtr) {
    CorrectedCoefficient corrected;
    Matrix<double> refDist, kclmtrDist;
    refDist.initializeV(3, 3);
    kclmtrDist.initializeV(3, 3);

    unsigned int error = 0;
    error |= distruibuteWhiteToRGB(reference, refDist);
    error |= distruibuteWhiteToRGB(kclmtr, kclmtrDist);

    Matrix<double> targetRGB;
    targetRGB.initializeV(3, 3);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            if(i == j) {
                targetRGB.v[i][j] = 100;
            } else {
                targetRGB.v[i][j] = 0;
            }
        }
    }

    makeCorrectedXYZ(refDist, kclmtrDist, corrected.colorMatrix);
    makeCorrectedXYZ(targetRGB, refDist, corrected.rgbMatrix);

    corrected.error |= error;

    return corrected;
}
void KClmtr::makeCorrectedXYZ(const Matrix<double> &target, const Matrix<double> &kclmtr, Matrix<double> &correctedXYZ) {
    //This uses kclmtr(2,2), Target(2,2), and CorrectedXYZ(2,2), so stash and restore these matrices if you are "borrowing" this subroutine
    //It gives the CorrectedXYZ() matrix which makes kclmtr() into Target(), ie  Target() = CorrectedXYZ() * kclmtr()
    //So if you want the inversion matrix of kclmtr() then use Target() = (1,1,1) (except in 3 dimensions), to calc CorrectedXYZ()

    //input kclmtr(2,2) and Target(2,2) matrices such that:
    //kclmtr(0,0)  kclmtr(0,1)  kclmtr(0,2) are the XYZ of RED Measured Field
    //kclmtr(1,0)  kclmtr(1,1)  kclmtr(1,2) are the XYZ of GREEN Measured Field
    //kclmtr(2,0)  kclmtr(2,1)  kclmtr(2,2) are the XYZ of BLUE Measured Field
    //
    //Target(0,0)  Target(0,1)  Target(0,2) are the Correct XYZ of RED   Field
    //Target(1,0)  Target(1,1)  Target(1,2) are the Correct XYZ of GREEN Field
    //Target(2,0)  Target(2,1)  Target(2,2) are the Correct XYZ of BLUE  Field
    //
    //create CorrectedXYZ(2,2) matrix which makes an X() into XC():
    //ie True X = gA_fromHead(0,0)*X + gA_fromHead(0,1)*Y + gA_fromHead(0,2)*Z (True X from measured XYZ)
    //ie True Y = gA_fromHead(1,0)*X + gA_fromHead(1,1)*Y + gA_fromHead(1,2)*Z (True Y from measured XYZ)
    //ie True Z = gA_fromHead(2,0)*X + gA_fromHead(2,1)*Y + gA_fromHead(2,2)*Z (True Z from measured XYZ)

    double Denom = kclmtr.v[0][0] * kclmtr.v[1][1] * kclmtr.v[2][2] +
                   kclmtr.v[1][0] * kclmtr.v[2][1] * kclmtr.v[0][2] +
                   kclmtr.v[2][0] * kclmtr.v[0][1] * kclmtr.v[1][2] -
                   kclmtr.v[0][2] * kclmtr.v[1][1] * kclmtr.v[2][0] -
                   kclmtr.v[0][1] * kclmtr.v[1][0] * kclmtr.v[2][2] -
                   kclmtr.v[0][0] * kclmtr.v[1][2] * kclmtr.v[2][1];

    if(Denom == 0) {
        Denom = 0.00001;
    }

    //substitute XC(0,0),XC(1,0),XC(2,0) for X(0,0),X(1,0),X(2,0)
    double top = (target.v[0][0] * kclmtr.v[1][1] * kclmtr.v[2][2] +
            target.v[1][0] * kclmtr.v[2][1] * kclmtr.v[0][2] +
            target.v[2][0] * kclmtr.v[0][1] * kclmtr.v[1][2] -
            kclmtr.v[0][2] * kclmtr.v[1][1] * target.v[2][0] -
            kclmtr.v[0][1] * target.v[1][0] * kclmtr.v[2][2] -
            target.v[0][0] * kclmtr.v[1][2] * kclmtr.v[2][1]);
    double number = top / Denom;
    correctedXYZ.v[0][0] =  number;
    //insert target.v[0,0],target.v[1,0],target.v[2,0] for kclmtr.v[0,1],kclmtr.v[1,1],kclmtr.v[2,1]
    top = (kclmtr.v[0][0] * target.v[1][0] * kclmtr.v[2][2] +
            kclmtr.v[1][0] * target.v[2][0] * kclmtr.v[0][2] +
            kclmtr.v[2][0] * target.v[0][0] * kclmtr.v[1][2] -
            kclmtr.v[0][2] * target.v[1][0] * kclmtr.v[2][0] -
            target.v[0][0] * kclmtr.v[1][0] * kclmtr.v[2][2] -
            kclmtr.v[0][0] * kclmtr.v[1][2] * target.v[2][0]);
    number = top / Denom;
    correctedXYZ.v[0][1] =  number;
    //insert target.v[0,0],target.v[1,0],target.v[2,0] for kclmtr.v[0,2],kclmtr.v[1,2],kclmtr.v[2,2]
    top = (kclmtr.v[0][0] * kclmtr.v[1][1] * target.v[2][0] +
            kclmtr.v[1][0] * kclmtr.v[2][1] * target.v[0][0] +
            kclmtr.v[2][0] * kclmtr.v[0][1] * target.v[1][0] -
            target.v[0][0] * kclmtr.v[1][1] * kclmtr.v[2][0] -
            kclmtr.v[0][1] * kclmtr.v[1][0] * target.v[2][0] -
            kclmtr.v[0][0] * target.v[1][0] * kclmtr.v[2][1]);
    number = top / Denom;
    correctedXYZ.v[0][2] =  number;
    //substitute target.v[0,1],target.v[1,1],target.v[2,1] for kclmtr.v[0,0],kclmtr.v[1,0],kclmtr.v[2,0]
    top = (target.v[0][1] * kclmtr.v[1][1] * kclmtr.v[2][2] +
            target.v[1][1] * kclmtr.v[2][1] * kclmtr.v[0][2] +
            target.v[2][1] * kclmtr.v[0][1] * kclmtr.v[1][2] -
            kclmtr.v[0][2] * kclmtr.v[1][1] * target.v[2][1] -
            kclmtr.v[0][1] * target.v[1][1] * kclmtr.v[2][2] -
            target.v[0][1] * kclmtr.v[1][2] * kclmtr.v[2][1]);
    number = top / Denom;
    correctedXYZ.v[1][0] = number;
    //substitute target.v[0,1],target.v[1,1],target.v[2,1] for kclmtr.v[0,1],kclmtr.v[1,1],kclmtr.v[2,1]
    top = (kclmtr.v[0][0] * target.v[1][1] * kclmtr.v[2][2] +
            kclmtr.v[1][0] * target.v[2][1] * kclmtr.v[0][2] +
            kclmtr.v[2][0] * target.v[0][1] * kclmtr.v[1][2] -
            kclmtr.v[0][2] * target.v[1][1] * kclmtr.v[2][0] -
            target.v[0][1] * kclmtr.v[1][0] * kclmtr.v[2][2] -
            kclmtr.v[0][0] * kclmtr.v[1][2] * target.v[2][1]);
    number = top / Denom;
    correctedXYZ.v[1][1] = number;
    //substitute target.v[0,1],target.v[1,1],target.v[2,1] for kclmtr.v[0,2],kclmtr.v[1,2],kclmtr.v[2,2]
    top = (kclmtr.v[0][0] * kclmtr.v[1][1] * target.v[2][1] +
            kclmtr.v[1][0] * kclmtr.v[2][1] * target.v[0][1] +
            kclmtr.v[2][0] * kclmtr.v[0][1] * target.v[1][1] -
            target.v[0][1] * kclmtr.v[1][1] * kclmtr.v[2][0] -
            kclmtr.v[0][1] * kclmtr.v[1][0] * target.v[2][1] -
            kclmtr.v[0][0] * target.v[1][1] * kclmtr.v[2][1]);
    number = top / Denom;
    correctedXYZ.v[1][2] =  number;

    //substitute target.v[0,2],target.v[1,2],target.v[2,2] for kclmtr.v[0,0],kclmtr.v[1,0],kclmtr.v[2,0]
    top = (target.v[0][2] * kclmtr.v[1][1] * kclmtr.v[2][2] +
            target.v[1][2] * kclmtr.v[2][1] * kclmtr.v[0][2] +
            target.v[2][2] * kclmtr.v[0][1] * kclmtr.v[1][2] -
            kclmtr.v[0][2] * kclmtr.v[1][1] * target.v[2][2] -
            kclmtr.v[0][1] * target.v[1][2] * kclmtr.v[2][2] -
            target.v[0][2] * kclmtr.v[1][2] * kclmtr.v[2][1]);
    number = top / Denom;
    correctedXYZ.v[2][0] =  number;
    //substitute target.v[0,2],target.v[1,2],target.v[2,2] for kclmtr.v[0,1],kclmtr.v[1,1],kclmtr.v[2,1]
    top = (kclmtr.v[0][0] * target.v[1][2] * kclmtr.v[2][2] +
            kclmtr.v[1][0] * target.v[2][2] * kclmtr.v[0][2] +
            kclmtr.v[2][0] * target.v[0][2] * kclmtr.v[1][2] -
            kclmtr.v[0][2] * target.v[1][2] * kclmtr.v[2][0] -
            target.v[0][2] * kclmtr.v[1][0] * kclmtr.v[2][2] -
            kclmtr.v[0][0] * kclmtr.v[1][2] * target.v[2][2]);
    number = top / Denom;
    correctedXYZ.v[2][1] = number;
    //substitute target.v[0,2],target.v[1,2],target.v[2,2] for kclmtr.v[0,2],kclmtr.v[1,2],kclmtr.v[2,2]
    top = (kclmtr.v[0][0] * kclmtr.v[1][1] * target.v[2][2] +
            kclmtr.v[1][0] * kclmtr.v[2][1] * target.v[0][2] +
            kclmtr.v[2][0] * kclmtr.v[0][1] * target.v[1][2] -
            target.v[0][2] * kclmtr.v[1][1] * kclmtr.v[2][0] -
            kclmtr.v[0][1] * kclmtr.v[1][0] * target.v[2][2] -
            kclmtr.v[0][0] * target.v[1][2] * kclmtr.v[2][1]);
    number = top / Denom;
    correctedXYZ.v[2][2] = number;
}

string appendMatrixPassword(char ID, string str) {
    //MAT*id*(
    char pre[] = {77, 65, 84, ID, 40};
    return string(pre, 5) + str + ")";
}
int KClmtr::deleteCalFile(int calFileID) {
    try {
        //The User Matrices stored in the head (#1-96) are packed as follows    LJ 12/3/02
        //  1 - 20   20 bytes  Name  byte 1- xx
        //  21 - 22    2 bytes  Reserved for later (need to store info? ask.)
        //    23      1 bit of byte 23 xy,uv mode LSB is a flag: 0, geCurrentSpec_xy_uv_ViewMode= geSpec_ViewMode_xy,   1 = geSpec_ViewMode_uv
        //  24 - 59   36 bytes  gsinCalMeasColorsFromHead.. to be able to display what created the XYZ cal file
        //  60 - 74   15 bytes  White Spec in XYZ format and Tolerance 5 signed klein floats
        //  75 - 101  27 bytes  RGB% Matrix converts converted XYZ to %
        //102 - 128  27 bytes  XYZ Matrix converts received XYZ to recalibrated XYZ
        string emptyCalFile = "";

        if(getCalFileID() == calFileID) {
            setCalFileID(0);
        }
        //Creating the blank Cal File
        for(int i = 0; i < 128; ++i) {
            emptyCalFile += (char)255;
        }

        emptyCalFile = appendMatrixPassword(calFileID, emptyCalFile);

        //Setting up to store
        string returnString;
        unsigned int error = sendMessageToKColorimeter(CALFILE_SAVING, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        if(returnString != "D9") {
            return KleinsErrorCodes::CAL_STORING;
        }

        //Saving CalFile
        error = sendMessageToSerialPort(m_CommPort, emptyCalFile, 3, 5, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        if(returnString == "<e>") {
            return KleinsErrorCodes::CAL_STORING;
        }
        //Get all the CalFiles on device
        error = sendMessageToKColorimeter(CALFILE_FILELIST, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        setCalFileList(returnString);
        return KleinsErrorCodes::NONE;
    } catch(...) {
        //Storing CalFile Failed.
        return KleinsErrorCodes::CAL_STORING;
    }
}
//CalFile - Store
int KClmtr::storeMatrices(int id, const string &name, const WRGB &reference, const WRGB &kclmtr) {
    //Gets the correctionMatrix
    CorrectedCoefficient corrected = getCoefficientTestMatrix(reference, kclmtr);
    return storeMatrices(id, name, corrected);


}
int KClmtr::storeMatrices(int id, const string &name, const Matrix<double> &correctedXYZ) {
    CorrectedCoefficient corrected;

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            corrected.colorMatrix.v[i][j] = correctedXYZ.v[i][j];
        }
    }

    corrected.rgbMatrix.v[0][0] = 3.85266;
    corrected.rgbMatrix.v[0][1] = -1.7702026;
    corrected.rgbMatrix.v[0][2] = -0.59213258;
    corrected.rgbMatrix.v[1][0] = -1.09594727;
    corrected.rgbMatrix.v[1][1] = 2.027709961;
    corrected.rgbMatrix.v[1][2] = 0.020358086;
    corrected.rgbMatrix.v[2][0] = 0.05136680603;
    corrected.rgbMatrix.v[2][1] = -0.190895080566;
    corrected.rgbMatrix.v[2][2] = 0.9453125;

    return storeMatrices(id, name, corrected);
}

int KClmtr::storeMatrices(int id, const string &name, const CorrectedCoefficient &correctionMatrix) {
    try {
        string CalFile;
        //Setting up to store
        string returnString;
        unsigned int error = sendMessageToKColorimeter(CALFILE_SAVING, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        if(returnString != "D9") {
            return KleinsErrorCodes::CAL_STORING;
        }

        //Adding the everything in one
        CalFile = packUserMatrix(name, correctionMatrix, error);
        if(error != KleinsErrorCodes::NONE){
            return error;
        }
        //Adding the Password
        CalFile = appendMatrixPassword(id, CalFile);
        error = sendMessageToSerialPort(m_CommPort, CalFile, 3, 5, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        if(returnString == "<e>") {
            return KleinsErrorCodes::CAL_STORING;
        }
        //Get all the CalFiles on device
        error = sendMessageToKColorimeter(CALFILE_FILELIST, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        setCalFileList(returnString);
        return KleinsErrorCodes::NONE;
    } catch(...) {
        //Storing CalFile Failed
        return KleinsErrorCodes::CAL_STORING;
    }
}

string KClmtr::packUserMatrix(string name, CorrectedCoefficient corrected, unsigned int &error) {
    unsigned char *calFile = new unsigned char[128];
    for(int i = 0; i < 128; ++i) {
        calFile[i] = 0;
    }

    //  1 - 20   20 bytes  Name  byte 1- xx
    for(int i = 0; i < 20; ++i) {
        calFile[i] = ' ';
    }
    for(int i = 0; i < (int)name.size() && i < 20; ++i) {
        calFile[i] = name[i];
    }

    //  21 - 22    2 bytes  Reserved for later (need to store info? ask.)
    calFile[20] = '/';
    calFile[21] = '/';

    //    23      1 bit of byte 23 xy,uv mode LSB is a flag: 0, geCurrentSpec_xy_uv_ViewMode= geSpec_ViewMode_xy,   1 = geSpec_ViewMode_uv
    //calFile[22] = 0;

    //  24 - 59   36 bytes  gsinCalMeasColorsFromHead.. to be able to display what created the XYZ cal file
    // seem to be 0

    //  60 - 74   15 bytes  White Spec in XYZ format and Tolerance 5 signed Klein floats
    memcpy(calFile + 59 + 3 * 0, convertNumberToBinaryStr(95.043, error), 3);
    memcpy(calFile + 59 + 3 * 1, convertNumberToBinaryStr(100.000, error), 3);
    memcpy(calFile + 59 + 3 * 2, convertNumberToBinaryStr(108.890, error), 3);
    memcpy(calFile + 59 + 3 * 3, convertNumberToBinaryStr(0.005, error), 3);
    memcpy(calFile + 59 + 3 * 4, convertNumberToBinaryStr(5.00, error), 3);

    //  75 - 101  27 bytes  RGB% Matrix converts converted XYZ to %
    for(int y = 0, i = 74; y < 3; ++y) {
        for(int x = 0; x < 3; ++x) {
            memcpy(calFile + i, convertNumberToBinaryStr(corrected.rgbMatrix.v[y][x], error), 3);
            i += 3;
        }
    }

    //102 - 128  27 bytes  XYZ Matrix converts received XYZ to recalibrated XYZ
    for(int y = 0, i = 101; y < 3; ++y) {
        for(int x = 0; x < 3; ++x) {
            memcpy(calFile + i, convertNumberToBinaryStr(corrected.colorMatrix.v[y][x], error), 3);
            i += 3;
        }
    }

    return string((char *)calFile, 128);
}

unsigned char *KClmtr::convertNumberToBinaryStr(double v, unsigned int &error) {
    //input single, output 3 chr string
    double TempReal, expReal, fractionReal;
    int NegBit, chr1, chr2, chr3;
    unsigned char *theConvert = new unsigned char[3];
    //Kfloat (Klein float) is a 3byte signed floating point number
    //chr1 chr2 chr3, where chr1 MSB is the sign, the remaining 7 bits of chr1 and 8 bits of chr2
    //are a fraction magnitude, where 1000000 000000000 is ".5"
    //chr3 is the 2's exponent (signed). To convert:
    if(v == 0) {
        theConvert[0] = theConvert[1] = theConvert[2] = 0;
        return theConvert;
    }

    if(v < 0) {
        NegBit = -1;
        v = -v;
    } else {
        NegBit = 1;
    }

    TempReal = log(v) / log(2.0);
    //expReal is now 2's exponent
    expReal = (int)TempReal + 1;
    fractionReal = exp(expReal * log(2.0));
    fractionReal = v / fractionReal;
    fractionReal = (int)(fractionReal * 128 * 256 + 0.5);
    if(fractionReal >= (128 * 256)) {
        fractionReal = fractionReal / 2;
        expReal = expReal + 1;
    }

    //MSB of fraction without sign
    chr1 = (int)(fractionReal / 256);
    //LSB of fraction
    chr2 = (int)fractionReal - 256 * chr1;
    //exponent
    chr3 = (int)expReal;

    if(NegBit < 0) {
        chr1 = chr1 + 128;
    }
    if(chr3 < 0) {
        chr3 = chr3 + 256;
    }

    if((chr1 > 256) | (chr1 < 0) | (chr2 > 256) | (chr2 < 0) | (chr3 > 256) | (chr3 < 0)) {
        error |= KleinsErrorCodes::CAL_CONVERT_BINARY;
    }

    theConvert[0] = chr1;
    theConvert[1] = chr2;
    theConvert[2] = chr3;
    return theConvert;
}
//BlackCal
bool KClmtr::checkTH1andTH2(const Counts &counts) {
    bool CheckTH1_and_TH2;
    CheckTH1_and_TH2 = true;
    if((counts.th1 < 50) || (counts.th1 > 200)) {
        CheckTH1_and_TH2 = false;
    }
    if((counts.th2 < 50) || (counts.th2 > 200)) {
        CheckTH1_and_TH2 = false;
    }
    return CheckTH1_and_TH2;
}

//BlackCal - Cold
BlackMatrix KClmtr::captureBlackLevel() {
    Counts counts = getNextMeasureCount();
    //Checks whether or not TH1 or TH2 is between 50 and 200
    if(!checkTH1andTH2(counts)) {
        return BlackMatrix(counts.errorcode | KleinsErrorCodes::BLACK_STORING_ROM);
    } else {
        //If so Grab the ranges
        string returnString;
        unsigned int error = sendMessageToKColorimeter(BLACKCAL_NEW_RAM, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return BlackMatrix(error);
        }
        //parsing it out in array
        BlackMatrix ranges = BlackMatrix(returnString, error);
        if(ranges.errorcode != KleinsErrorCodes::NONE) {
            return ranges;
        }
        //getting ready to store the Matrix
        error = sendMessageToKColorimeter(BLACKCAL_STORE, returnString);
        if(error != KleinsErrorCodes::NONE || returnString != BLACKCAL_STORE.commandString) {
            return BlackMatrix(error);
        }
        //Storing the Matrix with password
        error = sendMessageToKColorimeter("{00000000}@%#\r", 3, 1, returnString);
        if(error != KleinsErrorCodes::NONE) {
            //Commucation error
            return BlackMatrix(error);
        }
        if(returnString.substr(0, 3) != "<0>") {
            //Error storing
            error |= KleinsErrorCodes::BLACK_STORING_ROM;
            return BlackMatrix(error);
        }
        return ranges;
    }
}
BlackMatrix KClmtr::getFlashMatrix() {
    string returnString;
    unsigned int error = sendMessageToKColorimeter(BLACKCAL_FLASH, returnString);
    if(error != KleinsErrorCodes::NONE) {
        return BlackMatrix(error);
    }
    return BlackMatrix(returnString, error);;
}
//BlackCal - Hot
BlackMatrix KClmtr::getRAMMatrix() {
    string returnString;
    int error = sendMessageToKColorimeter(BLACKCAL_RAM, returnString);
    if(error != KleinsErrorCodes::NONE) {
        return BlackMatrix(error);
    }
    return BlackMatrix(returnString, error);
}
BlackMatrix KClmtr::getCoefficientMatrix() {
    string returnString;
    unsigned int error = sendMessageToKColorimeter(BLACKCAL_COEFFICIENTMATRIX, returnString);
    if(error != 0) {
        return BlackMatrix(error);
    }

    const unsigned char *myRead = (const unsigned char *)returnString.c_str();
    double mySin;
    double matrix[19];

    for(int i = 0; i < 18; ++i) {
        mySin = (int)myRead[(i + 1) * 2];
        mySin = mySin * 256 + (int)myRead[(i + 1) * 2 + 1];
        if(mySin > 32768) {
            mySin = 32768 - mySin;
        }
        mySin = mySin / 4096;
        matrix[i] = mySin;
    }

    BlackMatrix coefficientMatrix(matrix, error);
    coefficientMatrix.therm = 0;
    return coefficientMatrix;
}
//FFT
bool KClmtr::isFlickering() const {
    return m_Flickering;
}
void KClmtr::resetFlicker() {
    m_Flickering = false;
    //Setting FFT data with the correct array lenght
    if(m_ParsedOutRippleArray != NULL) {
        delete[] m_ParsedOutRippleArray;
        m_ParsedOutRippleArray = NULL;
    }
    m_ParsedOutRippleArray = new double[m_flickerSettings.samples];

    for(int i = 0; i < m_flickerSettings.samples; ++i) {
        m_ParsedOutRippleArray[i] = 0;
    }
    m_fft_numPass = m_flickerSettings.samples / 32;

    m_fft_count = 0;

    m_fft_lastRange = -1;
}
int KClmtr::startFlicker() {
    return startFlicker(true);
}
int KClmtr::startFlicker(bool grabConstantly) {
    //Setting FFT data with the correct array lenght
    resetFlicker();
    if(m_Model.find("KV-") == 0) {
        return KleinsErrorCodes::FFT_NOT_SUPPORTED;
    }
    stopFlicker();
    unsigned int error = KleinsErrorCodes::NONE;
    //Don't have range coef for flicker
    //Need to get them
    string returnString;
    if(!checkCoef()) {
        error = sendMessageToKColorimeter(FLICKER_INFO, returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        error = printStartupString(returnString);
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
    }
    //Have the Coef now,
    //and does it pass the test
    if(!checkCoef()) {
        return KleinsErrorCodes::FFT_RANGE_CAL;
    }
    m_Flickering = true;
    //Starting Thread if needed
    if(grabConstantly) {
        m_Flickering2 = false;
        startFlicker2();
        return KleinsErrorCodes::NONE;
    } else {
        if(isFlickerNew()) {
            m_flickerSettings.speed = 384;
            error = sendMessageToKColorimeter(FLICKER_384PERSECOND);
            if(error != KleinsErrorCodes::NONE) {
                return error;
            }
            m_CommPort.setSetting(9600 * 2, 8, 'n', 10000);
        } else {
            m_flickerSettings.speed = 256;
            error = sendMessageToKColorimeter(FLICKER_256PERSECOND);
        }
        if(error != KleinsErrorCodes::NONE) {
            return error;
        }
        sleep(10);
        return KleinsErrorCodes::NONE;
    }
}
unsigned int KClmtr::printStartupString(const string &read) {
    //reply to P4 (get startup string), 613 bytes in gBstrMsgAccum incls sn, vers, usermatrix names,fft matrices
    //     P4                       2
    //     model/sn:               16 (7 bytes model, 9 bytes sn#)
    //     firmw vers:              8
    //     fft matrices = 128*3 = 384  ' parse these so we do not have to get them later
    //     user names:  20 * 10 = 200  ' stash these 200 bytes for use later
    //     <0>                      3
    //     total:                 613 bytes

    m_firmware = read.substr(19, 7); //8 bytes FirmWareVersion

    //Load the FFT Matrix
    return loadFFTRangeCal(read.substr(26, 384));
}
unsigned int KClmtr::loadFFTRangeCal(const string &readString) {
    //InString is binary string
    double MyArray[3][129];
    double MySin;
    unsigned char Byte1;
    unsigned char Byte2;
    const unsigned char *InArray;
    string MyBStr;
    if(readString.length() != (128 * 3)) {
        return KleinsErrorCodes::FFT_RANGE_CAL;
    }

    //range 0 = [1 and 2], 1 = [3 and 4], 2 = [5 and 6]
    for(int i = 0; i < 3; ++i) {
        MyBStr = readString.substr(i * 128, 128);
        //InArray indexed 0-127
        InArray = (const unsigned char *)MyBStr.c_str();
        for(int j = 0; j < 127; j += 2) {
            Byte1 = InArray[j];
            Byte2 = InArray[j + 1];

            MySin = Byte1;

            MySin = MySin * 256;

            MySin = MySin + Byte2;
            if(MySin == 0) {
                MySin = 0.0000001;
            }
            MySin = 1 / MySin;
            //MyArray indexed 1-128
            MyArray[i][j + 1] = MySin * 256 * 128;
        }

        MyArray[i][0] = MyArray[i][1];
        for(int j = 1; j < 126; j += 2) {
            MyArray[i][j + 1] = (MyArray[i][j] + MyArray[i][j + 2]) / 2;
        }

        //(99) + ((99)-(98))
        MySin = 2 * MyArray[i][99] - MyArray[i][98];
        if(MySin != 0) {
            //If (100) is not approx linear to 98,99 then make it so
            if(fabs((MyArray[i][100] - MySin) / MySin) > 0.02) {
                MyArray[i][100] = MySin;
                MyArray[i][101] = 2 * MySin - MyArray[i][99];
            }
        }

        if(MyArray[i][102] < 1) {
            for(int j = 102; j < 129; ++j) {
                MyArray[i][j] = 0;
            }
        }

        MyArray[i][128] = MyArray[i][127];
    }
    unsigned int error = checkThreeFFT_Validity(MyArray);
    if(error != KleinsErrorCodes::NONE) {
        return error;
    }
    caluclateCoef(MyArray);
    return KleinsErrorCodes::NONE;
}
void KClmtr::caluclateCoef(double array[][129]) {
    for(int i = 0; i < 3; ++i) {
        Matrix<double> data;
        data.initializeV(100, 2);
        for(int j = 0; j <= 100; ++j) {
            if(j <= 70) {
                m_FFTCalRangeCoeff1[i][j] = array[i][j];
            }
            if(j > 0) {
                data.v[j-1][0] = j;           //x
                data.v[j-1][1] = array[i][j]; //y
            }
        }
        polyFit(data, polyDegree, m_FFTCalRangeCoeff2[i]);
    }
}
bool KClmtr::checkCoef() {
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < polyDegree; ++j) {
            if(m_FFTCalRangeCoeff2[i][j] == -1) {
                return false;
            }
        }
    }
    return true;
}

unsigned int KClmtr::checkThreeFFT_Validity(double array[][129]) {
    double OldValue;

    for(int i = 0; i < 3; ++i) {
        //check three ranges fft arrays. MyCnt=0,1,2
        if(array[i][1] != 1) {
            return KleinsErrorCodes::FFT_RANGE_CAL;
        }

        if((array[i][99] > 150) || (array[i][99] < 25)) {
            return KleinsErrorCodes::FFT_RANGE_CAL;
        }

        //which is equal to '1' anyway
        OldValue = array[i][1];
        for(int j = 2; j < 100; ++j) {
            if(array[i][j] <= OldValue * 0.99) {
                //must increase monotonically to within 1%
                return KleinsErrorCodes::FFT_RANGE_CAL;
            }
            OldValue = array[i][j];
        }
    }

    return KleinsErrorCodes::NONE;
}
bool KClmtr::getFlicker(Flicker &f) {
    f = m_flicker;
    bool temp = m_isFlickerfresh;
    m_isFlickerfresh = false;
    return temp;
}

Flicker KClmtr::getNextFlicker() {
    m_Flickering2 = true;

    //TOD: check if this works
    if(m_Flickering) {
        stopFlicker();
    }
    Flicker flicker;
    if(startFlicker(false) == KleinsErrorCodes::NONE &&
       m_Flickering) {

        string readString = "";
        m_CommPort.readExisting();
        unsigned int error = readFromKColorimeter(96 * (m_fft_numPass) + 2, 2 * (m_fft_numPass), readString);
        if(error != KleinsErrorCodes::NONE) {
            return Flicker(error);
        }
        //byte[] ByteARray = {84, 128, 163, 50, 95, 131, 82, 79, 51, 159, 86, 115, 11, 107, 163, 124, 130, 115, 66, 147, 3, 9, 154, 163, 184, 143, 115, 36, 112, 131, 3, 84, 211, 73, 80, 19, 60, 95, 115, 48, 118, 195, 62, 139, 51, 95, 151, 227, 95, 152, 99, 95, 130, 195, 95, 97, 99, 95, 79, 131, 95, 85, 99, 95, 106, 19, 95, 129, 3, 95, 146, 35, 95, 154, 147, 95, 145, 4, 95, 115, 4, 95, 86, 20, 95, 79, 164, 95, 93, 244, 95, 117, 36, 95, 138, 4, 84, 151, 84, 50, 153, 4, 81, 132, 196, 237, 99, 100, 11, 79, 244, 123, 84, 116, 136, 104, 132, 9, 127, 164, 185, 145, 68, 148, 154, 100, 3, 146, 84, 73, 117, 68, 60, 87, 100, 48, 79, 84, 62, 92, 148, 95, 115, 132, 95, 136, 212, 95, 150, 180, 95, 153, 132, 95, 134, 196, 95, 101, 100, 95, 80, 116, 95, 83, 132, 95, 102, 244, 95, 126, 68, 95, 144, 100, 95, 154, 36, 95, 147, 164, 95, 119, 164, 95, 88, 212, 95, 79, 20, 95, 91, 84};
        size_t x = readString.length() - 96;
        for(; x > 0; --x) {
            if(readString[x] == 'T' && readString[x + 3] == '2' && readString[x + 42] == '>'
                    && readString[x + 45] == '_' && readString[x + 48] == '_'
                    && readString[x + 51] == '_' && readString[x + 93] == '_') {
                break;
            }
        }
        string correctString = "";
        if(x > 0) {
            readString.erase(0, x % 3);
            x -= x % 3;
            //Coping all the makers and the samples
            for(int i = 0; i < 96 * (m_fft_numPass); ++i) {
                if((i % 3) == 0) {
                    correctString += readString[(i % 96) + x];
                } else {
                    correctString += readString[i];
                }
            }
        }

        if(correctString.length() >= 96U * (m_fft_numPass)) {
            flicker = parseAndPrintFFT(correctString);
        } else {
            return Flicker(KleinsErrorCodes::FFT_BAD_STRING);
        }

        m_Flickering2 = false;
    } else {
        return Flicker(KleinsErrorCodes::FFT_BAD_STRING);
    }

    //Thanks to THIERRY Andre adding this fix
    //Stopping Flicker with a command
    endFlicker();

    return flicker;
}
void KClmtr::setDeviceFlickerSpeed(bool use) {
    m_DeviceFlickerSpeed = use;
}
bool KClmtr::getDeviceFlickerSpeed() const {
    return m_DeviceFlickerSpeed;
}

bool KClmtr::isFlickerNew() {
	if(m_DeviceFlickerSpeed && m_firmware > "01.09fh") {
		return true;
	} else {
		return false;
	}
}
void KClmtr::endFlicker() {
    //Stopping Flicker with a command
    sendMessageToKColorimeter(DUMMY);
    sendMessageToKColorimeter(DUMMY);
    if(isFlickerNew()) {
        sleep(5);
        m_CommPort.setSetting(9600, 8, 'n', 10000);
    } else {
        sleep(10);
        sendMessageToKColorimeter(DUMMY);
    }
    sleep(10);
    m_CommPort.readExisting();
    resetFlicker();
}

void KClmtr::stopFlicker() {
    if(threadModeChild == NOT_RUNNING) {
        resetFlicker();
    } else {
        stopFlicker2();
    }
}
//FFT - Parsing
Flicker KClmtr::parseAndPrintFFT(string &read) {
    if(!verify_FFTString(read)) {
        return Flicker(KleinsErrorCodes::FFT_BAD_STRING);
    }

    //shifts gsinRippleArray() by 32 values, parses and appends
    unsigned int error = parseSignal_from_FFT_str(read);

    //error byte is in gintErrorByte, but just might mess up if multiple colorimeters, so get error byte directly:
    unsigned char ErrorByte = (unsigned char)read[39];

    if((ErrorByte != '0') && (ErrorByte != 'L')) {
        m_fft_lastRange = 100;
    }
    //parse the FFT command to a N5Command and then saves it to g_xyl
    double x, y, z;
    MeasurementRange range;
    error |= parseN5Command(read, x, y, z, range);

    y = x * m_CalMatrix.v[1][0] +
        y * m_CalMatrix.v[1][1] +
        z * m_CalMatrix.v[1][2];

    //Getting Range
    if((int)range != m_fft_lastRange) {
        if(m_fft_lastRange == -1) {
            m_fft_lastRange = (int)range;
        } else {
            //update the PrevRange to current range
            m_fft_lastRange = (int)range;
            error |= (int)KleinsErrorCodes::FFT_PREVIOUS_RANGE;
            if(!m_Flickering2) {
                m_fft_numPass = (m_flickerSettings.speed / 32);
            }
        }
        m_flickerSettings.deleteCorrection();
        m_flickerSettings.appendCorrection(FlickerSetting::Range(0, 70), CorrectionMode::ArrayFit, m_FFTCalRangeCoeff1[(int)((m_fft_lastRange - 1) / 2)], 71);
        m_flickerSettings.appendCorrection(FlickerSetting::Range(70, 151), CorrectionMode::PolyFit, m_FFTCalRangeCoeff2[(int)((m_fft_lastRange - 1) / 2)], polyDegree);
    }
    ++m_fft_count;

    Flicker theFlicker(m_flickerSettings, m_ParsedOutRippleArray, m_fft_count, y);
    theFlicker.bigY = y;
    theFlicker.range = range;

    --m_fft_numPass;
    if(m_fft_numPass > 0) {
        error |= KleinsErrorCodes::FFT_INSUFFICIENT_DATA;
        if(m_Flickering2) {
            read = read.substr(96, (int)read.length() - 96);
            theFlicker.errorcode = error;
            return parseAndPrintFFT(read);
        }
    } else {
        error &= ~KleinsErrorCodes::FFT_INSUFFICIENT_DATA;
    }
    read = read.substr(96, (int)read.length() - 96);

    theFlicker.errorcode = error;
    return theFlicker;


}
bool KClmtr::verify_FFTString(const string &FFTString) {
    if(FFTString[0] != 'T' ||
            FFTString[3] != '2' ||
            FFTString[42] != '>') {
        return false;
    }
    for(int j = 45; j < 96; j += 3) {
        if(FFTString[j] != '_') {
            return false;
        }
    }

    return true;
}
unsigned int KClmtr::parseSignal_from_FFT_str(const string &FFTString) {
    const unsigned char *ByteArray = (const unsigned char *)FFTString.c_str();;

    //256
    for(int i = 0; i < (m_flickerSettings.samples - 32); ++i) {
        //shift array LEFT 32 places
        m_ParsedOutRippleArray[i] = m_ParsedOutRippleArray[i + 32];
    }
    unsigned int error = KleinsErrorCodes::NONE;
    for(int i = 0; i < 32; ++i) {
        int nits = (int)ByteArray[(i + 1) * 3 - 2] * 256 + (int)ByteArray[(i + 1) * 3 - 1];
        //to test for saturation
        if(nits > 61000) {
            error |= (int)KleinsErrorCodes::FFT_OVER_SATURATED;
        }
        //Storing in the last 32 spaces
        m_ParsedOutRippleArray[i + m_flickerSettings.samples - 32] = nits;
    }
    return error;
}
unsigned int KClmtr::parseN5Command(const string &FFTString, double &outX, double &outY, double &outZ, MeasurementRange &outRange) {
    //15 char long string
    string xyzString = "";
    //X
    xyzString = FFTString.substr(6, 1);
    xyzString += FFTString.substr(9, 1);
    xyzString += FFTString.substr(12, 1);

    outX = parseK_float(xyzString);

    //Y
    xyzString = FFTString.substr(15, 1);
    xyzString += FFTString.substr(18, 1);
    xyzString += FFTString.substr(21, 1);

    outY = parseK_float(xyzString);

    //Z
    xyzString = FFTString.substr(24, 1);
    xyzString += FFTString.substr(27, 1);
    xyzString += FFTString.substr(30, 1);

    outZ = parseK_float(xyzString);

    //Range
    int ranges[3];
    parsingRange(FFTString.substr(33, 1).c_str()[0], ranges);
    outRange = (MeasurementRange)ranges[1];

    //Error
    // <0>
    //xyzString += FFTString.substr(36, 1);
    return KleinsErrorCodes::getErrorCodeFromKlein(FFTString.substr(39, 1));
    //xyzString += FFTString.substr(42, 1);

    //return parseAndPrintXYZ(N5CommandString);
}

//Send/Reseving
unsigned int KClmtr::sendMessageToKColorimeter(const command &m) {
    string readString;
    return sendMessageToKColorimeter(m, readString);
}
unsigned int KClmtr::sendMessageToKColorimeter(const command &m, string &readString) {
    return sendMessageToKColorimeter(m.commandString, m.expected, m.timeout, readString);
}
unsigned int KClmtr::sendMessageToKColorimeter(const string &strMsg, int expected, int timeOut_Sec) {
    string readString;
    return sendMessageToKColorimeter(strMsg, expected, timeOut_Sec, readString);
}
unsigned int KClmtr::sendMessageToKColorimeter(const string &strMsg, int expected, int timeOut_Sec, string &readString) {
    if(isMeasuring() &&
            (strMsg != COLOR_2PERSECOND.commandString &&
             strMsg != COLOR_4PERSECOND.commandString &&
             strMsg != COLOR_8PERSECOND.commandString &&
             strMsg != COLOR_16PERSECOND.commandString)) {
        stopMeasuring();
    }
    if(isMeasureCounts() &&
            strMsg != COUNTS_4PERSECOND.commandString) {
        stopMeasureCounts();
    }
    if(isFlickering() && expected != -1 &&
            (strMsg != FLICKER_256PERSECOND.commandString &&
             strMsg != FLICKER_384PERSECOND.commandString)) {
        stopFlicker();
    }
    return sendMessageToSerialPort(m_CommPort, strMsg, expected, timeOut_Sec, readString);
}
unsigned int KClmtr::readFromKColorimeter(int expected, long timeOut_Sec, string &readString) {
    if(m_CommPort.isOpen()) {
        if(!m_Flickering) {
            //simply clear the port buffer in case it has junk left from a previous cmd
            m_CommPort.readExisting();
            readString = "";
            return KleinsErrorCodes::TIMED_OUT;
        } else {
            unsigned int error = KleinsErrorCodes::NONE;
            error |= readFromSerialPort(m_CommPort, expected, timeOut_Sec, readString);
            if(error & KleinsErrorCodes::LOST_CONNECTION) {
                closePort();
            }
            return error;
        }
    } else {
        closePort();
        return KleinsErrorCodes::LOST_CONNECTION;
    }
}

unsigned int KClmtr::sendMessageToSerialPort(SerialPort &comPort, const command &m, string &readString) {
    return sendMessageToSerialPort(comPort, m.commandString, m.expected, m.timeout, readString);
}
unsigned int KClmtr::sendMessageToSerialPort(SerialPort &comPort, const string &strMsg, int expected, int timeOut_Sec, string &readString) {
    unsigned int error = KleinsErrorCodes::NONE;
    try {
        string commandString = string(strMsg);
        readString = "";
        //If it is open then we can move one
        if(comPort.isOpen()) {
            //Remove all bits out of the K10/8
            //m_CommPort.DiscardOutBuffer();
            //simply clear the port buffer in case it has junk left from a previous cmd
            comPort.readExisting();
            //Adds /r to the end of the command
            commandString.append(1, '\r');
            const unsigned char *myRead = reinterpret_cast<const unsigned char *>(commandString.c_str());
            //Send the command to the K10/8
            comPort.writePort(myRead, commandString.length());
            if(expected > 0) {
                error |= readFromSerialPort(comPort, expected, timeOut_Sec, readString);
            }
            return error;
        } else {
            //If not then we need to let the user know that it should be open
            error |= KleinsErrorCodes::NOT_OPEN;
            return error;
        }
    } catch(...) {
        error |= KleinsErrorCodes::LOST_CONNECTION;
        return error;
    }
}
unsigned int KClmtr::readFromSerialPort(SerialPort &comPort, int expected, long timeOut_Sec, string &readString) {
    try {
        int i = 0;
        //Makes sure if it Times out it will let the user know
        //If it is open then we can move one
        if(comPort.isOpen()) {
            readString += comPort.readExisting();
            //20 means 1 second
            //1.5 is added to make sure there isn't a range change
            //interrupting our commucation.
            timeOut_Sec *= 30;//20 * 1.5

            i = 0;
            //Goes until Times Out
            while(comPort.isOpen() && i < timeOut_Sec) {
                //Makes the PC sleeps so we can check the K10/8
                sleep(50);
                //Stores what is on the K10/8
                readString += comPort.readExisting();

                //Checks if we got all the info
                if((int)readString.length() < expected) {
                    ++i;
                } else {
                    //If we got all the info we will do something with the ReadString
                    return KleinsErrorCodes::NONE;
                }
            }
        } else {
            //If not then we need to let the user know that it should be open
            return KleinsErrorCodes::LOST_CONNECTION;
        }
        //If we timed out
        if(!comPort.isOpen()) {
            return KleinsErrorCodes::LOST_CONNECTION;
        }
        return KleinsErrorCodes::TIMED_OUT;
    } catch(...) {
        return KleinsErrorCodes::LOST_CONNECTION;
    }
}

bool KClmtr::setSerialNumberValues(const string &read, string &model, string &SN) {
    if(read.length() < 9) {
        model = "";
        SN = "";
    } else {
        model = trim(read.substr(2, 7));
        SN = trim(read.substr(9, 9));
        if(model.find("K-") == 0 ||
                model.find("KV-") == 0) {
            return true;
        }
    }
    return false;
}
bool KClmtr::getModelSN(SerialPort &comPort, string &model, string &SN) {
    string returnString;
    if(sendMessageToSerialPort(comPort, DEVICE_INFO, returnString) == (int)KleinsErrorCodes::NONE) {
        if(returnString.substr(0, 5) == "//////") {
            //Clearning the K10
            sendMessageToSerialPort(comPort, "P0", 131, 1, returnString);
            returnString = "";
            if(sendMessageToSerialPort(comPort, DEVICE_INFO, returnString) == (int)KleinsErrorCodes::NONE) {
                //Getting the Model and SerialNumber
                return setSerialNumberValues(returnString, model, SN);
            }
        } else {
            return setSerialNumberValues(returnString, model, SN);
        }
    }
    return false;
}

bool KClmtr::connect(const string &portName) {
    setPort(portName);
    return connect();
}
bool KClmtr::connect() {
    try {
        if(m_CommPort.openPort()) {
            m_CommPort.setSetting(9600, 8, 'n', 10000);
            m_isOpen = true;
        } else {
            m_isOpen = false;
        }

        if(!m_CommPort.isOpen()) {
            return false;
        }
        //Making sure it's a Klein product
        if(getModelSN(m_CommPort, m_Model, m_SerialNumber)) {
            //Check and see if its a K 8 or a 10 or not
            string returnString = "";
            if(sendMessageToKColorimeter(CALFILE_FILELIST, returnString) == 0) {
                //Get all the CalFiles on the K10/8
                setCalFileList(returnString);
                setCalFileID(getCalFileID());
                setRange(-1);
                setMaxAverageCount(32);
                return true;
            }
        }
        closePort();
        return false;
    } catch(...) {
        return false;
    }
}
bool KClmtr::testConnection(const string &portName, string &model, string &SN) {
    try {
        SerialPort CommPort;
        CommPort.portName = portName;
        if(CommPort.openPort()) {
            CommPort.setSetting(9600, 8, 'n', 10000);
        } else {
            return false;
        }

        //Making sure it's a Klein product
        bool r = getModelSN(CommPort, model, SN);
        CommPort.closePort();
        return r;
    } catch(...) {
        return false;
    }
}


void KClmtr::closePort() {
    if(isMeasuring()) {
        stopMeasuring();
    }
    if(isFlickering()) {
        stopFlicker();
    } else {
        resetFlicker();
    }

    try {
        m_CommPort.closePort();
    } catch(...) { /*ERROR*/  }
    for(int i = 1; i < 99; ++i) {
        m_CalFileList[i] = "";
    }

    //m_constantMeasuringM6 = false;
    m_range = -1;
    m_isOpen = false;
    m_CalFileName = "";
    m_CalFileID = 0;
    m_SerialNumber = "";
    if(m_Model.size() < 6 || m_Model.substr(m_Model.size() - 6, 6) != "CLOSED") {
        m_Model += "CLOSED";
    }
    for(int i = 0; i <= 70; ++i) {
        m_FFTCalRangeCoeff1[0][i] = -1;
        m_FFTCalRangeCoeff1[1][i] = -1;
        m_FFTCalRangeCoeff1[2][i] = -1;
    }
    for(int i = 0; i < polyDegree; ++i) {
        m_FFTCalRangeCoeff2[0][i] = -1;
        m_FFTCalRangeCoeff2[1][i] = -1;
        m_FFTCalRangeCoeff2[2][i] = -1;
    }
    m_Calindex = 0;
    _gs = GamutSpec::fromCode(GamutCode::defaultGamut);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            m_CalMatrix.v[i][j] = 0;
        }
    }
    if(m_ParsedOutRippleArray != NULL) {
        delete[] m_ParsedOutRippleArray;
        m_ParsedOutRippleArray = NULL;
    }
}
void KClmtr::startMeasureCounts() {
    stopFlicker();
    stopMeasuring();
    startMeasureCounts2();
    m_MeasuringM6 = true;
}
void KClmtr::stopMeasureCounts() {
    stopMeasureCounts2();
    m_MeasuringM6 = false;
}
bool KClmtr::isMeasureCounts() const {
    return m_MeasuringM6;
}
bool KClmtr::getMeasureCounts(Counts &c) {
    c = m_counts;
    bool temp = m_isCountsfresh;
    m_isCountsfresh = false;
    return temp;
}
Counts KClmtr::getNextMeasureCount() {
    //Clearing Buffer
    unsigned int error = sendMessageToKColorimeter(COUNTS_4PERSECOND);
    if(error != KleinsErrorCodes::NONE) {
        return Counts(error);
    }
    //Getting Measurement
    string returnString;
    error = sendMessageToKColorimeter(COUNTS_4PERSECOND, returnString);
    if(error != KleinsErrorCodes::NONE) {
        return Counts(error);
    }

    return Counts(returnString);
}
