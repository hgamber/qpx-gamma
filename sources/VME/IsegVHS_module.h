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
 *      QpxIsegVHSPlugin
 *
 ******************************************************************************/

#pragma once

#include "vmemodule.h"

#define VHS_ADDRESS_SPACE_LENGTH				0x0400

namespace Qpx {

class QpxIsegVHSPlugin : public VmeModule {
  
public:
  QpxIsegVHSPlugin();
  ~QpxIsegVHSPlugin();

  //QpxPlugin
  static std::string plugin_name() {return "VME/IsegVHS";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Qpx::Setting &set) override;
  bool read_settings_bulk(Qpx::Setting &set) const override;
  void get_all_settings() override;

  //VmeModule
  uint32_t baseAddressSpaceLength() const override { return VHS_ADDRESS_SPACE_LENGTH; }
  bool    connected() const override;
  std::string firmwareName() const;


  void    programBaseAddress(uint32_t address);
  uint32_t verifyBaseAddress(void) const;

private:
  //no copying
  void operator=(QpxIsegVHSPlugin const&);
  QpxIsegVHSPlugin(const QpxIsegVHSPlugin&);

protected:
  void rebuild_structure(Qpx::Setting &set);
  bool read_setting(Qpx::Setting& set) const;
  bool write_setting(Qpx::Setting& set);

  uint16_t readShort(int address) const;
  void    writeShort(int address, uint16_t data);
  uint32_t readLong(int address) const;
  void    writeLong(int address, uint32_t data);
  float   readFloat(int address) const;
  void    writeFloat(int address, float data);

};

}
