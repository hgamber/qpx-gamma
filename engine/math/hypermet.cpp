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
 *      Hypermet function ensemble
 *
 ******************************************************************************/

#include "hypermet.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <boost/lexical_cast.hpp>

#include "fityk_util.h"
#include "custom_logger.h"
#include "qpx_util.h"

#ifdef FITTER_ROOT_ENABLED
#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH1.h"
#endif


void Hypermet::set_center(const FitParam &ncenter)
{
  center_ = ncenter; //set without changing name?!
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_height(const FitParam &nheight)
{
  height_ = nheight;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_width(const FitParam &nwidth)
{
  width_ = nwidth;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Lskew_amplitude(const FitParam &nLskew_amplitude)
{
  Lskew_amplitude_ = nLskew_amplitude;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Lskew_slope(const FitParam &nLskew_slope)
{
  Lskew_slope_ = nLskew_slope;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Rskew_amplitude(const FitParam &nRskew_amplitude)
{
  Rskew_amplitude_ = nRskew_amplitude;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_Rskew_slope(const FitParam &nRskew_slope)
{
  Rskew_slope_ = nRskew_slope;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_tail_amplitude(const FitParam &ntail_amplitude)
{
  tail_amplitude_ = ntail_amplitude;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_tail_slope(const FitParam &ntail_slope)
{
  tail_slope_ = ntail_slope;
  user_modified_ = true;
  rsq_ = 0;
}

void Hypermet::set_step_amplitude(const FitParam &nstep_amplitude)
{
  step_amplitude_ = nstep_amplitude;
  user_modified_ = true;
  rsq_ = 0;
}



std::string Hypermet::fityk_definition() {

  return "define Hypermet(c, h, w,"
         "lskew_h, lskew_s,"
         "rskew_h, rskew_s,"
         "tail_h, tail_s,"
         "step_h) = "
         "h*("
         "   exp(-(xc/w)^2)"
         " + 0.5 * ("
         "   lskew_h*exp((0.5*w/lskew_s)^2 + (xc/lskew_s)) * erfc((0.5*w/lskew_s) + xc/w)"
         " + rskew_h*exp((0.5*w/rskew_s)^2 - (xc/rskew_s)) * erfc((0.5*w/rskew_s) - xc/w)"
         " + tail_h *exp((0.5*w/tail_s )^2 + (xc/tail_s )) * erfc((0.5*w/tail_s ) + xc/w)"
         " + step_h * erfc(xc/w)"
         " ) )"
         " where xc=(x-c)";
}

bool Hypermet::extract_params(fityk::Fityk* f, fityk::Func* func) {
  try {
    if (func->get_template_name() != "Hypermet")
      return false;
    center_.extract(f, func);
    height_.extract(f, func);
    width_.extract(f, func);
    Lskew_amplitude_.extract(f, func);
    Lskew_slope_.extract(f, func);
    Rskew_amplitude_.extract(f, func);
    Rskew_slope_.extract(f, func);
    tail_amplitude_.extract(f, func);
    tail_slope_.extract(f, func);
    step_amplitude_.extract(f, func);
  }
  catch ( ... ) {
    DBG << "Hypermet could not extract parameters from Fityk";
    return false;
  }
  return true;
}

std::string Hypermet::to_string() const {
  std::string ret = "Hypermet ";
  ret += "   area=" + area().to_string()
      + "   rsq=" + boost::lexical_cast<std::string>(rsq_) + "    where:\n";

  ret += "     " + center_.to_string() + "\n";
  ret += "     " + height_.to_string() + "\n";
  ret += "     " + width_.to_string() + "\n";
  ret += "     " + Lskew_amplitude_.to_string() + "\n";
  ret += "     " + Lskew_slope_.to_string() + "\n";
  ret += "     " + Rskew_amplitude_.to_string() + "\n";
  ret += "     " + Rskew_slope_.to_string() + "\n";
  ret += "     " + tail_amplitude_.to_string() + "\n";
  ret += "     " + tail_slope_.to_string() + "\n";
  ret += "     " + step_amplitude_.to_string();

  return ret;
}

Hypermet::Hypermet(Gaussian gauss, FitSettings settings) :
  height_("h", gauss.height_.value.value()),
  center_("c", gauss.center_.value.value()),
  width_("w", gauss.hwhm_.value.value() / sqrt(log(2))),
  Lskew_amplitude_(settings.Lskew_amplitude), Lskew_slope_(settings.Lskew_slope),
  Rskew_amplitude_(settings.Rskew_amplitude), Rskew_slope_(settings.Rskew_slope),
  tail_amplitude_(settings.tail_amplitude), tail_slope_(settings.tail_slope),
  step_amplitude_(settings.step_amplitude),
  rsq_(0),
  user_modified_(false)
{
  if (settings.gaussian_only)
  {
    Lskew_amplitude_.enabled = false;
    Rskew_amplitude_.enabled = false;
    tail_amplitude_.enabled = false;
    step_amplitude_.enabled = false;
  }
}

Gaussian Hypermet::gaussian() const
{
  Gaussian ret;
  ret.height_ = height_;
  ret.center_ = center_;
  ret.hwhm_ = width_;
  ret.hwhm_.value *= sqrt(log(2));
  ret.hwhm_.lbound *= sqrt(log(2));
  ret.hwhm_.ubound *= sqrt(log(2));
  ret.rsq_ = rsq_;
  return ret;
}

void Hypermet::fit(const std::vector<double> &x, const std::vector<double> &y,
              Gaussian gauss, FitSettings settings)
{
  *this = Hypermet(gauss, settings);
#ifdef FITTER_ROOT_ENABLED
  fit_root(x, y);
#else
  fit_fityk(x, y);
#endif
}


std::vector<Hypermet> Hypermet::fit_multi(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Hypermet> old,
                                          Polynomial &background,
                                          FitSettings settings)
{
#ifdef FITTER_ROOT_ENABLED
  bool use_w_common = (settings.width_common &&
                       settings.cali_fwhm_.valid() &&
                       settings.cali_nrg_.valid());

  if (use_w_common)
    return fit_multi_root_commonw(x,y, old, background, settings);
  else
    return fit_multi_root(x,y, old, background, settings);
#else
  return fit_multi_fityk(x,y, old, background, settings);
#endif
}


void Hypermet::fit_fityk(const std::vector<double> &x, const std::vector<double> &y)
{
  std::vector<double> sigma;
  for (auto &q : y) {
    sigma.push_back(sqrt(q));
  }
  //  sigma.resize(x.size(), 1);

  bool success = true;

  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    //        f->execute("set fitting_method = mpfit");
    //    f->execute("set fitting_method = nlopt_nm");
    //    f->execute("set fitting_method = nlopt_bobyqa");
    f->execute("set fitting_method = nlopt_lbfgs");
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet failed to define";
  }

  double lateral_slack = (x[x.size() -1]  - x[0]) / 5.0;

  center_.lbound = center_.value.value() - lateral_slack;
  center_.ubound = center_.value.value() + lateral_slack;

  height_.lbound = height_.value.value() * 0.003;
  height_.ubound = height_.value.value() * 3000;

  width_.lbound = width_.value.value() * 0.7;
  width_.ubound = width_.value.value() * 1.3;

  std::string initial_c = "$c = " + center_.def_bounds();
  std::string initial_h = "$h = " + height_.def_bounds();
  std::string initial_w = "$w = " + width_.def_bounds();

  std::string initial_lsh = "$lsh = " + Lskew_amplitude_.def_bounds();
  std::string initial_lss = "$lss = " + Lskew_slope_.def_bounds();

  std::string initial_rsh = "$rsh = " + Rskew_amplitude_.def_bounds();
  std::string initial_rss = "$rss = " + Rskew_slope_.def_bounds();

  //  std::string initial_llh = FitykUtil::var_def("llh", 0.0000, 0, 0.015);
  //  std::string initial_lls = FitykUtil::var_def("lls", 2.5, 2.5, 50);

  std::string initial_step = "$step = " + step_amplitude_.def_bounds();

  std::string initial = "F += Hypermet($c,$h,$w,$lsh,$lss,$rsh,$rss,$step)";

  //DBG << "Hypermet initial c=" << center_ << " h=" << height_ << " w=" << width_;

  try {
    f->execute(initial_h);
    f->execute(initial_c);
    f->execute(initial_w);
    f->execute(initial_lsh);
    f->execute(initial_lss);
    f->execute(initial_rsh);
    f->execute(initial_rss);
    //    f->execute(initial_llh);
    //    f->execute(initial_lls);
    f->execute(initial_step);
    f->execute(initial);

  } catch ( ... ) {
    success = false;
    DBG << "Hypermet: failed to set up initial";
  }

  try {
    f->execute("fit");
  }
  catch ( ... ) {
    DBG << "Hypermet could not fit";
    success = false;
  }

  if (success) {
    f->execute("set fitting_method = levenberg_marquardt");
    std::vector<fityk::Func*> fns = f->all_functions();
    if (fns.size()) {
      fityk::Func* func = fns.back();
      extract_params(f, func); //discard return value
    }
    rsq_ = f->get_rsquared(0);
  }


  delete f;
}

std::vector<Hypermet> Hypermet::fit_multi_fityk(const std::vector<double> &x,
                                          const std::vector<double> &y,
                                          std::vector<Hypermet> old,
                                          Polynomial &background,
                                          FitSettings settings)
{
  if (old.empty())
    return old;

  std::vector<double> sigma;
  for (auto &q : y) {
    sigma.push_back(sqrt(q));
  }

  bool success = true;

  fityk::Fityk *f = new fityk::Fityk;
  f->redir_messages(NULL);
  f->load_data(0, x, y, sigma);

  try {
    f->execute("set fitting_method = nlopt_lbfgs");
    //    f->execute("set fitting_method = mpfit");
    //    f->execute("set fitting_method = levenberg_marquardt");
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet multifit failed to set fitter";
  }

  try {
    f->execute(fityk_definition());
    f->execute(background.fityk_definition());
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet multifit failed to define";
  }


  bool use_w_common = (settings.width_common && settings.cali_fwhm_.valid() && settings.cali_nrg_.valid());

  if (use_w_common) {
    FitParam w_common = settings.width_common_bounds;
    UncertainDouble centers_avg;
    for (auto &p : old)
      centers_avg += p.center_.value;
    centers_avg /= old.size();

    double nrg = settings.cali_nrg_.transform(centers_avg.value());
    double fwhm_expected = settings.cali_fwhm_.transform(nrg);
    double L = settings.cali_nrg_.inverse_transform(nrg - fwhm_expected/2);
    double R = settings.cali_nrg_.inverse_transform(nrg + fwhm_expected/2);
    w_common.value.setValue((R - L) / (2* sqrt(log(2))));

    w_common.lbound = w_common.value.value() * w_common.lbound;
    w_common.ubound = w_common.value.value() * w_common.ubound;

    try {
      f->execute(w_common.def_var());
    } catch ( ... ) {
      success = false;
      DBG << "Hypermet: multifit failed to define w_common";
    }
  }


  try {
    background.add_self(f);
  } catch ( ... ) {
    success = false;
    DBG << "Hypermet: multifit failed to set up common background";
  }

  bool can_uncert = true;

  int i=0;
  for (auto &o : old) {

    if (!use_w_common) {
      double width_expected = o.width_.value.value();

      if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid()) {
        double fwhm_expected = settings.cali_fwhm_.transform(settings.cali_nrg_.transform(o.center_.value.value()));
        double L = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.value.value()) - fwhm_expected/2);
        double R = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.value.value()) + fwhm_expected/2);
        width_expected = (R - L) / (2* sqrt(log(2)));
      }

      o.width_.lbound = width_expected * settings.width_common_bounds.lbound;
      o.width_.ubound = width_expected * settings.width_common_bounds.ubound;

      if ((o.width_.value.value() > o.width_.lbound) && (o.width_.value.value() < o.width_.ubound))
        width_expected = o.width_.value.value();
      o.width_.value.setValue(width_expected);
    }

    o.height_.lbound = o.height_.value.value() * 1e-5;
    o.height_.ubound = o.height_.value.value() * 1e5;

    double lateral_slack = settings.lateral_slack * o.width_.value.value() * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.value.value() - lateral_slack;
    o.center_.ubound = o.center_.value.value() + lateral_slack;

    std::string initial = "F += Hypermet(" +
        o.center_.fityk_name(i) + "," +
        o.height_.fityk_name(i) + "," +
        o.width_.fityk_name(use_w_common ? -1 : i)  + "," +
        o.Lskew_amplitude_.fityk_name(i)  + "," +
        o.Lskew_slope_.fityk_name(i)  + "," +
        o.Rskew_amplitude_.fityk_name(i)  + "," +
        o.Rskew_slope_.fityk_name(i)  + "," +
        o.tail_amplitude_.fityk_name(i)  + "," +
        o.tail_slope_.fityk_name(i)  + "," +
        o.step_amplitude_.fityk_name(i)
        + ")";

    try {
      f->execute(o.center_.def_var(i));
      f->execute(o.height_.def_var(i));
      if (!use_w_common)
        f->execute(o.width_.def_var(i));
      f->execute(o.Lskew_amplitude_.enforce_policy().def_var(i));
      f->execute(o.Lskew_slope_.def_var(i));
      f->execute(o.Rskew_amplitude_.enforce_policy().def_var(i));
      f->execute(o.Rskew_slope_.def_var(i));
      f->execute(o.tail_amplitude_.enforce_policy().def_var(i));
      f->execute(o.tail_slope_.def_var(i));
      f->execute(o.step_amplitude_.enforce_policy().def_var(i));

      f->execute(initial);
    }
    catch ( ... ) {
      DBG << "Hypermet multifit failed to set up initial locals for peak " << i;
      success = false;
    }

    if (!o.Lskew_amplitude_.enabled || !o.Rskew_amplitude_.enabled ||
        !o.tail_amplitude_.enabled || !o.step_amplitude_.enabled)
      can_uncert = false;

    i++;
  }
  try {
    f->execute("fit " + boost::lexical_cast<std::string>(settings.fitter_max_iter));
    //    if (can_uncert) {
    //      f->execute("set fitting_method = mpfit");
    ////    f->execute("set fitting_method = levenberg_marquardt");
    //      f->execute("fit");
    ////    f->execute("fit " + boost::lexical_cast<std::string>(settings.fitter_max_iter));
    ////    DBG << "refit with mp done";
    //    }
  }
  catch ( ... ) {
    DBG << "Hypermet multifit failed to fit";
    success = false;
  }

  if (success) {
    //      try {
    //    f->execute("info errors");
    //    f->execute("info state > nl.fit");

    //    } catch ( ... ) {
    //      DBG << "Hypermet multifit failed to do error thing";
    //      success = false;
    //    }

    std::vector<fityk::Func*> fns = f->all_functions();
    int i = 0;
    for (auto &q : fns) {
      if (q->get_template_name() == "Hypermet") {
        old[i].extract_params(f, q);
        old[i].rsq_ = f->get_rsquared(0);
        i++;
      } else if (q->get_template_name() == "Polynomial") {
        background.extract_params(f, q);
      }
    }


    //        f->execute("info errors");

    //    f->execute("info state > lm.fit");
  }

  delete f;

  if (!success)
    old.clear();
  return old;
}

double Hypermet::eval_peak(double x) const {
  if (width_.value.value() == 0)
    return 0;

  double xc = x - center_.value.value();

  double gaussian = exp(- pow(xc/width_.value.value(), 2) );

  double left_short = 0;
  if (Lskew_amplitude_.enabled) {
    double lexp = exp(pow(0.5*width_.value.value()/Lskew_slope_.value.value(), 2) + xc/Lskew_slope_.value.value());
    if ((Lskew_slope_.value.value() != 0) && !std::isinf(lexp))
      left_short = Lskew_amplitude_.value.value() * lexp * erfc( 0.5*width_.value.value()/Lskew_slope_.value.value() + xc/width_.value.value());
  }

  double right_short = 0;
  if (Rskew_amplitude_.enabled) {
    double rexp = exp(pow(0.5*width_.value.value()/Rskew_slope_.value.value(), 2) - xc/Rskew_slope_.value.value());
    if ((Rskew_slope_.value.value() != 0) && !std::isinf(rexp))
      right_short = Rskew_amplitude_.value.value() * rexp * erfc( 0.5*width_.value.value()/Rskew_slope_.value.value()  - xc/width_.value.value());
  }

  double ret = height_.value.value() * (gaussian + 0.5 * (left_short + right_short) );

  return ret;
}

double Hypermet::eval_step_tail(double x) const {
  if (width_.value.value() == 0)
    return 0;

  double xc = x - center_.value.value();

  double step = 0;
  if (step_amplitude_.enabled)
    step = step_amplitude_.value.value() * erfc( xc/width_.value.value() );

  double tail = 0;
  if (tail_amplitude_.enabled) {
    double lexp = exp(pow(0.5*width_.value.value()/tail_slope_.value.value(), 2) + xc/tail_slope_.value.value());
    if ((tail_slope_.value.value() != 0) && !std::isinf(lexp))
      tail = tail_amplitude_.value.value() * lexp * erfc( 0.5*width_.value.value()/tail_slope_.value.value() + xc/width_.value.value());
  }

  return height_.value.value() * 0.5 * (step + tail);
}

std::vector<double> Hypermet::peak(std::vector<double> x) const {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(eval_peak(q));
  return y;
}

std::vector<double> Hypermet::step_tail(std::vector<double> x) const {
  std::vector<double> y;
  for (auto &q : x)
    y.push_back(eval_step_tail(q));
  return y;
}

UncertainDouble Hypermet::area() const {
  UncertainDouble ret;
  ret = height_.value * width_.value * UncertainDouble::from_double(sqrt(M_PI),0) *
      ( UncertainDouble::from_int(1,0) +
       (Lskew_amplitude_.enabled ? Lskew_amplitude_.value * width_.value * Lskew_slope_.value : UncertainDouble::from_int(0,0) ) +
       (Rskew_amplitude_.enabled ? Rskew_amplitude_.value * width_.value * Rskew_slope_.value : UncertainDouble::from_int(0,0) )
      );
  ret.setUncertainty(std::numeric_limits<double>::infinity());
  return ret;
}

bool Hypermet::gaussian_only() const
{
  return
      !(step_amplitude_.enabled
        || tail_amplitude_.enabled
        || Lskew_amplitude_.enabled
        || Rskew_amplitude_.enabled);
}

void Hypermet::to_xml(pugi::xml_node &root) const {
  pugi::xml_node node = root.append_child(this->xml_element_name().c_str());

  node.append_attribute("rsq").set_value(to_max_precision(rsq_).c_str());
  center_.to_xml(node);
  height_.to_xml(node);
  width_.to_xml(node);
  Lskew_amplitude_.to_xml(node);
  Lskew_slope_.to_xml(node);
  Rskew_amplitude_.to_xml(node);
  Rskew_slope_.to_xml(node);
  tail_amplitude_.to_xml(node);
  tail_slope_.to_xml(node);
  step_amplitude_.to_xml(node);
}


void Hypermet::from_xml(const pugi::xml_node &node) {
  rsq_ = node.attribute("rsq").as_double(0);
  for (auto &q : node.children()) {
    FitParam param;
    if (std::string(q.name()) == param.xml_element_name()) {
      param.from_xml(q);
      if (param.name() == center_.name())
        center_ = param;
      else if (param.name() == height_.name())
        height_ = param;
      else if (param.name() == width_.name())
        width_ = param;
      else if (param.name() == step_amplitude_.name())
        step_amplitude_ = param;
      else if (param.name() == tail_amplitude_.name())
        tail_amplitude_ = param;
      else if (param.name() == tail_slope_.name())
        tail_slope_ = param;
      else if (param.name() == Lskew_amplitude_.name())
        Lskew_amplitude_ = param;
      else if (param.name() == Lskew_slope_.name())
        Lskew_slope_ = param;
      else if (param.name() == Rskew_amplitude_.name())
        Rskew_amplitude_ = param;
      else if (param.name() == Rskew_slope_.name())
        Rskew_slope_ = param;
    }
  }
}

#ifdef FITTER_ROOT_ENABLED

std::string Hypermet::root_definition(uint16_t start)
{
  return root_definition(start, start+1);
}

std::string Hypermet::root_definition(uint16_t width, uint16_t i)
{
  std::string h = "[" + std::to_string(i) + "]";
  std::string w = "[" + std::to_string(width) + "]";
  std::string xc = "(x-[" + std::to_string(i+1) + "])";
  std::string xcw = xc + "/" + w;
  std::string lskewh = "[" + std::to_string(i+2) + "]";
  std::string lskews = "/[" + std::to_string(i+3) + "]";
  std::string rskewh = "[" + std::to_string(i+4) + "]";
  std::string rskews = "/[" + std::to_string(i+5) + "]";
  std::string tailh = "[" + std::to_string(i+6) + "]";
  std::string tails = "/[" + std::to_string(i+7) + "]";
  std::string steph = "[" + std::to_string(i+8) + "]";

  return h + "*("
         "   TMath::Exp(-(" + xcw + ")^2)"
         " + 0.5 * ("
         "   " + lskewh + "*TMath::Exp((0.5*"+ w+lskews +")^2 + ("+ xc+lskews +"))*TMath::Erfc((0.5*"+ w+lskews +") + "+ xcw +")"
         " + " + rskewh + "*TMath::Exp((0.5*"+ w+rskews +")^2 - ("+ xc+rskews +"))*TMath::Erfc((0.5*"+ w+rskews +") - "+ xcw +")"
         " + " + tailh  + "*TMath::Exp((0.5*"+ w+tails  +")^2 + ("+ xc+tails  +"))*TMath::Erfc((0.5*"+ w+tails  +") + "+ xcw +")"
         " + " + steph  + "*TMath::Erfc(" + xc +"/" + w + ")"
         " ) )";
}

void Hypermet::set_params(TF1* f, uint16_t start) const
{
  set_params(f, start, start+1);
}

void Hypermet::set_params(TF1* f, uint16_t width, uint16_t others_start) const
{
  width_.set(f, width);
  height_.set(f, others_start);
  center_.set(f, others_start+1);
  Lskew_amplitude_.set(f, others_start+2);
  Lskew_slope_.set(f, others_start+3);
  Rskew_amplitude_.set(f, others_start+4);
  Rskew_slope_.set(f, others_start+5);
  tail_amplitude_.set(f, others_start+6);
  tail_slope_.set(f, others_start+7);
  step_amplitude_.set(f, others_start+8);
}

void Hypermet::get_params(TF1* f, uint16_t start)
{
  get_params(f, start, start+1);
}

void Hypermet::get_params(TF1* f, uint16_t width, uint16_t others_start)
{
  width_.get(f, width);
  height_.get(f, others_start);
  center_.get(f, others_start+1);
  Lskew_amplitude_.get(f, others_start+2);
  Lskew_slope_.get(f, others_start+3);
  Rskew_amplitude_.get(f, others_start+4);
  Rskew_slope_.get(f, others_start+5);
  tail_amplitude_.get(f, others_start+6);
  tail_slope_.get(f, others_start+7);
  step_amplitude_.get(f, others_start+8);
}

void Hypermet::fit_root(const std::vector<double> &x, const std::vector<double> &y)
{
  if ((x.size() < 1) || (x.size() != y.size()))
    return;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }

  double lateral_slack = (x.back() - x.front()) / 5.0;

  center_.lbound = center_.value.value() - lateral_slack;
  center_.ubound = center_.value.value() + lateral_slack;

  height_.lbound = height_.value.value() * 0.003;
  height_.ubound = height_.value.value() * 3000;

  width_.lbound = width_.value.value() * 0.7;
  width_.ubound = width_.value.value() * 1.3;


  TF1* f1 = new TF1("f1", this->root_definition().c_str());

  set_params(f1);

    h1->Fit("f1", "N");
//  h1->Fit("f1", "QN");
  //  h1->Fit("f1", "EMN");

  get_params(f1);
  rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();
}

std::vector<Hypermet> Hypermet::fit_multi_root(const std::vector<double> &x,
                                               const std::vector<double> &y,
                                               std::vector<Hypermet> old,
                                               Polynomial &background,
                                               FitSettings settings)
{
  if (old.empty())
    return old;

  if ((x.size() < 1) || (x.size() != y.size()))
    return old;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }

  std::string definition = background.root_definition();
  uint16_t backgroundparams = background.coeffs().size();
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 3;
    definition += "+" + Gaussian().root_definition(num);
  }

