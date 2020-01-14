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
#include "SerialPort.h"

#ifdef WIN32
#else
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <cstdio>
#include <cstring>
#endif

using namespace std;
using namespace KClmtrBase::KClmtrNative;

SerialPort::SerialPort(void) {
#ifdef WIN32
    m_fileHandle = NULL;
#else
    m_fileHandle = -1;
#endif
}
SerialPort::~SerialPort(void) {
    closePort();
}
bool SerialPort::isOpen() {
#ifdef WIN32
    DCB dcb;
    if(GetCommState(m_fileHandle, &dcb)) {
        return true;
    } else {
        return false;
    }
#else
    struct termios options;
    return tcgetattr(m_fileHandle, &options) == 0;
#endif
}

bool SerialPort::openPort() {
#ifdef WIN32
    //    string portname = "\\\\.\\" + portName;
    m_fileHandle = CreateFileA(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0,	NULL);
    if(m_fileHandle == INVALID_HANDLE_VALUE) {
        return false;
    } else {
        return true;
    }
#else
    m_fileHandle = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if(m_fileHandle == -1) {
        return false;
    } else {
        //Locking File
        if(lockFile()) {
            return true;
        } else {
            closePort();
            return false;
        }
        return true;
    }
#endif

}
bool SerialPort::closePort() {
#ifdef WIN32
    if(m_fileHandle != NULL && CloseHandle(m_fileHandle)) {
        //Make sure after CloseHandle the handle gets reset
        m_fileHandle = NULL;
        return true;
    }

    return false;
#else
    bool returnValue = close(m_fileHandle) == 0;
    unLockFile();
    m_fileHandle = -1;
    return returnValue;
#endif
}

bool SerialPort::setSetting(int speed, int wordSize, char parity, int timeOut) {
#ifdef WIN32
    DCB dcb;
    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);

    GetCommState(m_fileHandle, &dcb);


    COMMPROP prop;

    GetCommProperties(m_fileHandle, &prop);



    dcb.BaudRate = (DWORD)speed;
    dcb.ByteSize = (byte)wordSize;
    dcb.StopBits =  ONESTOPBIT;
    dcb.Parity = (byte)(parity == 'n' ? 0 : 1);

    if(!SetCommState(m_fileHandle, &dcb)) {
        return false;
    }

    if(!SetupComm(m_fileHandle, 1600, 1600)) {
        return false;
    }

    COMMTIMEOUTS cmt;
    cmt.ReadIntervalTimeout = timeOut * 1000;
    cmt.ReadTotalTimeoutMultiplier = timeOut * 1000;
    cmt.ReadTotalTimeoutConstant = timeOut * 1000;
    cmt.WriteTotalTimeoutConstant = timeOut * 1000;
    cmt.WriteTotalTimeoutMultiplier = timeOut * 1000;

    if(!SetCommTimeouts(m_fileHandle, &cmt)) {
        return false;
    }

    return true;
#else
    struct termios options;
    speed_t speedT;
    tcgetattr(m_fileHandle, &options);
    //cfmakeraw(&options);
    //options.c_lflag = 0;

    switch(speed) {
        case 0:
            speedT = B0;
            break;
        case 50:
            speedT = B50;
            break;
        case 75:
            speedT = B75;
            break;
        case 110:
            speedT = B110;
            break;
        case 200:
            speedT = B200;
            break;
        case 300:
            speedT = B300;
            break;
        case 600:
            speedT = B600;
            break;
        case 1200:
            speedT = B1200;
            break;
        case 1800:
            speedT = B1800;
            break;
        case 2400:
            speedT = B2400;
            break;
        case 4800:
            speedT = B4800;
            break;
        case 9600:
            speedT = B9600;
            break;
        case 19200:
            speedT = B19200;
            break;
        case 38400:
            speedT = B38400;
            break;
        default:
            speedT = B19200;
            break;
    }

    cfsetispeed(&options, speedT);
    cfsetospeed(&options, speedT);

    options.c_cflag &= ~(CSIZE);

    switch(wordSize) {
        case 5:
            options.c_cflag |= CS5;
            break;
        case 6:
            options.c_cflag |= CS6;
            break;
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
        default:
            options.c_cflag |= CS8;
            break;
    }

    if(parity == 'e') {
        options.c_cflag &= ~PARODD;
        options.c_cflag |= PARENB;
    } else if(parity == 'o') {
        options.c_cflag |= PARENB | PARODD;
    } else {
        options.c_cflag &= ~PARENB;
    }


    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = timeOut * 10;
    //options.c_cflag = (CLOCAL | CREAD);
    options.c_cflag &= ~CSTOPB; //1 stopbit
    //No Flowcontrol
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);


    if(tcsetattr(m_fileHandle, TCSANOW, &options) != 0) {
        return false;
    }
    return true;
