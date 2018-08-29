/*~--------------------------------------------------------------------------~*
 * Copyright (c) 2017 Los Alamos National Security, LLC
 * All rights reserved.
 *~--------------------------------------------------------------------------~*/

 /*~--------------------------------------------------------------------------~*
 * 
 * /@@@@@@@@  @@           @@@@@@   @@@@@@@@ @@@@@@@  @@      @@
 * /@@/////  /@@          @@////@@ @@////// /@@////@@/@@     /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@   /@@/@@     /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@@@@@@ /@@@@@@@@@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@////  /@@//////@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@      /@@     /@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@      /@@     /@@
 * //       ///  //////   //////  ////////  //       //      //  
 *
 *~--------------------------------------------------------------------------~*/

/**
 * @file physics.h
 * @author Julien Loiseau
 * @date April 2017
 * @brief Basic physics implementation
 */

#ifndef _physics_kernel_h_
#define _physics_kernel_h_

#include <vector>

#include "tree.h"

namespace kernels{

  /**
   * @brief      Compute the size of the vector 
   * needed in the computation of the gradients
   *
   * @param      p     The vector, in point_t shape
   *
   * @return     The size of the vector
   */
  double vector_norm(
    const point_t& p){
    if (gdimension == 3)
      return sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
    else if (gdimension == 2)
      return sqrt(p[0]*p[0] + p[1]*p[1]);
    else
      return abs(p[0]);
  }

  // Coefficient for the kernels in 1d, 2d and 3d without h 
  // h depends of the dimension and is added in the kernel
  const double cubic_spline_sigma[3] = {4./3.,40./(7.*M_PI),8./M_PI};
  const double quintic_spline_sigma[3] = {1./40.,63./(478.*M_PI),81./(359.*M_PI)};
  const double wendland_c2_sigma[3] = {1.25,7./M_PI,21./(2.*M_PI)};
  const double wendland_c4_sigma[3] = {1.5,9./M_PI,495./(32.*M_PI)};
  const double wendland_c6_sigma[3] = {55./32.,78./(7.*M_PI),1365./(64.*M_PI)};
  const double gaussian_sigma[3] = {
      1.69260614115414981387661988500545775405505599758743902950990362363536L,
      2.86514256233641438778811727710055595794190432955152715087643977681460L,
      4.85098600188835377710867224691152783462583160522829280247414806262620L};
  const double super_gaussian_sigma[3] = {
      1.69225265632490690298438831093977954454948887037797715437044546925320L,  // ~ 3/sqrt(M_PI)
      2.86196342089351035329931251213987772907929746530058751630485064825889L,  // ~ 9/M_PI
      4.83280745997963041039733264826054665663830943853397556310561350442919L}; // ~ 27/M_PI^1.5

  // Sinc kernel is dependent on kernel index n (ranging from 3 to 12).
  const double si_b0[3] = {-1.5404568e-2,  5.2245027e-2, 2.7012593e-2};
  const double si_b1[3] = { 3.6632876e-1,  1.3090245e-1, 2.0410827e-2};
  const double si_b2[3] = {-4.6519576e-4,  1.9358485e-2, 3.7451957e-3};
  const double si_b3[3] = { 7.3658324e-2, -6.1642906e-3, 4.7013839e-2};
  double sinc_sigma[3];

