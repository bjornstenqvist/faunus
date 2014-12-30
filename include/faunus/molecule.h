#ifndef faunus_molecule_h
#define faunus_molecule_h

#include <faunus/io.h>
#include <faunus/inputfile.h>
#include <faunus/bonded.h>
#include <faunus/geometry.h>

namespace Faunus {

  /**
   * @brief Random position and orientation - typical for rigid bodies
   *
   * Molecule inserters take care of generating molecules
   * for insertion into space and can be used in Grand Canonical moves,
   * Widom analysis, and for generating initial configurations.
   * Inserters will not actually insert anything, but rather
   * return a particle vector with proposed coordinates.
   *
   * All inserters are function objects, expecting
   * a geometry, particle vector, and molecule data.
   */
  template<typename TMoleculeData>
    struct RandomInserter {
      typedef typename TMoleculeData::TParticleVector Tpvec;
      string name;
      Point dir;         //!< Scalars for random mass center position. Default (1,1,1)
      Point offset;      //!< Added to random position. Default (0,0,0)
      bool checkOverlap; //!< Set to true to enable container overlap check

      RandomInserter() : dir(1,1,1), offset(0,0,0), checkOverlap(true) { name = "random"; }

      Tpvec operator() (Geometry::Geometrybase &geo, const Tpvec &p, const TMoleculeData &mol) {
        typedef typename Tpvec::value_type Tparticle;
        bool _overlap=true;
        Tpvec v;
        do {
          if (mol.isAtomic()) {
            for (auto &id: mol.atoms) { // for each atom type id
              int n; // number of atoms to insert
              if (atom[id].activity>1e-9)
                n=1;
              else 
                n = atom[id].Ninit;
              v.reserve( n );
              while (n-- > 0) {
                v.push_back( Tparticle() );
                v.back() = atom[id];   // set part type
                Geometry::QuaternionRotate rot;
                Point u;
                u.ranunit(slump);
                rot.setAxis(geo, {0,0,0}, u, pc::pi * slump());
                v.back().rotate(rot);
                geo.randompos( v.back() );
                v.back() = v.back().cwiseProduct(dir) + offset;
                geo.boundary( v.back() );
              }
              atom[id].Ninit = -1;
            }
          } else {
            v = mol.getRandomConformation();
            Point a, b;
            geo.randompos(a);                  // random point in container
            a = a.cwiseProduct(dir);           // apply user defined directions (default: 1,1,1)
            Geometry::cm2origo(geo, v);        // translate to origo - obey boundary conditions
            Geometry::QuaternionRotate rot;
            b.ranunit(slump);                  // random unit vector
            rot.setAxis(geo, {0,0,0}, b, slump() * 2 * pc::pi); // random rot around random vector
            for (auto &i : v) {                // apply rotation to all points
              i = rot(i) + a + offset;         // ...and translate
              geo.boundary(i);                 // ...and obey boundaries
            }
          }

          assert( !v.empty() );
          _overlap=false;
          if ( checkOverlap )                  // check for container overlap
            for ( auto &i : v )
              if ( geo.collision(i, i.radius) ) {
                _overlap = true;
                break;
              }
        } while ( _overlap == true );
        return v;
      }
    };

  /**
   * @brief Storage for molecular properties
   *
   * Values can be read from a JSON object with the following format:
   *
   * ~~~~
   *    "mymolecule" : { "atoms":"ARG ARG GLU", "structure":"peptide.aam" }
   * ~~~~
   *
   * The key of type string is the `name` followed, in no particular order,
   * by properties:
   *
   * Key           | Type    | Description
   * :------------ | :------ | : ------------------------------------------------
   * `activity`    | float   | Chemical activity for grand canonical MC [mol/l]
   * `atoms`       | string  | List of atoms in molecule (use `AtomData` names)
   * `atomic`      | bool    | `true` if molecule is to be considered as a collection of free atomic species (default: false)
   * `bonds`       | string  | List of harmonic bonds - index 0 corresponds to first atom in structure
   * `dihedrals`   | string  | List of dihedrals (under construction!)
   * `insdir`      | string  | Directions for generation of random position. Default: "1 1 1" = XYZ
   * `insoffset`   | string  | Translate generated random position. Default: "0 0 0" = no translation
   * `Ninit`       | int     | Initial number of molecules
   * `structure`   | string  | Read conformation from AAM file
   */                         
  template<class Tpvec>
    class MoleculeData  : public PropertyBase {
      private:
        using PropertyBase::Tjson;
        bool _isAtomic;

