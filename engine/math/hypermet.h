/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      Voigt type functions
 *
 ******************************************************************************/

#pragma once

#include <vector>
#include <iostream>
#include <numeric>
#include "polynomial.h"
#include "fit_settings.h"
#include "gaussian.h"
#include "xmlable.h"

class Hypermet : public XMLable {
public:
  Hypermet() :
    Hypermet(Gaussian(), FitSettings())
  {}

  Hypermet(Gaussian gauss, FitSettings settings);

  const FitParam& center() const {return center_;}
  const FitParam& height() const {return height_;}
  const FitParam& width() const {return width_;}
  const FitParam& Lskew_amplitude() const {return Lskew_amplitude_;}
  const FitParam& Lskew_slope() const {return Lskew_slope_;}
  const FitParam& Rskew_amplitude() const {return Rskew_amplitude_;}
  const FitParam& Rskew_slope() const {return Rskew_slope_;}
  const FitParam& tail_amplitude() const {return tail_amplitude_;}
  const FitParam& tail_slope() const {return tail_slope_;}
  const FitParam& step_amplitude() const {return step_amplitude_;}
  double chi2() const {return chi2_;}

  bool user_modified() {return user_modified_;}

  void set_center(const FitParam &ncenter);
  void set_height(const FitParam &nheight);
  void set_width(const FitParam &nwidth);
  void set_Lskew_amplitude(const FitParam &nLskew_amplitude);
  void set_Lskew_slope(const FitParam &nLskew_slope);
  void set_Rskew_amplitude(const FitParam &nRskew_amplitude);
  void set_Rskew_slope(const FitParam &nRskew_slope);
  void set_tail_amplitude(const FitParam &ntail_amplitude);
  void set_tail_slope(const FitParam &ntail_slope);
  void set_step_amplitude(const FitParam &nstep_amplitude);

  void set_center(const UncertainDouble &ncenter);
  void set_height(const UncertainDouble &nheight);
  void set_width(const UncertainDouble &nwidth);
  void set_Lskew_amplitude(const UncertainDouble &nLskew_amplitude);
  void set_Lskew_slope(const UncertainDouble &nLskew_slope);
  void set_Rskew_amplitude(const UncertainDouble &nRskew_amplitude);
  void set_Rskew_slope(const UncertainDouble &nRskew_slope);
  void set_tail_amplitude(const UncertainDouble &ntail_amplitude);
  void set_tail_slope(const UncertainDouble &ntail_slope);
  void set_step_amplitude(const UncertainDouble &nstep_amplitude);

  void set_chi2(double);

  void constrain_center(double min, double max);
  void constrain_height(double min, double max);
  void constrain_width(double min, double max);

  std::string to_string() const;
  double eval_peak(double) const;
  double eval_step_tail(double) const;
  std::vector<double> peak(std::vector<double> x) const;
  std::vector<double> step_tail(std::vector<double> x) const;
  UncertainDouble area() const;
  bool gaussian_only() const;
  Gaussian gaussian() const;


  //XMLable dependency
  void to_xml(pugi::xml_node &node) const override;
  void from_xml(const pugi::xml_node &node) override;
  std::string xml_element_name() const override {return "Hypermet";}

private:
  FitParam center_, height_, width_,
  Lskew_amplitude_, Lskew_slope_,
  Rskew_amplitude_, Rskew_slope_,
  tail_amplitude_, tail_slope_,
  step_amplitude_;

  double chi2_ {0};
  bool user_modified_ {false};

};
