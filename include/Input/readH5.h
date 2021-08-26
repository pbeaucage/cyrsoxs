/////////////////////////////////////////////////////////////////////////////////
// MIT License
//
//Copyright (c) 2019 - 2020 Iowa State University
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.
//////////////////////////////////////////////////////////////////////////////////

#ifndef PRS_READH5_H
#define PRS_READH5_H

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <string>
#include <sstream>      // std::stringstream

#include <vector>
#include <assert.h>
#include "Input.h"
#include "H5Cpp.h"
#include <hdf5_hl.h>

namespace H5 {

  static constexpr int AXIS_LABEL_LEN = 2;


  template<typename T>
  static void XYZ_to_ZYX(std::vector<T> &data, const int numComponents, const UINT *voxelSize) {
    std::vector<T> _data = data;
    int counter = 0;
    UINT X = voxelSize[0];
    UINT Y = voxelSize[1];
    UINT Z = voxelSize[2];
    for (int i = 0; i < X; i++) {
      for (int j = 0; j < Y; j++) {
        for (int k = 0; k < Z; k++) {
          for (int c = 0; c < numComponents; c++) {
            _data[counter + numComponents * c] = data[
              (k * voxelSize[0] * voxelSize[1] + j * voxelSize[1] + i) * numComponents + c];
          }
          counter++;
        }
      }
    }
    std::swap(data, _data);
  }

/**
 *
 * @param [in] file HDF5 file pointer
 * @param [in] numMaterial  number of material
 * @param [in] voxelSize voxel size in 3D
 * @param [out] inputData inputData for Mat_alligned
 * Note : This is read after unaligned data. So, the morphology order and voxelSize is set according to alligned data.
 * Any mismatch will throw an error.
 */