  /* This function sets up the sinc kernel normalization */
  void set_sinc_kernel_normalization(const double si) {
    double sq = sqrt(si);
    double s2 = si*si;
    sinc_sigma[0] = 2.*(si_b0[0] + si_b1[0]*sq + si_b2[0]*si + si_b3[0]/sq);
    sinc_sigma[1] = 4.*(si_b0[1] + si_b1[1]*si + si_b2[1]/si + si_b3[1]/s2);
    sinc_sigma[2] = 8.*(si_b0[2] + si_b1[2]*sq + si_b2[2]*si + si_b3[2]*sq*si);
  }

/*============================================================================*/
/*   Cubic spline                                                             */
/*============================================================================*/
  /**
   * @brief      Cubic spline kernel
   * From Monaghan/92
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double 
  cubic_spline(
      const double r, 
      const double h)
  {
    double rh = 2.*r/h;
    double result = 0.;

    if (rh < 2.0) {
      if (rh < 1.0) 
        result = 1.0 - 1.5*rh*rh + .75*rh*rh*rh;
      else
        result = 0.25*(2 - rh)*(2 - rh)*(2 - rh);

      result *= cubic_spline_sigma[gdimension-1]
              / pow(h,gdimension);
    }
    return result;
  } // kernel

  /**
   * @brief      Gradient of cubic spline kernel
   * From Monaghan/92
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t 
  gradient_cubic_spline(
      const point_t & vecP, 
      const double h)
  {
    double r = vector_norm(vecP);
    double rh = 2.*r/h;

    point_t result = 0.0;
    if (rh < 2.0) {
      double dWdr;
      double sigma = 2.*cubic_spline_sigma[gdimension-1]
                   / pow(h,gdimension+1);
      if (rh < 1.0)
        dWdr = -3.0*rh + 9./4.*rh*rh;
      else
        dWdr = -.75*(2-rh)*(2-rh);

      result = vecP*sigma*dWdr/r;
    }
    return result;

  } // gradKernel 

/*============================================================================*/
/*   Gaussian                                                                 */
/*============================================================================*/
  /**
   * @brief      Gaussian kernel: W(r,h) = exp( -[3r/h]^2 ) 
   * From Liu/2010
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double 
  gaussian(
    const double r, 
    const double h)
  {
    double rh = 3.*r/h;

    double result = 0.;
    if(rh <= 3.){
      double sigma = gaussian_sigma[gdimension-1]
                   / pow(h,gdimension);
      result = sigma*exp(-rh*rh);
    }
    return result; 
  }

  /**
   * @brief      Gradient of gaussian kernel
   * From Liu/2010
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t 
  gradient_gaussian(
    const point_t & vecP,
    const double h)
  {
    double r = vector_norm(vecP);
    double rh = 3.*r/h;

    point_t result = 0.0;
    if (rh < 3.0) {
      double sigma = 3.*gaussian_sigma[gdimension-1]
                   / pow(h,gdimension+1);
      double dWdr = -2.*rh*exp(-rh*rh);
      result = vecP*sigma*dWdr/r;
    }
    return result;
  }

/*============================================================================*/
/*   Quintic spline                                                           */
/*============================================================================*/
    /**
   * @brief      Quintic spline kernel 
   * From Liu/2010
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double 
  quintic_spline(
    const double r, 
    const double h)
  {
    double rh = 3.*r/h;

    double result = 0.;
    if(rh < 3.) {
      result = pow(3-rh,5);
      if (rh < 2.)
        result += -6*pow(2-rh,5);

      if (rh < 1.)
        result += 15*pow(1-rh,5);

      double sigma = quintic_spline_sigma[gdimension-1]
                   / pow(h,gdimension);
      result *= sigma;
    }
    return result;
  }

  /**
   * @brief      Gradient of quintic spline kernel
   * From Liu/2010
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t 
  gradient_quintic_spline(
    const point_t & vecP,
    const double h)
  {
    const double r = vector_norm(vecP);
    double rh = 3.*r/h;

    point_t result = 0.0;
    if (rh < 3.) {
      double sigma = 3.*quintic_spline_sigma[gdimension-1]
                   / pow(h,gdimension+1);
      double dWdr = -5.*pow(3-rh,4);
      if(rh < 2.)
        dWdr += 30.*pow(2-rh,4);

      if(rh < 1.)
        dWdr += -75.*pow(1-rh,4);

      result = vecP*sigma*dWdr/r;
    }

    return result; 
  }

/*============================================================================*/
/*   Wendland C2                                                              */
/*============================================================================*/
  /**
   * @brief      Wendland C2-continuous kernel
   *             Reference: Dehnen & Aly (2012) MNRAS 425(2)
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double wendland_c2_1d(const double r, const double h) {
    double rh = r/h;
    double result = 0.;

    if(rh < 1.0) {
      double rh2 = (1 - rh)*(1 - rh);
      double sigma = wendland_c2_sigma[0]/h;
      result = sigma*rh2*(1 - rh)*(3*rh + 1);
    }
    return result; 
  }

  double wendland_c2_23d(const double r, const double h) {
    double rh = r/h;
    double result = 0.;
    
    if(rh < 1.0) {
      double hd = h*h;
      if (gdimension == 3) 
        hd *= h;

      double rh2 = (1 - rh)*(1 - rh);
      double sigma = wendland_c2_sigma[gdimension-1] / hd;
      result = sigma*rh2*rh2*(4*rh + 1);
    }
    return result; 
  }

  /**
   * @brief      Gradient of Wendland kernel
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t gradient_wendland_c2_1d(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = r/h;
    point_t result = 0.0;

    if(rh < 1.0) {
      double rh2 = (1 - rh)*(1 - rh);
      double sigma = wendland_c2_sigma[0]/(h*h);
      double dWdr = -12.*rh*rh2;
      result = vecP*sigma*dWdr/r; 
    }

    return result; 
  }

  point_t gradient_wendland_c2_23d(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = r/h;
    point_t result = 0.0;

    if(rh < 1.0) {
      double hd1 = h*h*h;
      if (gdimension == 3) 
        hd1 *= h;

      double rh2 = (1 - rh)*(1 - rh);
      double sigma = 2.*wendland_c2_sigma[gdimension-1]/hd1;
      double dWdr = -10.*rh*rh2*(1 - rh);
      result = vecP*sigma*dWdr/r; 
    }
    return result; 
  }

/*============================================================================*/
/*   Wendland C4                                                              */
/*============================================================================*/
  /**
   * @brief      Wendland C4-continuous kernel
   *             Reference: Dehnen & Aly (2012) MNRAS 425(2)
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double wendland_c4_1d(const double r, const double h) {
    double rh = r/h;
    double result = 0.;

    if(rh < 1.0) {
      double rh2 = (1 - rh)*(1 - rh);
      double rh3 = rh2*(1 - rh);
      double sigma = wendland_c4_sigma[0]/h;
      result = sigma*rh3*rh2*(1 + rh*(5 + rh*8));
    }
    return result; 
  }

  double wendland_c4_23d(const double r, const double h) {
    double rh = r/h;
    double result = 0.;

    if(rh < 1.0) {
      double hd = h*h;
      if (gdimension == 3) 
        hd *= h;

      double rh1 = 1 - rh;
      double rh2 = rh1*rh1;
      double rh6 = rh2*rh2*rh2;
      double sigma = wendland_c4_sigma[gdimension-1]/hd;
      result = sigma*rh6*(1 + rh*(6 + rh*35/3));
    }
    return result; 
  }

  /**
   * @brief      Gradient of the Wendland-C4 kernel
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t gradient_wendland_c4_1d(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = r/h;
    point_t result = 0.0;

    if(rh < 1.0) {
      double rh2 = (1 - rh)*(1 - rh);
      double sigma = 14.*wendland_c4_sigma[0]/(h*h);
      double dWdr = -rh*rh2*rh2*(1 + 4.*rh);

      result = vecP*sigma*dWdr/r; 
    }
    return result; 
  }


  point_t gradient_wendland_c4_23d(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = r/h;
    point_t result = 0.0;

    if(rh < 1.0) {
      double hd1 = h*h*h;
      if (gdimension==3) 
        hd1*=h;

      double rh2 = (1 - rh)*(1 - rh);
      double rh3 = rh2*(1 - rh);
      double sigma = 14.*wendland_c4_sigma[gdimension-1]/hd1;
      double dWdr = -4./3.*rh*rh3*rh2*(1 + 5.*rh);

      result = vecP*sigma*dWdr/r; 
    }
    return result; 
  }


/*============================================================================*/
/*   Wendland C6                                                              */
/*============================================================================*/
  /**
   * @brief      Wendland C6-continuous kernel
   *             Reference: Dehnen & Aly (2012) MNRAS 425(2)
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double wendland_c6_1d(const double r, const double h) {
    double rh = r/h;
    double result = 0.;

    if(rh < 1.0) {
      double rh2 = (1 - rh)*(1 - rh);
      double rh3 = rh2*(1 - rh);
      double rh4 = rh2*rh2;
      double sigma = wendland_c6_sigma[0]/h;
      result = sigma*rh3*rh4*(1 + rh*(7 + rh*(19 + rh*21)));
    }
    return result; 
  }

  double wendland_c6_23d(const double r, const double h) {
    double rh = r/h;
    double result = 0.;

    if(rh < 1.0) {
      double hd = h*h;
      if (gdimension == 3) 
        hd *= h;

      double rh2 = (1 - rh)*(1 - rh);
      double rh3 = rh2*(1 - rh);
      double rh4 = rh2*rh2;
      double sigma = wendland_c6_sigma[gdimension-1]/hd;
      result = sigma*rh4*rh4*(1 + rh*(8 + rh*(25 + rh*32)));
    }
    return result; 
  }


  /**
   * @brief      Gradient of Wendland-C6 kernel
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t gradient_wendland_c6_1d(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = r/h;
    point_t result = 0.0;

    if(rh < 1.0) {
      double rh2 = (1 - rh)*(1 - rh);
      double rh3 = rh2*(1 - rh);
      double rh4 = rh2*rh2;
      double sigma = wendland_c6_sigma[0]/h;
      double dWdr  = -6.*rh*rh3*rh3*(3 + rh*(18 + rh*35));
      result = vecP*sigma*dWdr/r; 
    }
    return result; 
  }

  point_t gradient_wendland_c6_23d(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = r/h;
    point_t result = 0.0;

    if(rh < 1.0) {
      double hd1 = h*h*h;
      if (gdimension==3) 
        hd1*=h;

      double rh2 = (1 - rh)*(1 - rh);
      double rh3 = rh2*(1 - rh);
      double rh4 = rh2*rh2;
      double sigma = wendland_c6_sigma[gdimension-1]/hd1;
      double dWdr  = -22.*rh*rh4*rh3*(1 + rh*(7 + rh*16));
      result = vecP*sigma*dWdr/r; 
    }

    return result; 
  }


/*============================================================================*/
/*   Super Gaussian                                                           */
/*============================================================================*/
  /**
   * @brief      Super Gaussian kernel
   *             Reference: Monaghan, J. J. (1992), ARAA 30:543-74
   *             Note: the kernel is negative in certain regions.
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double 
  super_gaussian(
    const double r, 
    const double h)
  {
    double rh = 3.*r/h, rh2 = rh*rh;

    double result = 0.;
    if(fabs(rh) < 3.){
      double sigma = super_gaussian_sigma[gdimension-1]
                   / pow(h,gdimension);
      result = sigma*exp(-rh2)*(gdimension/2.0 + 1 - rh2);
    }
    return result; 
  }

  /**
   * @brief      Gradient of Super Gaussian kernel
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t 
  gradient_super_gaussian(
    const point_t & vecP,
    const double h)
  {
    double r = vector_norm(vecP);
    double rh = 3.*r/h;

    point_t result = 0.0;
    if (rh < 3.0) {
      double sigma = 3.*super_gaussian_sigma[gdimension-1]
                   / pow(h,gdimension+1);
      double dWdr = exp(-rh*rh)*(2.*pow(rh,3.) - (gdimension+4.)*rh);
      result = vecP*sigma*dWdr/r;
    }
    return result;
  }

/*============================================================================*/
/*   Sinc	                                                                  */
/*============================================================================*/
  /**
   * @brief   Sinc kernel
   * From : Garcia-Senz, Cabezon et al. (2014), A&A 570, A14
   *
   * @param[in]  r     Distance between the particles 
   * @param[in]  h     Smoothing length 
   *
   * @return     Contribution from the particle 
   */
  double sinc_ker(const double r, const double h) {
    double rh = fabs(r/h), rh2;
    const double eps = 1e-24;
    const double eps_root = sqrt(eps);
    const double eps_root4 = sqrt(eps_root);

    //HL : It seems boost has sinc function as in thier special function lib 
    //     But, here, I implement anyway
    double result = 0.;
    if(rh < 1.) {
      double hd = h;
      for (int i=1; i<gdimension; i++) 
        hd *= h;

      double sinc = 1.0;
      rh *= M_PI;
      if(rh > eps_root4) {
        sinc = sin(rh)/rh;
      } else if(rh > eps) {
        rh2 = rh*rh;
        sinc += rh2*(.05*rh2 - 1.)/6.;
      }
      result = sinc_sigma[gdimension-1]/hd * pow(sinc, sph_sinc_index);
    }
    return result; 
  }