        /** @brief Signature for inserted function */
        typedef std::function<Tpvec( Geometry::Geometrybase&,
            const Tpvec&, const MoleculeData<Tpvec>& )> TinserterFunc;

        TinserterFunc inserterFunctor;              //!< Function for insertion into space

      public:
        typedef Tpvec TParticleVector;
        typedef typename Tpvec::value_type Tparticle;

        std::vector<typename Tparticle::Tid> atoms; //!< List of atoms in molecule
        vector<Tpvec> conformations;                //!< Conformations of molecule
        double activity;                            //!< Chemical activity (mol/l)
        double chemPot;                             //!< Chemical potential (kT)
        Average<double> rho;                        //!< Average concentration (1/angstrom^3)
        vector<Bonded::BondData> bonds;             //!< List of harmonic bonds
        vector<Bonded::DihedralData> dihedrals;     //!< List of harmonic bonds
        int Ninit;                                  //!< Initial number of molecules

        /** @brief Constructor - by default data is initialized; mass set to unity */
        inline MoleculeData( const Tjson &molecule=Tjson()) : _isAtomic(false), Ninit(0) {
          readJSON(molecule);
          auto ins = RandomInserter<MoleculeData<Tpvec>>();
          ins.dir    << json::value<string>( molecule.second, "insdir", "1 1 1" );
          ins.offset << json::value<string>( molecule.second, "insoffset", "0 0 0" );
          setInserter( ins );
        }

        /** @brief Get list of bonds for molecule */
        std::vector<Bonded::BondData>& getBondList() { return bonds; }

        /**
         * @brief Specify function to be used when inserting into space.
         *
         * By default a random position and orientation is generator and overlap
         * with container is avoided.
         */
        void setInserter( const TinserterFunc &ifunc ) { inserterFunctor = ifunc; };

        /** @brief Get a random conformation */
        Tpvec getRandomConformation() const {
          assert( !conformations.empty() );
          return *slump.element( conformations.begin(), conformations.end() );
        }

        /**
         * @brief Get random conformation that fits in container
         * @param geo Geometry
         * @param otherparticles Typically `spc.p` is insertion depends on other particle
         *
         * By default the molecule is placed at a random position and orientation with
         * no container overlap using the `RandomInserter` class. This behavior can
         * be changed by specifying another inserter using `setInserter()`.
         */
        Tpvec getRandomConformation( Geometry::Geometrybase &geo, const Tpvec &otherparticles=Tpvec() ) {
          return inserterFunctor( geo, otherparticles, *this );
        }

        /**
         * @brief Store a single configuration
         * @param vec Vector of particles
         * @todo Add weight parameter for chance of picking it
         */
        void pushConfiguration(const Tpvec& vec) {
          conformations.push_back(vec);
        }

        /** True if molecule holds individual atoms */
        bool isAtomic() const { return _isAtomic; }

        /** True if molecule holds molecular entities */
        bool isMolecular() const { return !_isAtomic; }

        bool operator==(const MoleculeData &d) const { return (*this==d); }