  static inline void getMatAllignment(const H5::H5File &file,
                                      const UINT *voxelSize,
                                      std::vector<Real > &inputData,
                                      const MorphologyOrder &morphologyOrder,
                                      const int materialID,
                                      bool isRequired = true) {
    std::string groupName = "vector_morphology/";
    BigUINT numVoxel = static_cast<BigUINT>((BigUINT) voxelSize[0] * (BigUINT) voxelSize[1] * (BigUINT) voxelSize[2]);
    inputData.resize(numVoxel * 3);

    int i = materialID;
    {
      std::string varname = groupName + "Mat_" + std::to_string(i) + "_alignment";
      H5::DataSet dataSet;
      // Check if data set exists
      bool dataExists = dataSet.exists(varname);

      if (isRequired and not(dataExists)) {
        std::cerr << "Dataset = " << varname << "does not exists";
        exit(EXIT_FAILURE);
      }
      if (dataExists) {
        dataSet = file.openDataSet(varname);
        H5::DataType dataType = dataSet.getDataType();
        H5::DataSpace space = dataSet.getSpace();
        hsize_t voxelDims[4];
        const int ndims = space.getSimpleExtentDims(voxelDims, NULL);
        char label[2][AXIS_LABEL_LEN];
        H5DSget_label(dataSet.getId(), 0, label[0], AXIS_LABEL_LEN);
        H5DSget_label(dataSet.getId(), 2, label[1], AXIS_LABEL_LEN);

        // We store voxel dimension as (X,Y,Z) irrespective of HDF axis label
        if (morphologyOrder == MorphologyOrder::ZYX) {
          if (not((strcmp(label[0], "Z") == 0) and (strcmp(label[1], "X") == 0))) {
            throw std::logic_error("Axis label mismatch for morphology");
          }
          if ((ndims != 4) or (voxelDims[0] != voxelSize[2]) or (voxelDims[1] != voxelSize[1]) or
              (voxelDims[2] != voxelSize[0]) or (voxelDims[3] != 3)) {
            std::cout << "Error in morphology for Material = " << i << "\n";
            std::cout << "Expected dimension (X,Y,Z) = " << voxelSize[0] << " " << voxelSize[1] << " " << voxelSize[2]
                      << "\n";
            std::cout << "Dimensions from HDF5 (X,Y,Z)          = " << voxelDims[2] << " " << voxelDims[1] << " "
                      << voxelDims[0] << "\n";
            throw std::logic_error("Dimension mismatch for morphology");
          }
        }
        if (morphologyOrder == MorphologyOrder::XYZ) {
          if (not((strcmp(label[0], "X") == 0) and (strcmp(label[1], "Z") == 0))) {
            throw std::logic_error("Axis label mismatch for morphology");
          }
          if ((ndims != 4) or (voxelDims[0] != voxelSize[0]) or (voxelDims[1] != voxelSize[1]) or
              (voxelDims[2] != voxelSize[2]) or (voxelDims[3] != 3)) {
            std::cout << "Error in morphology for Material = " << i << "\n";
            std::cout << "Expected dimension (X,Y,Z)   = " << voxelSize[0] << " " << voxelSize[1] << " " << voxelSize[2]
                      << "\n";
            std::cout << "Dimensions from HDF5 (X,Y,Z) = " << voxelDims[0] << " " << voxelDims[1] << " " << voxelDims[2]
                      << "\n";
            throw std::logic_error("Dimension mismatch for morphology");
          }
        }
#ifdef DOUBLE_PRECISION
        if(dataType != PredType::NATIVE_DOUBLE){
           std::cout << "The data format is not supported for double precision \n";
           exit(EXIT_FAILURE);
        }
        dataSet.read(inputData[i - 1].data(), H5::PredType::NATIVE_DOUBLE);

#else

        if (dataType == PredType::NATIVE_DOUBLE) {
          std::vector<double> alignedData(numVoxel * 3);
          dataSet.read(alignedData.data(), H5::PredType::NATIVE_DOUBLE);
          if (morphologyOrder == MorphologyOrder::XYZ) {
            XYZ_to_ZYX(alignedData, 3, voxelSize);
          }
          for (int id = 0; id < numVoxel; id++) {
            inputData[id * 3 + 0] = static_cast<Real>(alignedData[3 * id + 0]);
            inputData[id * 3 + 1] = static_cast<Real>(alignedData[3 * id + 1]);
            inputData[id * 3 + 2] = static_cast<Real>(alignedData[3 * id + 2]);
          }
        } else if (dataType == PredType::NATIVE_FLOAT) {
          std::vector<float> alignedData(numVoxel * 3);
          dataSet.read(alignedData.data(), H5::PredType::NATIVE_FLOAT);
          if (morphologyOrder == MorphologyOrder::XYZ) {
            XYZ_to_ZYX(alignedData, 3, voxelSize);
          }
          for (int id = 0; id < numVoxel; id++) {
            inputData[id * 3 + 0] = static_cast<Real>(alignedData[3 * id + 0]);
            inputData[id * 3 + 1] = static_cast<Real>(alignedData[3 * id + 1]);
            inputData[id * 3 + 2] = static_cast<Real>(alignedData[3 * id + 2]);
          }
        }
        dataType.close();
        dataSet.close();
#endif
      }
    }
  }

/**
 *
 * @param [in] file HDF5 file pointer
 * @param [in] numMaterial  number of material
 * @param [in] voxelSize voxel size in 3D
 * @param [out] inputData inputData for Mat_unaligned
 */

