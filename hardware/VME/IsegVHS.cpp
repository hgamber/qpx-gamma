#include "IsegVHS.h"
#include "boost/endian/conversion.hpp"
#include "custom_logger.h"


#define ISEG_VENDOR_ID									0x69736567
#define VME_ADDRESS_MODIFIER						0x29

// --- VHS12 ------------------------------------------------------------------
#define VhsFirmwareReleaseOFFSET          56
#define VhsDeviceClassOFFSET              62
#define VhsVendorIdOFFSET                 92
#define VhsNewBaseAddressOFFSET           0x03A0
#define VhsNewBaseAddressXorOFFSET        0x03A2
#define VhsNewBaseAddressAcceptedOFFSET   0x03A6

typedef union {
	long    l;
	float   f;
  uint16_t  w[2];
  uint8_t   b[4];
} TFloatWord;


IsegVHS::IsegVHS()
{
  m_controller = nullptr;
  m_baseAddress = 0;
}

IsegVHS::~IsegVHS()
{}

bool IsegVHS::connect(VmeController *controller, int baseAddress)
{
  m_controller = controller;
  setBaseAddress(baseAddress);
}

bool IsegVHS::connected() const
{
  uint32_t vendorId = 0;
  if (m_controller)
    vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);

	return vendorId == ISEG_VENDOR_ID;
}

void IsegVHS::disconnect()
{
  m_controller = nullptr;
  m_baseAddress = 0;
}

std::string IsegVHS::address() const {
  std::stringstream stream;
  stream << "VME BA 0x"
         << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
         << std::hex << m_baseAddress;
  return stream.str();
}

bool IsegVHS::read_setting(Gamma::Setting& set, int address_modifier) const {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.setting_type == Gamma::SettingType::floating)
    set.value_dbl = readFloat(m_baseAddress + address_modifier + set.metadata.address);
  else if (set.metadata.setting_type == Gamma::SettingType::binary) {
    if (set.metadata.maximum == 32)
      set.value_int = readLongBitfield(m_baseAddress + address_modifier + set.metadata.address);
    else if (set.metadata.maximum == 16)
      set.value_int = readShortBitfield(m_baseAddress + address_modifier + set.metadata.address);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::boolean)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.hardware_type == "u32")
      set.value_int = readLong(m_baseAddress + address_modifier + set.metadata.address);
    else if (set.metadata.hardware_type == "u16")
      set.value_int = readShort(m_baseAddress + address_modifier + set.metadata.address);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  return true;
}

bool IsegVHS::write_setting(Gamma::Setting& set, int address_modifier) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  if (set.metadata.setting_type == Gamma::SettingType::floating)
    writeFloat(m_baseAddress + address_modifier + set.metadata.address, set.value_dbl);
  else if (set.metadata.setting_type == Gamma::SettingType::binary) {
    if (set.metadata.maximum == 32)
      writeLongBitfield(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else if (set.metadata.maximum == 16)
      writeShortBitfield(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::boolean)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.hardware_type == "u32")
      writeLong(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else if (set.metadata.hardware_type == "u16")
      writeShort(m_baseAddress + address_modifier + set.metadata.address, set.value_int);
    else {
      PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
      return false;
    }
  }
  return true;
}

uint16_t IsegVHS::readShort(int address) const
{
  if (m_controller) {
		return m_controller->readShort(address, VME_ADDRESS_MODIFIER);
	} else {
		return 0;
	}
}

void IsegVHS::writeShort(int address, uint16_t data)
{
  if (m_controller) {
		m_controller->writeShort(address, VME_ADDRESS_MODIFIER, data);
	}
}

uint16_t IsegVHS::readShortBitfield(int address) const
{
	return readShort(address);
}

void IsegVHS::writeShortBitfield(int address, uint16_t data)
{
	writeShort(address, data);
}