        /** @brief Read data from a picojson object */
        void readJSON(const Tjson &molecule) FOVERRIDE {

          name = molecule.first;

          activity = json::value<double>(molecule.second, "activity", 0);
          chemPot = log( activity * 1.0_molar );

          _isAtomic = json::value<bool>(molecule.second, "atomic", false);

          Ninit = json::value<double>(molecule.second, "Ninit", 0 );

          // create bond list
          for (auto &i : json::object("bonds", molecule.second) ) {
            bonds.push_back( Bonded::BondData(i) );
          }

          // create dihedral list
          for (auto &i : json::object("dihedrals", molecule.second) )
            dihedrals.push_back( Bonded::DihedralData(i) );

          // read conformation from disk
          string structure = json::value<string>( molecule.second, "structure", "" );
          if ( !structure.empty() ) {
            Tpvec v;
            if ( FormatAAM::load( structure, v ) ) {
              if ( !v.empty() ) {
                conformations.push_back( v ); // add conformation
                for ( auto &p : v )           // add atoms to atomlist
                  atoms.push_back(p.id);
                cout << "# added molecular structure: " << structure << endl;
              }
            }
            else
              std::cerr << "# error loading molecule: " << structure << endl;
          }

          // add atoms to atom list
          if ( atoms.empty() ) {
            string atomlist = json::value<string>(molecule.second, "atoms", "");
            for ( auto &a : textio::words2vec<string>(atomlist) )
              if ( atom[a].id > 0 )
                atoms.push_back( atom[a].id );
          }

          // for atomic groups, add particles
          if ( isAtomic() ) {
            Tpvec _vec;
            Tparticle _p;
            for (auto id : atoms) {
              _p = atom[id];
              _vec.push_back( _p );
            }
            conformations.push_back( _vec );
          }
        }

        inline string info() {

          using namespace textio;
          ostringstream o;
          char w=22;

          o << header("Molecule: " + name)
            << pad(SUB, w, "Activity") << activity << " mol/l\n"
            << pad(SUB, w, "Chem. pot.") << chemPot << " kT\n"
            << pad(SUB, w, "Atomic") << std::boolalpha << isAtomic() << "\n"
            << pad(SUB, w, "Conformations") << conformations.size() << "\n"
            << pad(SUB, w, "Number of atoms") << std::max(atoms.size(), conformations.size()) << "\n";
          if (Ninit>0)
            o << pad(SUB, w, "N initial") << Ninit << "\n";

          if (rho.cnt>0) {
            o << pad(SUB, w, "Average conc.") << rho.avg() << " 1/"+angstrom+" = "
              << rho.avg() / 1.0_molar << " mol/l\n"
              << pad(SUB, w, "Activity coefficient") << activity / rho.avg() << "\n";
          }

          return o.str();
        }

    };

  /**
   * @brief Class for loading and storing Molecule properties
   *
   * This will load molecule properties from disk and store them in a
   * vector of `MoleculeData`. The file format is JSON (<http://www.json.org>)
   * and all molecule properties must be inclosed in an object with
   * the keyword `moleculelist`.
   * While not strictly JSON compliant, comments beginning
   * with `//` are allowed.
   *
   * For example:
   *
   * ~~~~
   * {
   *   "moleculelist" :
   *   {
   *     "salt" : { "atoms":"Na Cl", "atomic":True },
   *     "polymer" :
   *        { "activity":0.05, "atoms":"MM MM MM MM",
   *         "bonds" :
   *         {
   *           "0 1" : { "k":0.5, "req":4.0 },
   *           "1 2" : { "k":0.5, "req":4.0 },
   *           "2 3" : { "k":0.5, "req":4.0 }
   *         },
   *         "dihedrals" :
   *         {
   *           "0 1 2 3" : { "k1":0.01, "k2":2.123 }
   *         }
   *       }
   *   }
   * }
   * ~~~~
   *
   * Note that faunus currently has a global instance of `MoleculeMap`,
   * simply named `molecule`. This can be accessed from anywhere.
   *
   * @todo More documentation
   */
  template<class Tpvec, class base=PropertyVector<MoleculeData<Tpvec>>>
    class MoleculeMap : public base {
      public:
        using typename base::value_type;

        MoleculeMap() {
          base::jsonsection = "moleculelist";
          base::name = "Molecule Properties";
        }

        /** @brief Read JSON file given through `InputMap` */
        bool includefile(InputMap &in) {
          string file = in("moleculelist", string("") );
          return base::includefile(file);
        }

