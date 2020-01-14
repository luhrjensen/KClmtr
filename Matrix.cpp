#include "Matrix.h"
#include <cmath>

using namespace KClmtrBase::KClmtrNative;

template <typename T>
Matrix<T>::Matrix() {
    row = 0;
    column = 0;
}
template <typename T>
Matrix<T>::Matrix(const Matrix<T> &other) {
    column = 0;
    row = 0;
    initializeV(other.row, other.column);

    for(unsigned int i = 0; i < this->row; ++i) {
        for(unsigned int j = 0; j < this->column; ++j) {
            v[i][j] = other.v[i][j];
        }
    }
}
template <typename T>
Matrix<T>::Matrix(unsigned int _row, unsigned int _column) {
    row = 0;
    column = 0;
    initializeV(_row, _column);
}
template <typename T>
Matrix<T>::~Matrix() {
    deleteV();
}

template <typename T>
bool Matrix<T>::operator==(const Matrix<T> &other) const {
    if(getRow()!= other.getRow() ||
            getColumn() != other.getColumn()) {
        return false;
    }
    for(unsigned int i = 0; i < getRow(); ++i) {
        for(unsigned int j = 0; j < getColumn(); ++j) {
            if(v[i][j] != other.v[i][j]) {
                return false;
            }
        }
    }
    return true;
}
template <typename T>
Matrix<T>& Matrix<T>::operator=(const Matrix<T> &other) {
    this->initializeV(other.row, other.column);

    for(unsigned int i = 0; i < getRow(); ++i) {
        for(unsigned int j = 0; j < getColumn(); ++j) {
            this->v[i][j] = other.v[i][j];
        }
    }

    return *this;
}
template <typename T>
Matrix<T> Matrix<T>::operator*(const Matrix<T> &other) const {
    //Checking if we can
    if(getColumn() != other.getRow()) {
        return (*this);
    }
    Matrix<T> mulMatrix(row, other.column);
    for(unsigned int i = 0; i < getRow(); ++i) {
        for(unsigned int j = 0; j < other.getColumn(); ++j) {
            T number = 0;
            for(unsigned int k = 0; k < getColumn(); ++k) {
                number += v[i][k] * other.v[k][j];
            }
            mulMatrix.v[i][j] = number;
        }
    }
    return mulMatrix;

}
template <typename T>
unsigned int Matrix<T>::getRow() const {
    return row;
}
template <typename T>
unsigned int Matrix<T>::getColumn() const {
    return column;
}
template <typename T>
void Matrix<T>::clear() {
    for(unsigned int i = 0; i < row; ++i) {
        for(unsigned int j = 0; j < column; ++j) {
            v[i][j] = 0;
        }
    }
}
template <typename T>
void Matrix<T>::initializeV(unsigned int _row, unsigned int _column) {
    deleteV();
    row = _row;
    column = _column;
    if(row > 0) {
        v = new T *[row];

        for(unsigned int i = 0; i < row; ++i) {
            v[i] = new T[column];
        }
    }

    clear();
}
template <typename T>
void Matrix<T>::deleteV() {
    if(row > 0) {
        for(unsigned int i = 0; i < row; ++i) {
            delete[] v[i];
        }

        delete[] v;
    }

    column = 0;
    row = 0;
}
template <typename T>
Matrix<T> Matrix<T>::transpose() const {
    Matrix<T> TM(getColumn(), getRow());
    for(unsigned int i = 0; i < getRow(); i++) {
        for(unsigned int j = 0; j < getColumn(); j++) {
            TM.v[j][i] = v[i][j];
        }
    }
    return TM;
}
template <typename T>
T Matrix<T>::dotProduct(const Matrix<T> &other) const {
    bool ok;
    T number = dotProduct(other, ok);
    if(ok) {
        return number;
    } else {
        return (T)0;
    }
}
template <typename T>
T Matrix<T>::dotProduct(const Matrix<T> &other, bool &OK) const {
    if(column != other.getColumn() &&
            row == other.getRow()) {
        OK = false;
        return (T)0;
    }

    OK = true;
    T dot = 0;
    for(unsigned int i = 0; i < getRow(); ++i) {
        for(unsigned int j = 0; j < getColumn(); ++j) {
            dot += v[i][j] * other.v[i][j];
        }
    }
    return dot;
}
template <typename T>
void Matrix<T>::swapRows(int row1, int row2) {
    for(unsigned int i = 0; i < getColumn(); i++) {
        T temp = v[row1][i];
        v[row1][i] = v[row2][i];
        v[row2][i] = temp;
    }
}
template <typename T>
bool Matrix<T>::LUDecomposition(Matrix<T> &L, Matrix<T> &U, Matrix<T> &P) const {
    //LU Decomposition only works with Square Matrixies
    if(getRow() != getColumn()) {
        return false;
    }
    T max;
    int N = getRow();
    L.initializeV(N, N);
    U.initializeV(N, N);
    Matrix<T> tempA = *this;

    int pivot;
    for(int i = 0; i < N - 1; ++i) {
        max = abs(tempA.v[i][i]);
        pivot = i;

        for(int j = i + 1; j < N; ++j) {
            if(abs(tempA.v[j][i]) > max) {
                max = abs(tempA.v[j][i]);
                pivot = j;
            }
        }

        if(pivot != i) {
            tempA.swapRows(i, pivot);
            P.swapRows(i, pivot);
        }

        if(tempA.v[i][i] != 0.0) {
            for(int j = i + 1; j < N; ++j) {
                tempA.v[j][i] = tempA.v[j][i] / tempA.v[i][i];
                for(int k = i + 1; k < N; ++k) {
                    tempA.v[j][k] -= tempA.v[j][i] * tempA.v[i][k];
                }
            }
        }
    }
    for(int i = 0; i < N; ++i) {
        L.v[i][i] = 1;
        for(int j = 0; j < N; ++j) {
            if(j < i) {
                L.v[i][j] = tempA.v[i][j];
            } else {
                U.v[i][j] = tempA.v[i][j];
            }
        }
    }
    return true;
}
template <typename T>
Matrix<T> Matrix<T>::LUSolve(const Matrix<T> &L, const Matrix<T> &U, const Matrix<T> &b) {
    Matrix<T> tempb = b;
    int N = U.getRow();
    /*y: = Ux, thus Ly = b*/
    /*solve for y by forward substitution*/
    Matrix<T> y(tempb.getRow(), 1);
    y.v[0][0] = tempb.v[0][0] / L.v[0][0];
    for(int i = 1; i < N; ++i) {
        y.v[i][0] = tempb.v[i][0] / L.v[i][i];

        for(int j = 0; j < i; ++j) {
            y.v[i][0] += -L.v[i][j] * y.v[j][0] / L.v[i][i];
        }
    }

    /*Ux = y*/
    /*Solve for x by backward substitution*/
    Matrix x(tempb.getRow(), 1);
    x.v[N - 1][0] = y.v[N - 1][0] / U.v[N - 1][N - 1];
    for(int i = N - 2; i >= 0; --i) {
        x.v[i][0] = y.v[i][0] / U.v[i][i];

        for(int j = i + 1; j < N; ++j) {
            x.v[i][0] += -U.v[i][j] * x.v[j][0] / U.v[i][i];
        }
    }
    return x;
}
template <typename T>
Matrix<T> Matrix<T>::Unity(unsigned int rowColumns) {
    Matrix<T> out(rowColumns, rowColumns);
    for(unsigned int i = 0; i < rowColumns; ++i) {
        for(unsigned int j = 0; j < rowColumns; ++j) {
            if(i == j) {
                out.v[i][j] = (T)1;
            } else {
                out.v[i][j] = (T)0;
            }
        }
    }
    return out;
}

template class KClmtrBase::KClmtrNative::Matrix<double>;
template class KClmtrBase::KClmtrNative::Matrix<int>;
template class KClmtrBase::KClmtrNative::Matrix<float>;