//  DBG << "Definition = " << definition;

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    double width_expected = o.width_.value.value();

    if (settings.cali_fwhm_.valid() && settings.cali_nrg_.valid()) {
      double fwhm_expected = settings.cali_fwhm_.transform(settings.cali_nrg_.transform(o.center_.value.value()));
      double L = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.value.value()) - fwhm_expected/2);
      double R = settings.cali_nrg_.inverse_transform(settings.cali_nrg_.transform(o.center_.value.value()) + fwhm_expected/2);
      width_expected = (R - L) / (2* sqrt(log(2)));
    }

    o.width_.lbound = width_expected * settings.width_common_bounds.lbound;
    o.width_.ubound = width_expected * settings.width_common_bounds.ubound;

    if ((o.width_.value.value() > o.width_.lbound) && (o.width_.value.value() < o.width_.ubound))
      width_expected = o.width_.value.value();
    o.width_.value.setValue(width_expected);

    o.height_.lbound = o.height_.value.value() * 1e-5;
    o.height_.ubound = o.height_.value.value() * 1e5;

    double lateral_slack = settings.lateral_slack * o.width_.value.value() * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.value.value() - lateral_slack;
    o.center_.ubound = o.center_.value.value() + lateral_slack;
  }


  background.set_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    old[i].set_params(f1, num);
  }

    h1->Fit("f1", "N");
