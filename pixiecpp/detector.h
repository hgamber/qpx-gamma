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
 *      Pixie::Calibration defines calibration with units and math model
 *      Pixie::Detector defines detector with name, calibration, DSP parameters
 *
 ******************************************************************************/


#ifndef PIXIE_DETECTOR
#define PIXIE_DETECTOR

#include <vector>
#include <string>
#include <boost/date_time.hpp>
#include "xmlable.h"

namespace Pixie {

enum class CalibrationModel : int {none = 0, polynomial = 1};

class Calibration : public XMLable {
 public:
  Calibration();
  Calibration(uint16_t bits): Calibration() {bits_ = bits;}
  void to_xml(tinyxml2::XMLPrinter&) const;
  void from_xml(tinyxml2::XMLElement*);
  std::string xml_element_name() const override {return "Calibration";}

  bool shallow_equals(const Calibration& other) const {return (bits_ == other.bits_);}
  bool operator!= (const Calibration& other) const {return !operator==(other);}
  bool operator== (const Calibration& other) const {
    if (calib_date_ != other.calib_date_) return false;
    if (type_ != other.type_) return false;
    if (units_ != other.units_) return false;
    if (model_ != other.model_) return false;
    if (coefficients_ != other.coefficients_) return false;
    if (bits_ != other.bits_) return false;
    return true;
  }
  
  double transform(double) const;
  double transform(double, uint16_t) const;
  std::vector<double> transform(std::vector<double>) const;
  std::string coef_to_string() const;
  void coef_from_string(std::string);

  boost::posix_time::ptime calib_date_;
  std::string type_, units_;
  uint16_t bits_;
  CalibrationModel model_;
  std::vector<double> coefficients_;
 private:
  double polynomial(double) const;
};

class Detector : public XMLable {
 public:
  Detector() : energy_calibrations_("Calibrations"),
                       name_("none"), type_("none") {}
  Detector(tinyxml2::XMLElement* el) : Detector() {from_xml(el);}
  Detector(std::string name) : Detector() {name_ = name;}

  std::string xml_element_name() const override {return "Detector";}
  
  bool shallow_equals(const Detector& other) const {return (name_ == other.name_);}
  bool operator!= (const Detector& other) const {return !operator==(other);}
  bool operator== (const Detector& other) const {
    if (name_ != other.name_) return false;
    if (type_ != other.type_) return false;
    if (setting_values_ != other.setting_values_) return false;
    if (setting_names_ != other.setting_names_) return false;
    //if (energy_calibration_ != other.energy_calibration_) return false;
    return true;
  }

  void to_xml(tinyxml2::XMLPrinter&) const;
  void from_xml(tinyxml2::XMLElement*);
  Calibration highest_res_calib();
  
  std::string name_, type_;
  XMLableDB<Calibration> energy_calibrations_;
  std::vector<double> setting_values_;
  std::vector<std::string> setting_names_;
  
};

}

#endif
