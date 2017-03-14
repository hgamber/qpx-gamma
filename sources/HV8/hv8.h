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
 *      QpxVmePlugin
 *
 ******************************************************************************/

#pragma once

#include "daq_source.h"
#include "detector.h"
#include "TimeoutSerial.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

namespace Qpx {

class QpxHV8Plugin : public Source {
  
public:
  QpxHV8Plugin();
  ~QpxHV8Plugin();

  static std::string plugin_name() {return "HV8";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Qpx::Setting &set) override;
  bool read_settings_bulk(Qpx::Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool die() override;

private:
  //no copying
  void operator=(QpxHV8Plugin const&);
  QpxHV8Plugin(const QpxHV8Plugin&);

  void set_voltage(int, double);

protected:
  void rebuild_structure(Qpx::Setting &set);

  bool get_prompt(uint16_t attempts);

  TimeoutSerial port;

  std::string portname;
  unsigned int baudrate;
  int attempts_;
  double timeout_;

  boost::asio::serial_port_base::character_size charactersize;
  boost::asio::serial_port_base::parity parity;
  boost::asio::serial_port_base::stop_bits stopbits;
  boost::asio::serial_port_base::flow_control flowcontrol;

  std::string controller_name_;

  std::vector<double> voltages;
};

}
