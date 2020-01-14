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

#include "Flicker.h"
#include <cmath>
#include <cstdlib>

#define PI 3.141592653589793238

#ifndef uint
typedef unsigned int uint;
#endif

using namespace KClmtrBase;
using namespace KClmtrBase::KClmtrNative;

//TODO: get better values
// perhaps base on (0,10,20,30,40,50,60)  (1,1,1,.708,.501,.251,.01)
static const double EIAJ_array[] =  {
    1.000000, //0
    1.000000, //1
    1.000000, //2
    1.000000, //3
    1.000000, //4
    1.000000, //5
    1.000000, //6
    1.000000, //7
    1.000000, //8
    1.000000, //9
    1.000000, //10
    1.000000, //11
    1.000000, //12
    1.000000, //13
    1.000000, //14
    1.000000, //15
    1.000000, //16
    1.000000, //17
    1.000000, //18
    1.000000, //19
    1.000000, //20
    0.970795, //21
    0.941589, //22
    0.912384, //23
    0.883178, //24
    0.853973, //25
    0.824768, //26
    0.795562, //27
    0.766357, //28
    0.737151, //29
    0.707946, //30
    0.687270, //31
    0.666594, //32
    0.645918, //33
    0.625242, //34
    0.604567, //35
    0.583891, //36
    0.563215, //37
    0.542539, //38
    0.521863, //39
    0.501187, //40
    0.476517, //41
    0.451847, //42
    0.427178, //43
    0.402508, //44
    0.377838, //45
    0.353168, //46
    0.328498, //47
    0.303829, //48
    0.279159, //49
    0.251189, //50
    0.227070, //51
    0.202951, //52
    0.178832, //53
    0.154713, //54
    0.130595, //55
    0.106476, //56
    0.082357, //57
    0.058238, //58
    0.034119, //59
    0.010000, //60
    0.005158, //61
    0.000316, //62
    0.000000, //63
};
static double getNumberBetween(double high, double low, double number) {
    return (high - low) * (number - (int)number) + low;
}

FlickerSetting::FlickerSetting() {
    samples 				= 256;
    speed 					= 256;
    numberOfPeaks			= 3;
    cosine 					= true;
    smoothing 				= true;
    JETIADiscount_DB 		= true;
    JEITADiscount_Percent 	= false;
    decibel 				= DecibelMode::VESA;
    percent 				= PercentMode::ContrastMethod;
    corrections				= std::vector<RangeCorrecionArray>();
}
FlickerSetting::FlickerSetting(const FlickerSetting &other) {
    copy(other);
}
FlickerSetting::~FlickerSetting() {
    deleteCorrection();
}
double FlickerSetting::getResolution() const {
    return (double)speed / (double)samples;
}
double FlickerSetting::getCorrection(double hz) const {
    for(uint i = 0; i < corrections.size(); ++i) {
        //Checking the range
        if(corrections[i].rangeOfHz.start <= hz &&
                corrections[i].rangeOfHz.end > hz) {
            double c = 0;
            switch(corrections[i].mode) {
                case CorrectionMode::ArrayFit:
                    c = getNumberBetween(corrections[i].array[(int)hz], corrections[i].array[(int)hz + 1], hz);
                    break;
                case CorrectionMode::PolyFit: {
                    for(uint j = 0; j < corrections[i].array.size(); ++j) {
                        c += corrections[i].array[j] * pow(hz, j);
                    }
                    break;
                }
                default:
                    //ERROR
                    break;
            }
            return c;
        }
    }

    return 1.0;
}
//Array must have the same number as the Range
bool FlickerSetting::appendCorrection(const Range &rangeOfHz, const CorrectionMode &mode, const double *array, int size) {
    std::vector<double> data(array, array + size);
    return appendCorrection(rangeOfHz, mode, data);
}