float IsegVHS::readFloat(int address) const
{
	TFloatWord fw = { 0 };

#ifdef BOOST_LITTLE_ENDIAN
		fw.w[1] = readShort(address + 0); // swap words
		fw.w[0] = readShort(address + 2);
#elif BOOST_BIG_ENDIAN
		fw.w[0] = readShort(address + 0);
		fw.w[1] = readShort(address + 2);
#endif

	return fw.f;
}

void IsegVHS::writeFloat(int address, float data)
{
	TFloatWord fw;

	fw.f = data;

#ifdef BOOST_LITTLE_ENDIAN
		writeShort(address + 0, fw.w[1]);
		writeShort(address + 2, fw.w[0]);
#elif BOOST_BIG_ENDIAN
		writeShort(address + 0, fw.w[0]);
		writeShort(address + 2, fw.w[1]);
#endif
}

uint32_t IsegVHS::readLong(int address) const
{
	TFloatWord fw = { 0 };

#ifdef BOOST_LITTLE_ENDIAN
		fw.w[1] = readShort(address + 0); // swap words
		fw.w[0] = readShort(address + 2);
#elif BOOST_BIG_ENDIAN
		fw.w[0] = readShort(address + 0);
		fw.w[1] = readShort(address + 2);
#endif

	return fw.l;
}

void IsegVHS::writeLong(int address, uint32_t data)
{
	TFloatWord fw;

	fw.l = data;

#ifdef BOOST_LITTLE_ENDIAN
		writeShort(address + 0, fw.w[1]);
		writeShort(address + 2, fw.w[0]);
#elif BOOST_BIG_ENDIAN
		writeShort(address + 0, fw.w[0]);
		writeShort(address + 2, fw.w[1]);
#endif
}

uint32_t IsegVHS::readLongBitfield(int address) const
{
	return readLong(address);
}

void IsegVHS::writeLongBitfield(int address, uint32_t data)
{
	writeLong(address, data);
}

/**
 * Mirrors the Bit positions in a 16 bit word.
 */
uint16_t IsegVHS::mirrorShort(uint16_t data)
{
  uint16_t result = 0;

	for (int i = 16; i; --i) {
		result >>= 1;
		result |= data & 0x8000;
		data <<= 1;
	}

	return result;
}

/**
 * Mirrors the Bit positions in a 32 bit word.
 */
uint32_t IsegVHS::mirrorLong(uint32_t data)
{
  uint32_t result = 0;

	for (int i = 32; i; --i) {
		result >>= 1;
		result |= data & 0x80000000;
		data <<= 1;
	}

	return result;
}

//=============================================================================
// Module Commands
//=============================================================================


std::string IsegVHS::firmwareName() const
{
  if (!m_controller)
    return std::string();

  uint32_t version = readLong(m_baseAddress + VhsFirmwareReleaseOFFSET);

  return std::to_string(version >> 24) + std::to_string(version >> 16) + "." +
    std::to_string(version >>  8) + std::to_string(version >>  0);
}


bool IsegVHS::setBaseAddress(uint16_t baseAddress)
{
	m_baseAddress = baseAddress;

  //uint32_t vendorId = readLong(m_baseAddress + VhsVendorIdOFFSET);
  uint32_t tmp = readLong(m_baseAddress + 60);

  int deviceClass = (uint16_t)tmp;

  //PL_DBG << "device class = " << deviceClass;

  int V12C0 = 20;		// IsegVHS 12 channel common ground VME

  return (deviceClass == V12C0);
}

//=============================================================================
// Special Commands
//=============================================================================

void IsegVHS::programBaseAddress(uint16_t address)
{
  writeShort(m_baseAddress + VhsNewBaseAddressOFFSET, address);
  writeShort(m_baseAddress + VhsNewBaseAddressXorOFFSET, address ^ 0xFFFF);
}

uint16_t IsegVHS::verifyBaseAddress(void) const
{
  uint16_t newAddress = -1;
  uint16_t temp;

  newAddress = readShort(m_baseAddress + VhsNewBaseAddressAcceptedOFFSET);
  temp = readShort(m_baseAddress + VhsNewBaseAddressOFFSET);
  if (newAddress != temp) {
    newAddress = -1;
  }

	return newAddress;
}