        /** @brief Information string */
        string info() {
          char w=15;
          using namespace textio;
          ostringstream o;
          o << header("Molecule list") << "\n"
            << std::left << setw(w+4) << "  Molecule" << setw(w) << "activity"
            << setw(w) << "atoms" << setw(w) << "atomic" << "\n"
            << "  " << string(4*w, '-') << "\n";

          for (auto &i : *this)
            if ( !i.name.empty() )
              o << setw(w+4) << "  "+i.name << setw(w) << i.activity
                << setw(w) << std::max(i.conformations.size(), i.atoms.size())
                << std::boolalpha << i.isAtomic()
                << "\n";
          return o.str();
        }
    };

  /**
   * @brief Molecular combinations for Grand Canonical moves, Widom insertion etc.
   *
   * JSON entry examples:
   *
   * ~~~~
   * "moleculelist" : {
   *    "ion1" : { "atoms" : "Na" },
   *    "ion2" : { "atoms" : "Cl" }
   * },
   *
   * "moleculecombinations" : {
   *    "NaCl" : { "molecules" : "ion1 ion2", "prob" : 0.5 },
   * }
   * ~~~~
   *
   * The key of type string is the `name` followed, in no particular order,
   * by properties:
   *
   * Key           | Description
   * :------------ | :--------------------------------------
   * `molecules`   | list of space separated molecule names
   * `prob`        | insertion probability [0:1] (default: 1=100%)
   *
   * the final probability of combination is: prob_i / (sum_all prob_i)
   */
  template<class Tpvec>
    class MoleculeCombination : public PropertyBase {

      private:

        inline void readJSON(const Tjson &comb) FOVERRIDE {
          molComb.clear();
          name = comb.first;
          probability = json::value<double>( comb.second, "prob", 1.0 );
          string mollist = json::value<string>( comb.second, "molecules", "" );
          molName = textio::words2vec<string>(mollist);
        }

      public:

        MoleculeCombination(const Tjson &comb=Tjson()) { readJSON(comb); }

        vector<PropertyBase::Tid> molComb;/// vector of molecule index in combination

        vector<string> molName;           /// vector of molecule names in combination

        double probability;               /// probability of this combination in GC-MC move

    };

  /**
   * @brief Vector of molecular combinations
   *
   * When examining a JSON file, individual entries must be placed
   * in a section called `moleculecombinations`. When loading
   * from `InputMap` the keyword for the json file is
   * also `moleculecombinations`.
   */
  template<class Tpvec, class base=PropertyVector< MoleculeCombination<Tpvec> >>
    class MoleculeCombinationMap : public base {
      private:

        MoleculeMap<Tpvec> *molmap; // typically points to `Space::molecule`

        void update() { // convert name (string) -> id (int) using MoleculeMap
          for (auto &i : *this) {
            assert( !i.molName.empty() && "Molecule combination cannot be empty");
            for (auto &name : i.molName) {
              auto it = molmap->find(name);
              if ( it!=molmap->end() )
                i.molComb.push_back(it->id);
            }
            assert( i.molName.size() == i.molComb.size() );
            assert( !i.molName.empty() );
          }
        }

      public:

        bool includefile(InputMap &in) {
          bool rc = base::includefile( in(base::jsonsection, string("") ) );
          update();
          return rc;
        }

        bool includefile(const string& file) {
          bool rc = base::includefile(file);
          update();
          return rc;
        }

        MoleculeCombinationMap(MoleculeMap<Tpvec> &moleculemap) : molmap(&moleculemap) {
          assert( molmap != nullptr);
          base::name = "Molecule Combinations";
          base::jsonsection = "moleculecombinations";
        }

        string info() {
          using namespace textio;
          char w=25;
          std::ostringstream o;
          o << header(base::name)
            << pad(SUB,w,"Number of entries:") << base::size() << endl;
          if (!base::jsonfile.empty())
            o << pad(SUB,w,"Input JSON file:") << base::jsonfile << endl;
          o << indent(SUB) << "Element info:\n";
          for (auto &i : *this) {
            o << std::left << setw(w) << "    "+i.name+":";
            for (auto &n : i.molName)
              o << setw(10) << n;
            o << "\n";
          }
          o << endl;
          return o.str();
        }

    };

}//namespace

#endif
