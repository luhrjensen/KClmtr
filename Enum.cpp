#include "Enums.h"

using namespace KClmtrBase;

std::string KClmtrBase::intToString(int value) {
    if(value == 0) {
        return "0";
    }
    std::string temp;
    while(value > 0) {
        temp += value % 10 + 48;
        value /= 10;
    }
    std::string returnvalue;
    for(int i = 0; i < (int)temp.length(); ++i) {
        returnvalue += temp[temp.length() - i - 1];
    }
    return returnvalue;
}

void KClmtrBase::parsingRange(unsigned char range, int(&out)[3]) {
    for(int i = 0; i < 3; ++i) {
        out[i] = ((1 << (7 - i)) & range) ? 1 : 0;
    }
    range &= 0x1f;
    for(int i = 2; i >= 0; --i) {
        out[i] += 2 * (range % 3);
        range /= 3;
    }
    //increment to be 1-6 not 0-5
    for(int i = 0; i < 3; ++i) {
        ++out[i];
    }
}

#ifdef __cplusplus_cli
    System::String^ KleinsErrorCodes::errorCodesToString(unsigned int errorcode) {
        return errorCodesToString(errorcode, ingoreWarningList);
    }
    System::String^ KleinsErrorCodes::errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode) {
#elif QT_CORE_LIB
    QString KleinsErrorCodes::errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode) {
#elif NSAppKitVersionNumber10_0
    NSString *KleinsErrorCodes::errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode) {
#else
    std::string KleinsErrorCodes::errorCodesToString(unsigned int errorcode, unsigned int ingoreErrorcode) {
#endif
        //Removing errorcodes to be ingored
        errorcode &= ~ingoreErrorcode;

        std::string s = "";
        //There was no error
        if(errorcode != 0) {
            s = "ErrorCode " + intToString(errorcode) + " Reads:\n";
        //Serial Port
        if(errorcode & KleinsErrorCodes::NOT_OPEN) {
            s += "Error: The Port isn't open.";
        }
        if(errorcode & KleinsErrorCodes::TIMED_OUT) {
            s += "Error: Did not receive the info from port.\nFix: Try again, or reconnect the device.\n";
        }
        if(errorcode & KleinsErrorCodes::LOST_CONNECTION) {
            s += "Error: The Device was unplugged during commucation.\nFix: Please reconnect the device.\n";
        }

        //Measurement
        if(errorcode & KleinsErrorCodes::BAD_VALUES) {
            s += "Error: Values couldn't be retrived from the device.\n";
        }
        if(errorcode & KleinsErrorCodes::CONVERTED_NM) {
            s += "Event: Couldn't convert XYZ to NM.\n";
        }
        if(errorcode & KleinsErrorCodes::KELVINS) {
            s += "Event: Couldn't convert XYZ to Kelvins.\n";
        }
        if(errorcode & KleinsErrorCodes::AIMING_LIGHTS) {
            s += "Event: The Aiming lights are on.\nFix: Please turn off the aimming lights with the Knob\n";
        }
        if(errorcode & KleinsErrorCodes::AVERAGING_LOW_LIGHT) {
            s += "Event: The Device needs more time to average because the light level is low.\n";
        }

        //Ranges - Can Ingore
        if(errorcode & (KleinsErrorCodes::BOTTOM_UNDER_RANGE |
                        KleinsErrorCodes::TOP_OVER_RANGE |
                        KleinsErrorCodes::OVER_HIGH_RANGE)) {
            s += "Event: The device switched ranges.\n";
        }

        //Black Cals
        if(errorcode & (KleinsErrorCodes::BLACK_ZERO |
                        KleinsErrorCodes::BLACK_OVERDRIVE |
                        KleinsErrorCodes::BLACK_EXCESSIVE |
                        KleinsErrorCodes::BLACK_PARSING_ROM)) {
            s += "Event: Creating Black cal into the device failed\nFix: Rerun the BlackCal\n";
        }
        if(errorcode & KleinsErrorCodes::BLACK_STORING_ROM) {
            s += "Event: Storing the Black cal failed\nFix: Rerun the BlackCal\n";
        }

        //CalFiles
        if(errorcode & KleinsErrorCodes::CAL_WHITE_RGB) {
            s += "Event: There was a problem with converting the white spec and RGB\nFix: Rerun Calibration\n";
        }
        if(errorcode & KleinsErrorCodes::CAL_STORING) {
            s += "Event: Storing the CalFile failed\nFix: Re-Store calibration\n";
        }
        if(errorcode & KleinsErrorCodes::CAL_CONVERT_BINARY) {
            s += "Event: Couldn't convert the numbers to a string to be stored\nFix: Rerun calibration\n";
        }

        //FFT
        if(errorcode & KleinsErrorCodes::FFT_BAD_STRING) {
            s += "Event: FFT received a bad string\nFix: Re-Start Flicker\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_RANGE_CAL) {
            s += "Event: FFT received a bad rang cal file\nFix: Contact Klein Instruments\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_NO_XYZ) {
            s += "Event: There was no measurement in the FFT string\nFix: Re-start Flicker\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_NO_RANGE) {
            s += "Event: There was no range in FFT\nFix: Re-start Flicker\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_INSUFFICIENT_DATA) {
            s += "Event: There is not enough data to display FFT\nFix: Continue to gather Flicker data\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_PREVIOUS_RANGE) {
            s += "Event: The device is switching ranges\nFix: Ingore this Flicker Data, and continue gather Flicker Data\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_NOT_SUPPORTED) {
            s += "Event: The device that you are currently hooked up to cannot do FFT\nFix: Can't use this device on Flicker\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_BAD_SAMPLES) {
            s += "Event: FFT received a bad string\nFix: Re-Start Flicker\n";
        }
        if(errorcode & KleinsErrorCodes::FFT_OVER_SATURATED) {
            s += "Event: The counts are too high for the current range to get good data\nFix: Skip this Flicker measurement, or change ranges if you have locked the range\n";
        }

        //Miscellaneous
        if(errorcode & KleinsErrorCodes::FIRMWARE) {
            s += "Error: The Firmware could be corrupted. Please contact Klein Instruments.\n";
        }
        if(errorcode & KleinsErrorCodes::NEGATIVE_VALUES) {
            s += "Event: Measurement received had negative values.\nFix: Run Black Calibration\n";
        }

        //Removing the last \n
        s = s.substr(0, s.size()-1);
        }
#ifdef __cplusplus_cli
    return gcnew System::String(s.c_str());
#elif QT_CORE_LIB
    return QString::fromStdString(s.c_str());
#elif NSAppKitVersionNumber10_0
    return [NSString stringWithUTF8String:s.c_str()];
#else
    return s;
#endif
}
/**
* @brief To help to know when to stop the measurement cycle, or keep going
*/
#ifdef __cplusplus_cli
bool KleinsErrorCodes::shouldStopMeasuring(unsigned int errorcode) {
    return shouldStopMeasuring(errorcode, ingoreWarningList);
}
bool KleinsErrorCodes::shouldStopMeasuring(unsigned int errorcode, unsigned int ingoreErrorcode) {
#else
bool KleinsErrorCodes::shouldStopMeasuring(unsigned int errorcode, unsigned int ingoreErrorcode) {
#endif
    return (errorcode & ~ingoreErrorcode) > 0;
}

unsigned int KleinsErrorCodes::getErrorCodeFromKlein(std::string MyError) {
    if(MyError == "L") {
        return KleinsErrorCodes::AIMING_LIGHTS;
    } else if(MyError == "u") {
         return KleinsErrorCodes::BOTTOM_UNDER_RANGE;
    } else if(MyError == "v") {
         return KleinsErrorCodes::TOP_OVER_RANGE;
    } else if(MyError == "w") {
         return KleinsErrorCodes::OVER_HIGH_RANGE;
    } else if(MyError == "t") {
         return KleinsErrorCodes::BLACK_ZERO;
    } else if(MyError == "s") {
         return KleinsErrorCodes::BLACK_OVERDRIVE;
    } else if(MyError == "b") {
         return KleinsErrorCodes::BLACK_EXCESSIVE;
    } else if(MyError == "X") {
         return KleinsErrorCodes::FIRMWARE;
    } else if(MyError == "B") {
         return KleinsErrorCodes::FIRMWARE;
    } else {
        //0
        return KleinsErrorCodes::NONE;
    }
}