bool FlickerSetting::appendCorrection(const Range &rangeOfHz, const CorrectionMode &mode, const std::vector<double> &array) {
    //TODO check if there is a good array
    corrections.resize(corrections.size() + 1);
    corrections.back().rangeOfHz = rangeOfHz;
    corrections.back().mode = mode;
    corrections.back().array = array;
    return true;
}
void FlickerSetting::deleteCorrection() {
    corrections.resize(0);
}
void FlickerSetting::copy(const FlickerSetting &other) {
    samples = other.samples;
    speed = other.speed;
    cosine = other.cosine;
    smoothing = other.smoothing;
    JETIADiscount_DB = other.JETIADiscount_DB;
    JEITADiscount_Percent = other.JEITADiscount_Percent;
    decibel = other.decibel;
    percent = other.percent;
    corrections = other.corrections;
}
double Flicker::getBigY() const {
    return bigY;
}
MeasurementRange Flicker::getRange() const {
    return range;
}
const Matrix<double> &Flicker::getPeakFrequencyPercent() const {
    return peakfrequencyPercent;
}
const Matrix<double> &Flicker::getPeakFrequencyDB() const {
    return peakfrequencyDB;
}
const Matrix<double> &Flicker::getFlickerDB() const {
    return flickerDB;
}
const Matrix<double> &Flicker::getFlickerPercent() const {
    return flickerPercent;
}
const Matrix<double> &Flicker::getCounts() const {
    return counts;
}
const Matrix<double> &Flicker::getNits() const {
    return nits;
}
const Matrix<double> &Flicker::getAmplitude() const {
    return amplitude;
}
unsigned int Flicker::getErrorCode() const {
    return errorcode;
}
const FlickerSetting &Flicker::getSettings() const {
    return settings;
}
double Flicker::getFlickerIndex() const {
    return flickerIndex;
}

Flicker::Flicker() {
    bigY = 0;
    flickerIndex = 0;
    range = MeasurementRange::range1;
    errorcode = 0;
    peakfrequencyPercent.initializeV(0, 0);
    peakfrequencyDB.initializeV(0, 0);
    flickerPercent.initializeV(0, 0);
    flickerDB.initializeV(0, 0);
    flickerPercent.initializeV(0, 0);
    amplitude.initializeV(0, 0);
    counts.initializeV(0, 0);
    nits.initializeV(0, 0);
    settings = FlickerSetting();
}
Flicker::Flicker(unsigned int error) {
    bigY = 0;
    flickerIndex = 0;
    range = MeasurementRange::range1;
    errorcode = 0;
    peakfrequencyPercent.initializeV(0, 0);
    peakfrequencyDB.initializeV(0, 0);
    flickerPercent.initializeV(0, 0);
    flickerDB.initializeV(0, 0);
    flickerPercent.initializeV(0, 0);
    amplitude.initializeV(0, 0);
    counts.initializeV(0, 0);
    nits.initializeV(0, 0);
    settings = FlickerSetting();
    errorcode = error;
}