#endif
}
int SerialPort::readPort(unsigned char *buf, int bufSize) {
#ifdef WIN32
    if(!isOpen() || m_fileHandle == NULL) {
        return(0);
    }

    DWORD numRead;

    BOOL ret = ReadFile(m_fileHandle, buf, bufSize, &numRead, NULL);

    if(!ret) {
        return 0;
    }

    buf[numRead] = '\0';

    return ret;

#else
    return read(m_fileHandle, buf, bufSize);
#endif
}
int SerialPort::writePort(const unsigned char *buf, size_t bufSize) {
#ifdef WIN32
    DWORD write = -1;
    WriteFile(m_fileHandle, buf, (DWORD)bufSize, &write, NULL);
    return write;
#else
    int w = write(m_fileHandle, buf, bufSize);
    return w;
#endif
}
const string SerialPort::readExisting() {
    int count = 1;
#ifdef WIN32
    COMSTAT status;
    DWORD errors;
    if(ClearCommError(m_fileHandle, &errors, &status)) {
        count = status.cbInQue;
    } else {
        count = 0;
    }

#else
    ioctl(m_fileHandle, FIONREAD, &count);
#endif

    unsigned char *bytes = new unsigned char[count + 1];
    for(int i = 0; i < count; ++i) {
        bytes[i] = '\0';
    }

    //bytes = { '\0' };

    readPort(bytes, count);

    string read = string((char *)bytes, count);
    delete[] bytes;
    return read;
}
#ifndef WIN32
void SerialPort::unLockFile() {
    if(lockedFile != "") {
        close(fd);
        unlink(lockedFile.c_str());
        lockedFile = "";
    }
}

bool SerialPort::lockFile() {
    static const char *lock_dir_list[5] = {
        "/var/lock/LCK..",
        "/etc/locks/LCK..",
        "/var/spool/locks/LCK..",
        "/var/spool/uucp/LCK..",
        "/tmp/LCK.."
    };
    for(int i = 0; i < 5; ++i) {
        lockedFile = lock_dir_list[i];
        string port = "";
        port += strrchr(portName.c_str(), '/');
        lockedFile += port.substr(1);
        fd = open(lockedFile.c_str(), O_RDWR | O_EXCL | O_CREAT, 0644);

        if(fd != -1) {
            // We made a lock file...
            char pid[10];
            int n = sprintf(pid, "%ld\n", (long)getpid());
            write(fd, pid, n);
            close(fd);
            return true;
        }
    }
    close(fd);
    return false;
}
#endif

void SerialPort::setDataTerminalReady(bool value) {
#ifdef WIN32
    DCB dcb;
    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);

    GetCommState(m_fileHandle, &dcb);

    dcb.fDtrControl = value;

    SetCommState(m_fileHandle, &dcb);
#else
    int status = TIOCM_DTR;
    ioctl(m_fileHandle, value ? TIOCMBIS : TIOCMBIC, &status);
#endif
}

void SerialPort::setRequestToSend(bool value) {
#ifdef WIN32
    DCB dcb;
    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);

    GetCommState(m_fileHandle, &dcb);

    dcb.fRtsControl = value;

    SetCommState(m_fileHandle, &dcb);
#else
    int status = TIOCM_RTS;
    ioctl(m_fileHandle, value ? TIOCMBIS : TIOCMBIC, &status);
#endif
}