//  h1->Fit("f1", "QN");
  //  h1->Fit("f1", "EMN");

  background.get_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = backgroundparams + i * 10;
    old[i].get_params(f1, num);
    old[i].rsq_ = f1->GetChisquare();
  }
  background.rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();

  return old;
}

std::vector<Hypermet> Hypermet::fit_multi_root_commonw(const std::vector<double> &x,
                                                       const std::vector<double> &y,
                                                       std::vector<Hypermet> old,
                                                       Polynomial &background,
                                                       FitSettings settings)
{
  if (old.empty())
    return old;

  if ((x.size() < 1) || (x.size() != y.size()))
    return old;

  TH1D* h1 = new TH1D("h1", "h1", x.size(), x.front(), x.back());
  int i=1;
  for (const auto &h : y)
  {
    h1->SetBinContent(i, h);
    h1->SetBinError(i, sqrt(h));
    i++;
  }

  FitParam w_common = settings.width_common_bounds;
  UncertainDouble centers_avg;
  for (auto &p : old)
    centers_avg += p.center_.value;
  centers_avg /= old.size();

  double nrg = settings.cali_nrg_.transform(centers_avg.value());
  double fwhm_expected = settings.cali_fwhm_.transform(nrg);
  double L = settings.cali_nrg_.inverse_transform(nrg - fwhm_expected/2);
  double R = settings.cali_nrg_.inverse_transform(nrg + fwhm_expected/2);
  w_common.value.setValue((R - L) / (2* sqrt(log(2))));
  w_common.lbound = w_common.value.value() * w_common.lbound;
  w_common.ubound = w_common.value.value() * w_common.ubound;

  for (auto g : old)
    g.width_ = w_common;

  std::string definition = background.root_definition();
  uint16_t backgroundparams = background.coeffs().size();
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 2;
    definition += "+" + Gaussian().root_definition(num, num+1, backgroundparams);
  }

//  DBG << "Definition = " << definition;

  TF1* f1 = new TF1("f1", definition.c_str());

  for (auto &o : old)
  {
    o.height_.lbound = o.height_.value.value() * 1e-5;
    o.height_.ubound = o.height_.value.value() * 1e5;

    double lateral_slack = settings.lateral_slack * o.width_.value.value() * 2 * sqrt(log(2));
    o.center_.lbound = o.center_.value.value() - lateral_slack;
    o.center_.ubound = o.center_.value.value() + lateral_slack;
  }

  background.set_params(f1, 0);
  w_common.set(f1, backgroundparams);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 9;
    old[i].set_params(f1, backgroundparams, num);
  }

    h1->Fit("f1", "N");
//  h1->Fit("f1", "QN");
  //  h1->Fit("f1", "EMN");

  background.get_params(f1, 0);
  for (size_t i=0; i < old.size(); ++i)
  {
    uint16_t num = 1 + backgroundparams + i * 9;
    old[i].get_params(f1, backgroundparams, num);
    old[i].rsq_ = f1->GetChisquare();
  }
  background.rsq_ = f1->GetChisquare();

  f1->Delete();
  h1->Delete();

  return old;
}

#endif
