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
#include <iostream>
#include <iomanip>

namespace KClmtrBase {
namespace KClmtrNative {
/**
 * @ingroup Structs Structures
 */
/**
 * @brief A 2D Matrix of any size
 *
 */
template<typename T>
class Matrix {
public:
    Matrix();
    Matrix(const Matrix<T> &other);
    Matrix(unsigned int _row, unsigned int _column);
    ~Matrix();
    void initializeV(unsigned int _row, unsigned int _column);

    T **v;
    unsigned int getRow() const;
    unsigned int getColumn() const;
    void clear();

    bool     operator  ==(const Matrix<T> &other) const;
    Matrix  &operator   =(const Matrix<T> &other);
    Matrix   operator   *(const Matrix<T> &other) const;

    Matrix transpose() const;
    T dotProduct(const Matrix<T> &other) const;
    T dotProduct(const Matrix<T> &other, bool &OK) const;
    void swapRows(int row1, int row2);


    bool LUDecomposition(Matrix<T> &L, Matrix<T> &U, Matrix<T> &P) const;
    static Matrix<T> LUSolve(const Matrix<T> &L, const Matrix<T> &U, const Matrix<T> &b);
    static Matrix<T> Unity(unsigned int rowColumns);
private:
    void deleteV();
    unsigned int row;
    unsigned int column;
};

template <class T>
inline std::ostream &operator<<(std::ostream& stream, const Matrix<T> &matrix) {
    for (unsigned int i = 0; i < matrix.getRow(); ++i) {
        for (unsigned int j = 0; j < matrix.getColumn(); ++j) {
            if (j != 0) {
                stream << '\t';
            }
            stream << std::fixed << std::setprecision(3) << matrix.v[i][j];
        }
        if (i < matrix.getRow() - 1) {
            std::endl(stream);
        }
    }
    return stream;
}

}
}