  /**
   * @brief      Gradient of sinc kernel
   *
   * @param[in]  vecP  The vector pab = pa - pb 
   * @param[in]  h     The smoothing length 
   *
   * @return     Contribution from the particle 
   */
  point_t gradient_sinc_ker(const point_t & vecP, const double h) {
    double r = vector_norm(vecP);
    double rh = fabs(r/h), rh2;
    const double eps = 1e-24;
    const double eps_root = sqrt(eps);
    const double eps_root4 = sqrt(eps_root);

    point_t result = 0.0;
    if(rh < 1.) {
      double hd = h;
      for (int i=1; i<gdimension; i++) 
        hd *= h;
      double sigma = sinc_sigma[gdimension-1]/hd;

      rh *= M_PI;
      double dWdr = 0.0;
      if(rh > eps_root4) {
        double sinc = sin(rh)/rh, cosx = cos(rh)/rh;
        dWdr = -sph_sinc_index * pow(sinc, sph_sinc_index-1) * M_PI
                               * (sinc/rh - cosx);
      } else if(rh > eps) {
        rh2 = rh*rh;
        dWdr = -sph_sinc_index * rh/3. * M_PI
                               * (1 - .5*rh2*(.2 - (sph_sinc_index-1)/18));
      }
      result = vecP*sigma*dWdr/r;
    }
    return result; 
  }

}; // kernel

#endif // _physics_kernel_h_
