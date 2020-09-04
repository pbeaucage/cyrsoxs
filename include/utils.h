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
#ifndef CY_RSOXS_UTILS_H
#define CY_RSOXS_UTILS_H
#include <Input/InputData.h>
#include <iomanip>
#include <Output/outputUtils.h>

/**
 * @brief writes to HDF5 file
 * @param inputData Input data
 * @param voxelSize Voxel Size
 * @param projectionGPUAveraged The scattering pattern
 */
static void writeH5(const InputData & inputData, const UINT * voxelSize,const Real *projectionGPUAveraged){
    createDirectory("HDF5");
    // TODO: Make it work in parallel.
    omp_set_num_threads(1);

    const UINT
    numEnergyLevel = static_cast<UINT>(std::round((inputData.energyEnd - inputData.energyStart) / inputData.incrementEnergy + 1));
    const UINT voxel2DSize = voxelSize[0] * voxelSize[1];
    Real *oneEnergyData = new Real[voxel2DSize];
    UINT chunkSize = static_cast<UINT>(std::ceil(numEnergyLevel * 1.0 / (omp_get_num_threads() * 1.0)));
    UINT threadID = omp_get_thread_num();
    UINT startID = threadID * chunkSize;
    UINT endID = ((threadID + 1) * chunkSize);

    for (UINT csize = startID; csize < std::min(endID, numEnergyLevel); csize++) {
      std::stringstream stream;
      Real energy = inputData.energyStart + csize * inputData.incrementEnergy;
      stream << std::fixed << std::setprecision(2) << energy;
      std::string s = stream.str();
      std::memcpy(oneEnergyData, &projectionGPUAveraged[csize * voxel2DSize], sizeof(Real) * voxel2DSize);
      const std::string outputFname = "HDF5/Energy_" + s;

      H5::writeFile2D(outputFname, oneEnergyData, voxelSize);
    }
};

/**
 * Prints the copyRight Info
 */
static void printCopyrightInfo(){

  const char separator    = ' ';
  const int charWidth     = 80;
  std::cout << std::left << std::setw(charWidth) << std::setfill('_') << "\n";
  std::cout << "\n";
  std::cout << std::left << std::setw(charWidth) << "|_____________________________Thanks for using Cy-RSoXS"  << std::setfill(separator) << "|\n";
  std::cout << std::left << std::setw(charWidth) << "|Copyright @ Iowa State." << std::setfill(separator )  << "|\n";
  std::cout << std::left << std::setw(charWidth) << "|Developed at Iowa State in collaboration with NIST" << std::setfill(separator )  << "|\n";
  std::cout << std::left << std::setw(charWidth) << "|Distributed freely under MIT Licence." << std::setfill(separator) << "|\n";;
  std::cout << std::left << std::setw(charWidth) << "|Cite the publication for using this: ----" << std::setfill(separator) << "|\n";;
  std::cout << std::left << std::setw(charWidth) << "|Comments/Questions:" << std::setfill(separator) << "|\n";
  std::cout << std::left << std::setw(charWidth) << "|    Dr. Baskar Ganapathysubramanian (baskarg@iastate.edu)"<< std::setfill(separator) << "|\n";
  std::cout << std::left << std::setw(charWidth) << "|    Dr. Adarsh Krishnamurthy (adarsh@iastate.edu)" << std::setfill(separator) << "|\n";
  std::cout << std::left << std::setw(charWidth) << "|" << std::setfill('-') << "|\n";
  std::cout << "\n";
  std::cout << "\n";
  std::cout << "\n";
  std::cout << "\n";
}

/**
 * @brief Writes meta data
 * @param inputData Input data
 */

static void printMetaData(const InputData & inputData){
  std::ofstream file("metadata.txt");
  file << "---------------- Scaling Information--------------\n";
  file << "Number of pixel = [" << inputData.numX << "," << inputData.numY << "]\n";
  file << "Q range  = [" << -M_PI / inputData.physSize << "," << M_PI / inputData.physSize << "]\n";
  file << "Electric field rotated through = [" << inputData.startAngle << "," << inputData.endAngle << "]\n";
  file << "Increments in electric field rotation " << inputData.incrementEnergy << "\n";
  file << "\n\n";

  file << "-----------------Simulation information -----------------\n";
  file << "Size of Real = " << sizeof(Real) << "\n";
  file << "Number of materials =" << NUM_MATERIAL << "\n";
  file << "Energies simulated from " << inputData.energyStart << " to " << inputData.energyEnd << " with increment of "
       << inputData.incrementEnergy << "\n";
  file.close();
}

#endif //CY_RSOXS_UTILS_H