  static inline void getScalar(const H5::H5File &file,
                               const std::string &groupName,
                               const std::string &strName,
                               UINT *voxelSize,
                               int  &morphologyOrder,
                               std::vector<Real> &inputData,
                               const int materialID,
                               const bool isRequired = true,
                               const bool assignLabelandDims = false ) {

    BigUINT numVoxel = static_cast<BigUINT>((BigUINT) voxelSize[0] * (BigUINT) voxelSize[1] * (BigUINT) voxelSize[2]);
    inputData.resize(numVoxel);


    int i = materialID; {
      std::string varname = groupName + "/Mat_" + std::to_string(i) + strName;

      H5::DataSet dataSet;
      bool dataExists = dataSet.exists(varname);

      // Check if dataset exists and required
      if (isRequired and not(dataExists)) {
        std::cerr << "Dataset = " << varname << "does not exists";
        exit(EXIT_FAILURE);
      }

      // Fill with 0 if not exists
      if(not(dataExists)) {
        std::fill(inputData.begin(),inputData.end(),0.0);
        return;
      }

      H5::DataType dataType = dataSet.getDataType();
      H5::DataSpace space = dataSet.getSpace();
      hsize_t voxelDims[3];
      const int ndims = space.getSimpleExtentDims(voxelDims, NULL);
      char label[2][AXIS_LABEL_LEN];
      H5DSget_label(dataSet.getId(), 0, label[0], AXIS_LABEL_LEN);
      H5DSget_label(dataSet.getId(), 2, label[1], AXIS_LABEL_LEN);

      if(assignLabelandDims) {
        if(ndims!=3) {
          std::cerr << "Expected 3D dimension for morphology. Found DIM = " << ndims << "\n";
          exit(EXIT_FAILURE);
        }
        assert(morphologyOrder == MorphologyOrder::INVALID);
        if (((strcmp(label[0], "Z") == 0) and (strcmp(label[1], "X") == 0))) {
          morphologyOrder = MorphologyOrder::ZYX;
          voxelSize[0] = voxelDims[2];
          voxelSize[1] = voxelDims[1];
          voxelSize[2] = voxelDims[0];
        }
        else if(((strcmp(label[0], "X") == 0) and (strcmp(label[1], "Z") == 0))){
          morphologyOrder = MorphologyOrder::XYZ;
          voxelSize[0] = voxelDims[0];
          voxelSize[1] = voxelDims[1];
          voxelSize[2] = voxelDims[2];
        }
        else {
          std::cerr << "Not supported morphology order \n";
          exit(EXIT_FAILURE);
        }
      }
      else {
        // We store voxel dimension as (X,Y,Z) irrespective of HDF axis label
        if (morphologyOrder == MorphologyOrder::ZYX) {
          if (not((strcmp(label[0], "Z") == 0) and (strcmp(label[1], "X") == 0))) {
            throw std::logic_error("Axis label mismatch for morphology");
          }
          if ((ndims != 3) or (voxelDims[0] != voxelSize[2]) or (voxelDims[1] != voxelSize[1]) or (voxelDims[2] != voxelSize[0])) {
            std::cout << "Error in morphology for Material = " << i << "\n";
            std::cout << "Expected dimension (X,Y,Z) = " << voxelSize[0] << " " << voxelSize[1] << " " << voxelSize[2]
                      << "\n";
            std::cout << "Dimensions from HDF5 (X,Y,Z)          = " << voxelDims[2] << " " << voxelDims[1] << " "
                      << voxelDims[0] << "\n";
            throw std::logic_error("Dimension mismatch for morphology");
          }
        }
        if (morphologyOrder == MorphologyOrder::XYZ) {
          if (not((strcmp(label[0], "X") == 0) and (strcmp(label[1], "Z") == 0))) {
            throw std::logic_error("Axis label mismatch for morphology");
          }
          if ((ndims != 3) or (voxelDims[0] != voxelSize[0]) or (voxelDims[1] != voxelSize[1]) or (voxelDims[2] != voxelSize[2])) {
            std::cout << "Error in morphology for Material = " << i << "\n";
            std::cout << "Expected dimension (X,Y,Z)   = " << voxelSize[0] << " " << voxelSize[1] << " " << voxelSize[2]
                      << "\n";
            std::cout << "Dimensions from HDF5 (X,Y,Z) = " << voxelDims[0] << " " << voxelDims[1] << " " << voxelDims[2]
                      << "\n";
            throw std::logic_error("Dimension mismatch for morphology");
          }
        }
      }

#ifdef DOUBLE_PRECISION
      if(dataType != PredType::NATIVE_DOUBLE){
         std::cout << "The data format is not supported for double precision \n";
         exit(EXIT_FAILURE);
      }
      dataSet.read(inputData[i - 1].data(), H5::PredType::NATIVE_DOUBLE);
#else
      if (dataType == PredType::NATIVE_DOUBLE) {
        std::vector<double> unalignedData(numVoxel);
        dataSet.read(unalignedData.data(), H5::PredType::NATIVE_DOUBLE);
        if(morphologyOrder == MorphologyOrder::XYZ){
          XYZ_to_ZYX(unalignedData,1,voxelSize);
        }
        for (int id = 0; id < unalignedData.size(); id++) {
          inputData[id] = static_cast<Real>(unalignedData[id]);
        }
      } else if (dataType == PredType::NATIVE_FLOAT) {
        std::vector<float> unalignedData(numVoxel);
        dataSet.read(unalignedData.data(), H5::PredType::NATIVE_FLOAT);
        if(morphologyOrder == MorphologyOrder::XYZ){
          XYZ_to_ZYX(unalignedData,1,voxelSize);
        }
        for (int id = 0; id < unalignedData.size(); id++) {
          inputData[id] = static_cast<Real>(unalignedData[id]);
        }
      } else {
        std::cerr << "This data format is not supported \n";
        exit(EXIT_FAILURE);
      }
      dataType.close();
      dataSet.close();
#endif
    }

  }

/**
 * @brief reads the hdf5 file
 * @param hdf5file hd5 file to read
 * @param voxelSize voxelSize in 3D
 * @param voxelData voxelData
 * @param isAllocated true if the size of voxelData is allocated
 */

