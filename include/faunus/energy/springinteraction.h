#ifndef FAU_SPRINGENERGY_H
#define FAU_SPRINGENERGY_H

//#include "faunus/energy/base.h"

namespace Faunus {
  /*!
   * \brief Implementation of all energy functions
   * \author Mikael Lund
   *
   * This is an expression template that constructs the
   * energy functions of a particular pair potential. All
   * returned energies are in units of kT.
   *
   * \code
   * inputfile in("input.conf");
   * interaction<pot_coulomb> pot(in);
   * pot.energy(...);
   * \endcode
   */
  template<class T> class springinteraction : public interaction<T> {
    private:
      vector<unsigned short> l;
    public:
      using interaction<T>::pair;
      double k; //!< Spring constant
      double req; //!< Equilibrium distance
      springinteraction(inputfile &in) : interaction<T>(in) {
        interaction<T>::name="Full N^2 w. spring potential";
        k   = in.getflt("springconstant", 30);
        req = in.getflt("springeqdist", 2);
      };

      double u_monomer(const vector<particle> &p, const polymer &g, unsigned int i ) {
        // normal energy function if no neighbors
        // isnt this function a bit ambiguous? It either returns the interaction of particle i 
        // with a polymer or it returns the interaction of a monomer in a polymer with the 
        // rest of the system?? or did I get it wrong??
        if ( g.nb[i-g.beg].size()==0 )
          return interaction<T>::energy(p,g,i);

        // energy w. all particles, skip neighbors (uses stl::find - maybe slow).
        double u=0;
        for (unsigned int j=0; j<i; j++) 
          if ( g.areneighbors(i,j)==false ) 
            u+=pair.pairpot( p[i], p[j] );
        for (unsigned int j=i+1; j<p.size(); j++) 
          if ( g.areneighbors(i,j)==false )
            u+=pair.pairpot( p[i], p[j] );
        u=u*pair.f;

        // calc. spring and coulomb energy w. neighbors
        for (unsigned int j=0; j<g.nb[i-g.beg].size(); j++) {
          double r=sqrt( pair.sqdist( p[i], p[ g.nb[i-g.beg][j] ] ) ),
                 dr = r - req;
          u+=k*dr*dr + pair.f *pair.pairpot(p[i],p[g.nb[i-g.beg][j] ]); 
        }
        return u;
      }
      double uself_polymer(const vector<particle> &p, const polymer &g) {
        double dr,r,u=0;
        int i,j,n=g.end+1;
        for (i=g.beg; i<n-1; i++)
          for (j=i+1; j<n; j++)
            if ( g.areneighbors(i,j)==false )
              u+=pair.f*pair.pairpot(p[i], p[j]);
            else {
              r=sqrt( pair.sqdist( p[i], p[j] ) );
              dr = r - req;
              u+=k*dr*dr + pair.f * pair.pairpot(p[i],p[j]); 
            }
        return u;
      }
      double uself_popscmem(const vector<particle> &p, const popscmembrane &g) {
        double u=0;
        int i,j;
        for (i=0; i<g.pops.size(); i++) {
          u+=uself_polymer(p, g.pops[i]);
          for (j=i+1; j<g.pops.size(); j++) 
            u+=interaction<T>::energy(p,g.pops[i],g.pops[j]);
          for (j=0; j<g.popc.size(); j++)
            u+=interaction<T>::energy(p,g.pops[i],g.popc[j]);
        }
        for (i=0; i<g.popc.size(); i++) {
          u+=uself_polymer(p, g.popc[i]); 
          for (j=i+1; j<g.popc.size();j++)
            u+=interaction<T>::energy(p, g.popc[i], g.popc[j]);
        }
        return u;
      }
  };
}//namespace
#endif