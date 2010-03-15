#ifndef FAU_POINT_H
#define FAU_POINT_H

#include "faunus/common.h"
#include "faunus/slump.h"

namespace Faunus {
  /*!
   * \brief Hypersphere coordinates
   * \author Martin Trulsson
   * \date Lund, 2009
   *
   * This class defines particles on a hypersphere. When the macro definition
   * HYPERSPHERE is set the normal cartesian point class will inherit from this
   * class. The build system currently creates a separate dynamic library, linhyperfaunus,
   * to be used specifically for simulations on a hypersphere.
   */
  class hyperpoint {
    public:
      double z1,z2,z3,z4;                     //!< Reduced Coordinates on hypersphere
      void move(double,double,double);        //!< Translate a point along the surface of the sphere
      friend std::ostream &operator<<(std::ostream &, hyperpoint &);
      hyperpoint &operator<<(std::istream &);

      void hypclear() {
        z1=z2=z3=0;
        z4=1;
      }

      hyperpoint() {
        hypclear();
      }

      /*!
       * \brief Squared distance between two points.
       * \return \f[ r^2 = z_1z_1' + z_2z_2' + z_3z_3' + z_4z_4' \f]
       */
      inline double hypsqdist(const hyperpoint &p) const {
        return z1*p.z1+z2*p.z2+z3*p.z3+z4*p.z4;
      }

      /*!
       * \brief Geodesic distance between two hyperpoints
       * \return \f[ r_{\mbox{\scriptsize{geod}}} = \arccos{ (r^2) } \f]
       */
      inline double geodesic(const hyperpoint &p) const {
        return std::acos(hypsqdist(p));
      }
  };

  /*!
   * \brief Cartesian coordinates
   * \author Mikael Lund
   * \date 2002-2007
   */
  class point
#ifdef HYPERSPHERE
    : public hyperpoint
#endif
    {
      private:
        inline int anint(double) const;
      public:
        double x,y,z;                       //!< Cartesian coordinates
        point();                            //!< Constructor, zero data.
        point(double,double,double);        //!< Constructor, set vector
        void clear();                       //!< Zero all data.
        double len() const; 
        inline double sqdist(const point &) const;      //!< Squared distance to another point
        inline double sqdist(const point &,
            const double &, const double &) const;      //!< Squared distance to andother point (3D minimum image)
        inline double dist(const point &) const;        //!< Distance to another point
        inline double dist(const point &, const double &, const double &) const ; //!< Distance to another point
        void ranunit(random &);               //!< Generate a random unit vector
        double dot(const point &) const;      //!< Angle with another point
        point operator-();                    //!< Sign reversal
        point operator*(const point);         //!< Multiply two vectors
        point operator*(double) const;        //!< Scale vector
        point operator+(const point);         //!< Add two vectors
        point operator-(const point) const;   //!< Subtract vector
        point operator+(double);              //!< Displace x,y,z by value
        point & operator+=(const point&);
        bool operator==(const point&) const;
        std::string str();
        friend std::ostream &operator<<(std::ostream &, point &); //!< Output information
        point &operator<<(std::istream &);                        //!< Get information
    };

  /*!
   * \brief Class for particles
   * \author Mikael Lund
   * \date 2002-2007
   *
   * Example\n
   * \code
   * vector<particle> p(2);
   * p[0].radius = 2.0;
   * p[1].z = 10;
   * p[0].overlap( p[1] ); --> false
   * \endcode
   */
  class particle : public point {
    public:
      particle();
      double charge;                         //!< Charge number
      double radius;                         //!< Radius
      float mw;                              //!< Molecular weight
      unsigned char id;                      //!< Particle identifier
      bool hydrophobic;                      //!< Hydrophobic flag
      inline bool overlap(const particle &) const; //!< Hardsphere overlap test
      inline bool overlap(const particle &, const double &) const;
      inline bool overlap(const particle &, const double &, const double &) const;
      inline double potential(const point &);   //!< Electric potential in point
      double volume() const;                    //!< Return volume of sphere
      double mw2vol(double=1) const;            //!< Estimate volume from weight
      double mw2rad(double=1) const;            //!< Estimate radius from weight
      particle& operator=(const point&);        //!< Copy coordinates from a point
      void deactivate();                        //!< Deactivate for use w. faster energy loops
      void clear();                             //!< Clear/reset all data
      friend std::ostream &operator<<(std::ostream &, particle &); //!< Output information
      particle &operator<<(std::istream &);                        //!< Get information
   };

  /*! 
   * \brief Class for spherical coordinates
   * \author Mikael Lund
   * \date Canberra, 2005-2006
   */
  class spherical {
    public:
      double r,     //!< Radial
             theta, //!< Zenith angle \f$[0:\pi]\f$
             phi;   //!< Azimuthal angle \f$[0:2\pi]\f$
      spherical(double=0,double=0,double=0);
      point cartesian();                            //!< Convert to cartesian coordinates
      void operator=(point &);                      //!< Convert from cartesian coordinates
      void random_angles();                         //!< Randomize angles
  };

  inline void spherical::operator=(point &p) {
    r=p.len();
    theta=acos(p.z/r);
    phi=asin( p.y/sqrt(p.x*p.x+p.y*p.y) );
    if (p.x<0)
      phi=acos(-1.) - phi;
  }

  inline point spherical::cartesian() {
    point p;
    p.x=r*sin(theta)*cos(phi);
    p.y=r*sin(theta)*sin(phi);
    p.z=r*cos(theta);
    return p;
  }

  //! \todo This function is not completed
  inline void spherical::random_angles() {
    r=1.0 ;
  }

  /*!
   * \return \f$ r_{12}^2 = \Delta x^2 + \Delta y^2 + \Delta z^2 \f$
   */
  inline double point::sqdist(const point &p) const {
    register double dx,dy,dz;
    dx=x-p.x;
    dy=y-p.y;
    dz=z-p.z;
    return dx*dx + dy*dy + dz*dz;
  }

  inline int point::anint(double a) const { return int(a>0 ? a+.5 : a-.5); }

  inline double point::sqdist(const point &p, const double &len, const double &halflen) const {
    double dx,dy,dz;
    dx=std::abs(x-p.x);
    dy=std::abs(y-p.y);
    dz=std::abs(z-p.z);
    if (dx>halflen) dx-=len;
    if (dy>halflen) dy-=len;
    if (dz>halflen) dz-=len;
    return dx*dx + dy*dy + dz*dz;
  }

  inline double point::dist(const point &p) const { return sqrt(sqdist(p)); }

  inline double point::dist(const point &p, const double &len, const double &halflen) const { 
    return sqrt(sqdist(p, len, halflen)); }

  /*!
   * \return \f$ \phi = \frac{z}{r_{12}}\f$
   * \note Not multiplied with the Bjerrum length!
   */
  inline double particle::potential(const point &p) { return charge / dist(p); }

  /*!
   * \return True if \f$ r_{12}<(\sigma_1+\sigma_2)/2 \f$ - otherwise false.
   */
  inline bool particle::overlap(const particle &p) const {
    double r=radius+p.radius;
    return (sqdist(p) < r*r) ? true : false;
  }

  inline bool particle::overlap(const particle &p, const double &s) const {
    double r=radius+p.radius+s;
    return (sqdist(p) < r*r) ? true : false;
  }

  inline bool particle::overlap(const particle &p, const double &len, const double &halflen) const {
    double r=radius+p.radius;
    return (sqdist(p,len,halflen) < r*r) ? true : false;
  }

}//namespace
#endif