Flicker::Flicker(const Flicker &f) {
    bigY = f.bigY;
    flickerIndex = f.flickerIndex;
    range = f.range;
    errorcode = f.errorcode;
    peakfrequencyPercent = f.peakfrequencyPercent;
    peakfrequencyDB = f.peakfrequencyDB;
    flickerPercent = f.flickerPercent;
    flickerDB = f.flickerDB;
    flickerPercent = f.flickerPercent;
    amplitude = f.amplitude;
    counts = f.counts;
    nits = f.nits;
    settings = f.settings;
}
Flicker::Flicker(const FlickerSetting &_settings, const double data[], int sectionOfFFT, double nitsLast32Samples) {
    settings = _settings;

    int numberInArray = (settings.samples / 2) - (28 * settings.samples / 256)  + 1;   //Example: 256 samples = 128, but will only use 100hz, we dropping 28 Hzs, +1 for DC at 0
    amplitude.initializeV(numberInArray, 2);
    flickerDB.initializeV(numberInArray, 2);
    flickerPercent.initializeV(numberInArray, 2);

    //Storing the counts over Time
    counts.initializeV(settings.samples, 2);
    nits.initializeV(settings.samples, 2);
    double countsAvg = 0;

    //Storing FFT Data
    double *fftData = new double[settings.samples * 2 + 1];
    fftData[0] = 0;
    double timeBefore = (((sectionOfFFT * 32) - settings.samples) / (double)settings.speed);
    double timePerUnit = 1 / (double)settings.speed;
    for(int i = 0; i < settings.samples; ++i) {
        counts.v[i][0] = nits.v[i][0] = i * timePerUnit + timeBefore; //Time in Seconds from the start
        counts.v[i][1] = fftData[2 * i + 1] = data[i];
        fftData[2 * i + 2] = 0;

        //Adding Cosine Correction
        if(settings.cosine) {
            fftData[2 * i + 1] *= 1 + cos(2 * PI * ((double)i / ((double)settings.samples - 1)) + PI);
        }

        //Summing the last 32 samples for avg later
        if(i > settings.samples - 33) {
            countsAvg += counts.v[i][1];
        }
    }

    //Storing the nits over Time
    countsAvg /= 32;
    double muiltyP = 0;
    double flickerIndexArea1 = 0;
    double flickerIndexArea2 = 0;
    if(nitsLast32Samples != -1 ||
            nitsLast32Samples == 0) {
        muiltyP = nitsLast32Samples / countsAvg;
        for(int i = 0; i < settings.samples; ++i) {
            //Nits
            nits.v[i][1] = counts.v[i][1] * muiltyP;

            //Flicker Index with counts
            if(counts.v[i][1] > countsAvg) {
                flickerIndexArea1 += counts.v[i][1] - countsAvg ;
                flickerIndexArea2 += countsAvg;
            } else {
                flickerIndexArea2 += counts.v[i][1];
            }
        }
    }
    //Calucating FlickerIndex
    flickerIndex  = flickerIndexArea1 / (flickerIndexArea1 + flickerIndexArea2);

    //Getting FFT Data
    four1(fftData, settings.samples, 1);

    //Storing Amps
    //For Smoothing
    double MySqr1;
    double MySqr2;
    double MySqr3;
    double MyOldVal;
    MySqr1 = 0;
    MySqr2 = 0;
    MySqr3 = 0;
    for(int i = 0; i < settings.samples / 2; ++i) {
        double real = fftData[2 * i + 1];
        double img =  fftData[2 * i + 2];
        double amps = sqrt(pow(real, 2) + pow(img, 2));
        //double phase = atan2(img, real);

        //Applying correction
        amps *= settings.getCorrection(i * settings.getResolution());
        //Storing in the amplitude matrix
        if(i < numberInArray) {
            amplitude.v[i][1] = amps;
        }

        //Smoothing out the curve
        //only 3 in, and one before the end
        if(settings.smoothing && i >= 4 && i < numberInArray) {
            //value of previous
            MyOldVal = sqrt(MySqr1 + MySqr2 + MySqr3);
            //sqrrt(sum of squares) works well
            MySqr1 = pow(amplitude.v[i - 2][1], 2);
            //simple average does not work at all
            MySqr2 = pow(amplitude.v[i - 1][1], 2);
            MySqr3 = pow(amplitude.v[i][1], 2);
            if(i > 4) {
                amplitude.v[i - 2][1] = MyOldVal;
            }
         }


        //To Reverse
        //After Correction revert the FFT to get a better Singal
        //real
        //fftData[2 * i + 1] = amps * cos(phase);
        //imagary
        //fftData[2 * i + 2] = amps * sin(phase);
    }
    //Reviersing
    //four1(fftData, settings.samples, -1);
    //Undoing cosine
    //for(int i = 0; i < settings.samples; ++i) {
        //Storing the singal into counts
        //counts.v[i][1] = fftData[2 * i + 1] / settings.samples;
        //counts.v[i][1] /= pow(sin(PI * ((double)i / ((double)settings.samples - 1))), 2);
    //}
    delete[] fftData;

    //DC level
    double DCLevel = amplitude.v[0][1];
    //Normalize for 0hz
    amplitude.v[0][0] = flickerDB.v[0][0] = flickerPercent.v[0][0] = 0; //Hz
    amplitude.v[0][1] = 1; // DC should be 1, for it's normalzed now
    flickerPercent.v[0][1] = 100.0;
    flickerDB.v[0][1] = 0;
    //Normalized for 1hz
    amplitude.v[1][1] = amplitude.v[2][1] ;

    //storing
    for(int i = 1; i < numberInArray; ++i) {
        //Hz
        double hz = flickerPercent.v[i][0] = i * settings.getResolution();
        amplitude.v[i][0] = flickerDB.v[i][0] = hz;
        //Normalizing Amps
        amplitude.v[i][1] = amplitude.v[i][1] / DCLevel;

        //Adding fudge Factor
        if(settings.cosine && settings.smoothing) {
            amplitude.v[i][1] *= 0.819;
        } else { // Smoothing, Cosine or Nothing on
            amplitude.v[i][1] *= 1.046;
        }

        //Getting Percent
        flickerPercent.v[i][1] = amplitude.v[i][1] * 100;
        if(settings.percent == PercentMode::ContrastMethod) {
            flickerPercent.v[i][1] *= 4;
        }
        //Applying JEITA to Percent
        if(settings.JEITADiscount_Percent) {
            if(hz < 62) {
                flickerPercent.v[i][1] *= getNumberBetween(EIAJ_array[(int)hz], EIAJ_array[(int)hz + 1], hz);
            } else {
                flickerPercent.v[i][1] = 0;
            }
        }

        //Getting DB
        double weighted = 1;
        if(settings.JETIADiscount_DB && hz >= 62) {
            flickerDB.v[i][1] = -100;
        } else {
            if(settings.JETIADiscount_DB) {
                weighted = getNumberBetween(EIAJ_array[(int)hz], EIAJ_array[(int)hz + 1], hz);
            }
            flickerDB.v[i][1] = 20 * log10(sqrt(2) * weighted * amplitude.v[i][1]);
            //So VESA Method
            if(settings.decibel == DecibelMode::VESA) {
                flickerDB.v[i][1] += 3.01;
            }
        }
    }
    //Peaks
    peakfrequencyPercent.initializeV(settings.numberOfPeaks, 2);
    peakfrequencyDB.initializeV(settings.numberOfPeaks, 2);
    for(int i = 0; i < settings.numberOfPeaks; ++i) {
        peakfrequencyPercent.v[i][0] = 0;       //Hz
        peakfrequencyPercent.v[i][1] = 0;       //Percent

        peakfrequencyDB.v[i][0] = 0;       //Hz
        peakfrequencyDB.v[i][1] = -100;    //db
    }

    for(int x = 0; x < 2; ++x) {
        Matrix<double> *checkData;
        Matrix<double> *checkPeaks;
        if(x == 0) {
            checkData = &flickerPercent;
            checkPeaks = &peakfrequencyPercent;
        } else {
            checkData = &flickerDB;
            checkPeaks = &peakfrequencyDB;
        }

        for(int i = 1; i < numberInArray; ++i) {
            //Is this a peak
            bool isPeak = false;
            if(i == numberInArray - 1) {
                if(checkData->v[i - 1][1] < checkData->v[i][1]) {
                    isPeak = true;
                }
            } else {
                if(checkData->v[i - 1][1] < checkData->v[i][1] &&
                        checkData->v[i + 1][1] < checkData->v[i][1]) {
                    isPeak = true;
                }
            }


            if(isPeak) {
                //Finding where to put the peak if it's greater than any others
                for(int storePoint = 0; storePoint < settings.numberOfPeaks; ++storePoint) {
                    if(checkPeaks->v[storePoint][1] < checkData->v[i][1]) {
                        //Found the peak
                        //putting it in the correct location, but also move everything down
                        for(int k = settings.numberOfPeaks - 1; k > storePoint; --k) {
                            checkPeaks->v[k][0] = checkPeaks->v[k - 1][0];
                            checkPeaks->v[k][1] = checkPeaks->v[k - 1][1];
                        }
                        //Storing the new point
                        checkPeaks->v[storePoint][0] = checkData->v[i][0];
                        checkPeaks->v[storePoint][1] = checkData->v[i][1];
                        break;
                    }
                }
            }
        }
    }
}
void Flicker::four1(double data[], int nn, int isign) {
    int n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = nn << 1;
    j = 1;
    for(i = 1; i < n; i += 2) {  /* This is the bit-reversal section of the routine. */
        if(j > i) {
            //Swapping j with i
            tempr = data[j];
            data[j] = data[i];
            data[i] = tempr;
            //Swapping j+1 with i+1
            tempr = data[j + 1];
            data[j + 1] = data[i + 1];
            data[i + 1] = tempr;
        }
        m = n >> 1;
        while(m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    mmax = 2;
    while(n > mmax) {                                               /* Outer loop executed log2 nn times. */
        istep = 2 * mmax;
        theta = 2 * PI / (isign * mmax);                            /* Initialize the trigonometric recurrence. */
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for(m = 1; m < mmax; m += 2) {                      /* Here are the two nested inner loops. */
            for(i = m; i <= n; i += istep) {
                j          = i + mmax;                      /* This is the Danielson-Lanczos formula. */
                tempr      = wr * data[j]   - wi * data[j + 1];
                tempi      = wr * data[j + 1] + wi * data[j];
                data[j]    = data[i]      - tempr;
                data[j + 1]  = data[i + 1]    - tempi;
                data[i]   += tempr;
                data[i + 1] += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;        /* Trigonometric recurrence. */
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
    }
}
