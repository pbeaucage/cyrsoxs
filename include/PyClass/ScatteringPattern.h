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
#ifndef CY_RSOXS_PROJECTIONDATA_H
#define CY_RSOXS_PROJECTIONDATA_H
#include <Input/InputData.h>
#include <utils.h>
/**
 * @brief Stores the Scattering pattern data
 */
class ScatteringPattern{

    /// Input data
    const InputData & inputData_;
    /// Pointer to the scattering pattern data
    Real * data_ = nullptr;
    /// Track the state of data whether its allocated or not
    bool isAllocated_ = false;

public:
    /**
     * @brief Constructor
     * @param InputData inputData
     */
    ScatteringPattern(const InputData & InputData)
    :inputData_(InputData){

    }

    /**
     * @brief Allocates the memory
     */
    void allocate(){

      if(isAllocated_){
        py::print("The memory is already allocated. Please deallocate it first by calling clear");
        return;
      }
      isAllocated_ = true;

      const UINT
          numEnergyLevel = static_cast<UINT>(std::round((inputData_.energyEnd - inputData_.energyStart) / inputData_.incrementEnergy + 1));
      data_ = new Real[numEnergyLevel * inputData_.numX * inputData_.numY];
    }

    /**
     * @brief Pointer to the data
     * @return the pointer to the scattering pattern data
     */
    inline Real * data(){
      if(not(isAllocated_)){
        py::print("[ERROR] Memory not allocated. Please allocate it first");
      }
      return data_;
    }

    /**
     * @brief Clears up the memory
     */
    void clear(){
      if(data_ != nullptr) {
        delete[] data_;
        data_ = nullptr;
        isAllocated_ = false;
      }
    }

    /**
     * @brief Destructor
     */
    ~ScatteringPattern(){
      clear();
    }

    /**
     * @brief Writes Scattering pattern data to HDF5 file
     */
    void writeToHDF5(){
      const UINT voxelDimensions[3]{inputData_.numX,inputData_.numY,inputData_.numZ};
      writeH5(inputData_, voxelDimensions, data_);
    }

    /**
     * @brief Writes scattering pattern data to VTI paraview format
     */
    void writeToVTI(){
      const UINT voxelDimensions[3]{inputData_.numX,inputData_.numY,inputData_.numZ};
      writeVTI(inputData_, voxelDimensions, data_);
    }


    /**
     * @brief returns the data in the numpy array. Note that it is the same memory allocation
     * with numpy wrapper.
     * @param energy Energy for which the numpy array is needed
     * @return numpy numpy array with the scattering pattern data of the energy
     */
    py::array_t<Real> writeToNumpy(const Real energy){
      if((energy < inputData_.energyStart) or (energy > inputData_.energyEnd)){
        py::print("[LOG]: Wrong EnergyID");
        return py::array_t<Real>{};
      }

      const UINT
      energyID = static_cast<UINT>(std::round( (energy - inputData_.energyStart)/ inputData_.incrementEnergy));

      if(not(FEQUALS(inputData_.energyStart + energyID*inputData_.incrementEnergy,energy))){
        py::print("[LOG]: Wrong EnergyID");
        return py::array_t<Real>{};
      }
      py::capsule free_when_done(this->data_, [](void *f) {
      });

      return (py::array_t<Real>(
      {(int)inputData_.numX,(int)inputData_.numY},
      {sizeof(Real)*inputData_.numY,sizeof(Real)},
      &this->data_[energyID*(inputData_.numY*inputData_.numX)],
          free_when_done));


    }


};
#endif //CY_RSOXS_PROJECTIONDATA_H