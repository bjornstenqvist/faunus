#pragma once

#include "move.h"
#include "aux/pairmatrix.h"
#include <set>

namespace Faunus {
namespace Move {
/**
 * @brief Molecular cluster move
 */
class Cluster : public Movebase {
  private:
    typedef typename Space::Tpvec Tpvec;
    typedef typename Space::Tgroup Tgroup;
    Tspace &spc;
    Average<double> translational_mean_square_displacement;
    Average<double> rotational_mean_square_displacement;
    Average<double> average_cluster_size;
    double _bias = 0;                           //!< Current bias energy (currently zero or infinity)
    double translation_displacement_factor = 0; //!< User defined scaling of translation
    double rotation_displacement_factor = 0;    //!< User defined scaling of rotation
    double rotation_angle = 0;                  //!< Current rotation angle
    unsigned int bias_rejected = 0;             //!< Number of times rejection occurred due to bias rejection
    bool perform_rotation = true;               //!< true if cluster should be rotated
    bool single_layer = false;                  //!< stop cluster search after first layer of neighbors
    Point translation_direction = {1, 1, 1};    //!< Directions which to translate
    Point translation_displacement = {0, 0, 0}; //!< Current translational displacement
    Point rotation_axis = {0, 0, 0};            //! predefined axis of rotation (if zero, generate randomly)
    std::vector<std::string> molecule_names;    //!< names of molecules to be considered
    std::vector<int> molids;                    //!< molecule id's of molecules to be considered (must be sorted!)
    std::set<int> satellites;           //!< subset of molecule id's to cluster, but NOT act as nuclei (cluster centers)
    std::vector<size_t> molecule_index; //!< index of all possible molecules to be considered
    std::map<size_t, size_t> cluster_size_distribution; //!< distribution of cluster sizes
    PairMatrix<double, true> thresholds_squared;        //!< Cluster thresholds for pairs of groups

    void registerSatellites(const std::vector<std::string> &);      //!< Register satellites
    void parseThresholds(const json &j);                            //!< Read thresholds from json input
    Point clusterMassCenter(const std::vector<size_t> &) const;     //!< Calculates cluster mass center
    void updateMoleculeIndex();                                     //!< update `molecule_index`
    Eigen::Quaterniond setRandomRotation();                         //!< Sets random rotation angle and axis
    std::size_t findSeed();                                         //!< Find first particle; exclude satellites
    std::vector<size_t> findCluster(Space &spc, size_t seed_index); //!< Find cluster
    void _move(Change &change) override;                            //!< Performs the move
    double bias(Change &, double, double) override; //!< adds extra energy change not captured by the Hamiltonian
    void _reject(Change &) override;
    void _accept(Change &) override;
    void _to_json(json &j) const override;
    void _from_json(const json &molid) override;
    virtual double clusterProbability(const Tgroup &group1, const Tgroup &group2) const;

  public:
    Cluster(Space &spc);
};

} // namespace Move
} // namespace Faunus
