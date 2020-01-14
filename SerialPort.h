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
#ifdef WIN32
#include <windows.h>
#else
#endif

#include <string>

namespace KClmtrBase {
namespace KClmtrNative {
/**
 * @brief Object to control the serial port in Linux, Mac, and Windows
 *
 */
class SerialPort {
public:
    /**
     * @brief constructor
     */
    SerialPort();
    virtual ~SerialPort();
    /**
     * @brief Open the port after portName is set
     */
    bool openPort();
    /**
     * @brief Closes the port
     */
    bool closePort();
    /**
     * @brief Settings need to be set after opening the portName
     * @param speed 	The baud rate of the device
     * @param wordSize 	The size of a char
     * @param parity
     * @param timeOut	The amount of time it needs to wait in windows before stoping
     */
    bool setSetting(int speed, int wordSize, char parity, int timeOut);
    /**
     * @brief Read from the port
     * @param buf The string to store it in
     * @param bufSize the max size of buf
     * @return the size of string that was returned in buf
     */
    int readPort(unsigned char *buf, int bufSize);
    /**
     * @brief Write to the port
     * @param buf The string to write
     * @param bufSize the max size of buf
     * @return The size of string that was written
     */
    int writePort(const unsigned char *buf, size_t bufSize);
    /**
     * @brief To find all the bites in the buffer and return it in a string
     * @return String in the buffer
     */
    const std::string readExisting();

    /**
     * @brief To read and set the port location
     */
    std::string portName;
    /**
     * @brief To see if the port is open or not
     * return Is the port Open? true = yes
     */
    bool isOpen();

    void setDataTerminalReady(bool value);
    void setRequestToSend(bool value);

private:
#ifdef WIN32
    HANDLE m_fileHandle;
#else
    int fd; //lock file
    int m_fileHandle;
    std::string lockedFile;
    bool lockFile();
    void unLockFile();
#endif
};
}
}
