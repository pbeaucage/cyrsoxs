//
// Created by maksbh on 9/4/20.
//

#ifndef CY_RSOXS_REFRACTIVEINDEX_H
#define CY_RSOXS_REFRACTIVEINDEX_H


#include <pybind11/numpy.h>
#include <omp.h>
#include <Input/readH5.h>
#include <Input/InputData.h>

namespace py = pybind11;

/**
 * @brief Class to store the refractive index data
 */
class RefractiveIndexData {
    const Real &energyStart; /// Start of energy
    const Real &energyEnd;  /// End of energy
    const Real &incrementEnergy; /// Increment energy
    std::vector<Material<NUM_MATERIAL> > refractiveIndex; /// Refractive index data at all energy level
    std::vector<bool> isValid; /// A vector of bool to check if the optical constants is added at each energy level.
public:
    /**
     * @brief Constructor
     * @param inputData InputData
     */
    RefractiveIndexData(const InputData &inputData)
        : energyStart(inputData.energyStart), energyEnd(inputData.energyStart),
          incrementEnergy(inputData.incrementEnergy) {
      UINT numEnergyData = std::round(inputData.energyEnd - inputData.energyStart) / (inputData.incrementEnergy) + 1;
      refractiveIndex.resize(numEnergyData);
      isValid.resize(numEnergyData);
      std::fill(isValid.begin(), isValid.end(), false);
    }

    /**
     * @brief Clears up the memory
     */
    void clear() {
      std::vector<Material<NUM_MATERIAL> > _materialInput;
      std::swap(_materialInput, refractiveIndex);
    }


    /**
     * @brief Computes the refractive index based on the optical constants Data
     * @param values The value for the material. Must be of the size (NUM_MATERIAL X 4).
     *                Note the optical constant data must be in the order of :
     *               [$\delta_{\parallel}$,$\beta_{\parallel}$, $\delta_{\perp}$, $\beta_{\perp}$]
     * @param Energy The value of energy
     */
    void addData(const std::vector<std::vector<Real>> &values, const Real Energy) {
      enum EnergyValues : u_short {
          DeltaPara = 0,
          BetaPara = 1,
          DeltaPerp = 2,
          BetaPerp = 3
      };
      if (not(values.size() == NUM_MATERIAL)) {
        py::print("Wrong input for Energy. Number not matching with number of Materials");
        return;
      }
      for (auto &value:values) {
        if ((value.size() != 4)) {
          py::print("Wrong number of input parameters. Parameters must be in the order of "
                    "(DeltaPara, BetaPara, DeltaPerp, , BetaPerp)");
          return;
        }
      }
      UINT counter = std::round((Energy - energyStart) / incrementEnergy);
      for (UINT i = 0; i < NUM_MATERIAL; i++) {
        refractiveIndex[counter].npara[i].x = 1 - values[i][EnergyValues::DeltaPara];
        refractiveIndex[counter].npara[i].y = values[i][EnergyValues::BetaPara];
        refractiveIndex[counter].nperp[i].x = 1 - values[i][EnergyValues::DeltaPerp];
        refractiveIndex[counter].nperp[i].y = values[i][EnergyValues::BetaPerp];
      }

      isValid[counter] = true;

    }

    /**
     * @brief prints the energy Data
     */
    void printEnergyData() const {
      UINT count = 0;
      for (auto &values : refractiveIndex) {
        Real currEnegy = energyStart + count * incrementEnergy;
        py::print("Energy = ", currEnegy);
        for (int i = 0; i < NUM_MATERIAL; i++) {
          py::print("Material = ", i, "npara = ", std::complex<Real>(values.npara[i].x, values.npara[i].y),
                    "nperp = ", std::complex<Real>(values.nperp[i].x, values.nperp[i].y));
        }
        count++;

      }
    }

    /**
     * @brief Getter function
     * @return The refractive index data
     */
    const std::vector<Material<NUM_MATERIAL>> &getRefractiveIndexData() const {
      return refractiveIndex;
    }

    /**
     * @brief Validates if the energy data is valid, i.e all the energy has been added to the energy data
     * @return True if the energy data is valid. False otherwise
     */
    bool validate() const {
      if(std::all_of(isValid.begin(), isValid.end(), [](bool x) { return (x == true); })){
        return true;
      }
      else{
        for(UINT i = 0; i < isValid.size(); i++){
          if(not(isValid[i])){
            Real energy = energyStart + i * incrementEnergy;
            py::print("Optical constants data missing for energy = ",energy,"eV");
          }
        }
      }
      return false;
    }
};

#endif //CY_RSOXS_REFRACTIVEINDEX_H