#ifndef FAU_POT_HSCOULOMB_H
#define FAU_POT_HSCOULOMB_H
#include "faunus/potentials/base.h"
namespace Faunus {
  /*! \brief Hardsphere + Coulomb potential
   *  \author Mikael Lund
   *  \date Prague, 2007
   */
  class pot_hscoulomb {
    public:
      double f;
      string name;
      pot_hscoulomb (const inputfile &in) {
        f=in.getflt("bjerrum",7.1);
        name="Coulomb + Hardsphere";
      }
      /*! Return Coulomb energy between a pair of particles
       *
       *  \return Energy in units of kT/f (f=lB).
       *  \f[ \beta u/l_B = \frac{z_1 z_2}{r} + u_{hs} \f]
       *  \f[ u_{hs}(r) = \infty \hspace{0.5cm} r_{ij} < \frac{\sigma_i+\sigma_j}{2} \f]
       */
      inline double pairpot(const particle &p1, const particle &p2) {
        double r2=p1.sqdist(p2), s=p1.radius+p2.radius;
        return (r2<s*s) ? 200. : p1.charge*p2.charge/sqrt(r2);
      }
      virtual string info() {
        std::ostringstream o;
        o << "#   Type              = " << name << std::endl
          << "#   Bjerrum length    = " << f << std::endl;
        return o.str();
      }
  };
}
#endif
