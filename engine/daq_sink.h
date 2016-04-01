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
 *      Qpx::Sink generic spectrum type.
 *                       All public methods are thread-safe.
 *                       When deriving override protected methods.
 *
 *      Qpx::SinkFactory creates spectra of appropriate type
 *                               by type name, from template, or
 *                               from file.
 *
 *      Qpx::SinkRegistrar for registering new spectrum types.
 *
 ******************************************************************************/

#ifndef QPX_SINK_H
#define QPX_SINK_H

#include <initializer_list>
#include <boost/thread.hpp>

#include "spill.h"
#include "detector.h"
#include "custom_logger.h"

namespace Qpx {

typedef std::pair<std::vector<uint16_t>, PreciseFloat> Entry;
typedef std::list<Entry> EntryList;
typedef std::pair<uint32_t, uint32_t> Pair;


struct Metadata : public XMLable {
 private:
  //this stuff from factory
  std::string type_, type_description_;
  uint16_t dimensions_;
  std::list<std::string> input_types_;
  std::list<std::string> output_types_;

 public:
  //user sets these in prototype
  std::string name;
  uint16_t bits;
  Qpx::Setting attributes;

  //take care of these...
  bool changed;
  PreciseFloat total_count;
  std::vector<Qpx::Detector> detectors;

  std::string type() const {return type_;}
  std::string type_description() const {return type_description_;}
  uint16_t dimensions() const {return dimensions_;}
  std::list<std::string> input_types() const {return input_types_;}
  std::list<std::string> output_types() const {return output_types_;}
  void set_det_limit(uint16_t limit);

  Metadata()
    : type_("invalid")
    , dimensions_(0)
    , bits(14)
    , attributes("Options")
    , total_count(0.0)
    , changed(false)
    { attributes.metadata.setting_type = SettingType::stem; }

  Metadata(std::string tp, std::string descr, uint16_t dim,
           std::list<std::string> itypes, std::list<std::string> otypes)
    : type_(tp)
    , type_description_(descr)
    , dimensions_(dim)
    , input_types_(itypes)
    , output_types_(otypes)
    , bits(14)
    , attributes("Options")
    , total_count(0.0)
    , changed(false)
    { attributes.metadata.setting_type = SettingType::stem; }

 std::string xml_element_name() const override {return "SinkMetadata";}

 void to_xml(pugi::xml_node &node) const override;
 void from_xml(const pugi::xml_node &node) override;

 bool shallow_equals(const Metadata& other) const {return (name == other.name);}
 bool operator!= (const Metadata& other) const {return !operator==(other);}
 bool operator== (const Metadata& other) const {
   if (name != other.name) return false;
   if (type_ != other.type_) return false; //assume other type info same
   if (bits != other.bits) return false;
   if (attributes != other.attributes) return false;
   return true;
 }
};


class Sink
{
public:
  Sink();

  //named constructors, used by factory
  bool from_prototype(const Metadata&);
  bool from_xml(const pugi::xml_node &);

  void to_xml(pugi::xml_node &) const;

  //data acquisition
  void push_spill(const Spill&);
  void flush();

  //get count at coordinates in n-dimensional list
  PreciseFloat data(std::initializer_list<uint16_t> list = {}) const;

  //optimized retrieval of bulk data as list of Entries
  //parameters take dimensions_ number of ranges of minimum (inclusive) and maximum (exclusive)
  std::unique_ptr<EntryList> data_range(std::initializer_list<Pair> list = {});
  void append(const Entry&);

  //retrieve axis-values for given dimension (can be precalculated energies)
  std::vector<double> axis_values(uint16_t dimension) const;

  //export to some format (factory keeps track of file types)
  bool write_file(std::string dir, std::string format) const;
  bool read_file(std::string name, std::string format);

  //Metadata
  Metadata metadata() const;

  //Convenience functions for most common metadata
  std::string name() const;
  std::string type() const;
  uint16_t dimensions() const;
  uint16_t bits() const;

  //Change metadata
  void set_option(Setting setting, Match match = Match::id | Match::indices);
  void set_options(Setting settings);
  void set_detectors(const std::vector<Qpx::Detector>& dets);
  void reset_changed();

protected:
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////THIS IS THE MEAT////////////////
  ///implement these to make custom types///
  //////////////////////////////////////////
  
  virtual std::string my_type() const = 0;
  virtual bool _initialize();

  virtual void _set_detectors(const std::vector<Qpx::Detector>& dets) = 0;
  virtual void _push_spill(const Spill&);
  virtual void _push_hit(const Hit&) = 0;
  virtual void _push_stats(const StatsUpdate&) = 0;
  virtual void _flush() {}

  virtual PreciseFloat _data(std::initializer_list<uint16_t>) const {return 0;}
  virtual std::unique_ptr<std::list<Entry>> _data_range(std::initializer_list<Pair>)
    { return std::unique_ptr<std::list<Entry>>(new std::list<Entry>); }
  virtual void _append(const Entry&) {}

  virtual std::string _data_to_xml() const = 0;
  virtual uint16_t _data_from_xml(const std::string&) = 0;
  virtual bool _write_file(std::string, std::string) const {return false;}
  virtual bool _read_file(std::string, std::string) {return false;}

  virtual void _recalc_axes() = 0;
  Setting get_attr(std::string name) const;
  
  Metadata metadata_;
  std::vector<std::vector<double> > axes_;

  mutable boost::shared_mutex shared_mutex_;
  mutable boost::mutex unique_mutex_;
};

}

#endif