  static int readFile(const std::string &hdf5file, UINT *voxelSize, Voxel *&voxelData,
                      const MorphologyType &morphologyType, int &morphologyOrder, bool isAllocated = false) {
    H5::H5File file(hdf5file, H5F_ACC_RDONLY);
    BigUINT numVoxel = static_cast<BigUINT>((BigUINT) voxelSize[0] * (BigUINT) voxelSize[1] * (BigUINT) voxelSize[2]);


    if (not isAllocated) {
      voxelData = new Voxel[numVoxel * NUM_MATERIAL];
    }
    if (morphologyType == MorphologyType::VECTOR_MORPHOLOGY) {
      {
        for(int numMat = 0; numMat < NUM_MATERIAL; numMat++){
          std::vector<Real> unalignedData;
          getScalar(file, "vector_morphology", "_unaligned", voxelSize, morphologyOrder, unalignedData,numMat,true, (numMat == 0)?true:false);
          for (UINT i = 0; i < numVoxel; i++) {
            voxelData[numMat * numVoxel + i].s1.w = unalignedData[i];
          }
        }
      }
      {
        for (UINT numMat = 0; numMat < NUM_MATERIAL; numMat++) {
        std::vector<Real > alignmentData;
        getMatAllignment(file, voxelSize, alignmentData, static_cast<const MorphologyOrder>(morphologyOrder), numMat,true);
          for (BigUINT i = 0; i < numVoxel; i++) {
            voxelData[numMat * numVoxel + i].s1.x = alignmentData[3 * i + 0];
            voxelData[numMat * numVoxel + i].s1.y = alignmentData[3 * i + 1];
            voxelData[numMat * numVoxel + i].s1.z = alignmentData[3 * i + 2];
          }
        }
      }
    } else if (morphologyType == MorphologyType::EULER_ANGLES) {
      /// TODO
    } else {
      throw std::runtime_error("Wrong type of morphology");
    }
    return EXIT_SUCCESS;
  }
}
#endif //PRS_READH5_H
