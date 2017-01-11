#include "custom_logger.h"
#include <boost/algorithm/string.hpp>
#include "cpx.h"

const int MAX_CHARS_PER_LINE = 512;
const int MAX_TOKENS_PER_LINE = 20;
const char* const DELIMITER = " ";

using namespace Qpx;

int main(int argc, char *argv[])
{
  //user input
  if (argc < 2) {
    std::cout << "Usage: cpx gamma_acquisition_batch.gab\n";
    return 1;
  }

  std::string gab(argv[1]);
  if (gab.empty()) {
    std::cout << "Bad batch file\n";
    return 1;
  }

  std::ifstream gabfile;
  gabfile.open(gab);
  if (!gabfile.good()) {
    std::cout << "Could not open batch file\n";
    return 1;
  }
  
  CustomLogger::initLogger(nullptr, "qpx_%N.log");
  LINFO << "--==cpx console tool for gamma acquisition==--";

  std::vector<std::string> cmd_params;
  for (int i=2; i<argc; ++i)
    cmd_params.push_back(std::string(argv[i]));
  
  std::list<CpxLine> program;

  if (!parse_file(gabfile, program, cmd_params)) {
    ERR << "<cpx> parsing failed. Aborting";
    return 1;
  }

  Cpx interpreter;
  std::vector<double> variables;
  if (!interpreter.interpret(program, variables)) {
    ERR << "<cpx> interpreting failed. Aborting";
    return 1;
  }

  return 0;
}

bool parse_file(std::ifstream &file, std::list<CpxLine> &program, std::vector<std::string> &cmd_params) {
   std::list<std::string> lines;

  //read whole file, omitting commented and empty lines
  while (!file.eof())
  {
    char buf[MAX_CHARS_PER_LINE];
    file.getline(buf, MAX_CHARS_PER_LINE);
    if (buf[0] != '#') {
      std::string buf_str(buf);
      boost::algorithm::trim(buf_str);
      if (buf_str.size() > 0)
        lines.push_back(buf_str);
    }
  }

  while (!lines.empty()) {
    //tokenize
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, lines.front(), boost::algorithm::is_any_of(" "));

    //populate parameters, replacing $x with command line arguments
    CpxLine line;
    line.command = tokens[0];
    for (size_t j=1; j<tokens.size(); ++j) {
      if (tokens[j][0] == '$') {
        int parnr = boost::lexical_cast<int>(tokens[j].substr(1,std::string::npos)) - 1;
        if ((parnr > -1) && (parnr < int(cmd_params.size())))
          line.params.push_back(cmd_params[parnr]);
        else {
          ERR << "<cpx> command line option not provided for token " << tokens[j]
                 << " for line: " << lines.front();
          return false;
        }
      } else
        line.params.push_back(tokens[j]);
    }
    lines.pop_front();
    program.push_back(line);
  }
  return true;
}


bool Cpx::interpret(std::list<CpxLine> commands, std::vector<double> variables) {
  bool running = true;

  while (running && !commands.empty()) {
    CpxLine line = commands.front();
    commands.pop_front();

    for (size_t i=0; i<line.params.size(); ++i) {
      if (line.params[i][0] == '%') {
        int varnr = boost::lexical_cast<int>(line.params[i].substr(1,std::string::npos));
        int j = varnr;

        if (varnr < 0)
          j = 0 - varnr - 1;
        else if (varnr > 0)
          j = varnr - 1;
        else {
          ERR << "<cpx> no variable 0";
          return false;
        }
          
        
        if (j < int(variables.size()))
        {
          std::string val = std::to_string(variables[j]);
          if (varnr > 0)
            line.params[i] = val;
          else if ((varnr < 0) && (i > 0))
            line.params[i-1] += val;
          else {
            ERR << "<cpx> cannot concatenate below token 0";
            return false;
          }
          
          
        } else {
          ERR << "<cpx> no variable " << j
                 << " in  this scope";
          return false;
        }
      }
    }

    LINFO << "<cpx> interpreting " << line.command;
    boost::this_thread::sleep(boost::posix_time::seconds(2));
    
    bool success=false;
    if (line.command == "end") {
      LINFO << "<cpx> exiting";
      return true;      
    }
    else if (line.command == "boot")
      success = boot(line.params);
    else if (line.command == "templates")
      success = templates(line.params);
    else if (line.command == "run_mca")
      success = run_mca(line.params);
    else if (line.command == "save_qpx")
      success = save_qpx(line.params);
    else if (line.command == "endfor") {
      if (variables.size())
        return true;
      else {
        ERR << "<cpx> not inside loop";
        return false;
      }
    }
    else if (line.command == "for") {
      if (line.params.size() < 3) {
        ERR << "<cpx> not enough parameters provided for FOR";
        return false;
      }
      double start = boost::lexical_cast<double>(line.params[0]);
      double step = boost::lexical_cast<double>(line.params[1]);
      double end = boost::lexical_cast<double>(line.params[2]);

      variables.push_back(0.0);
      for (double d=start; d <= end; d +=step) {
        variables[variables.size() - 1] = d;
        bool ret =  interpret(commands, variables);
        if (!ret)
          return false;
      }
      success = true;
      variables.pop_back();
      while (!commands.empty() && (commands.front().command != "endfor"))
        commands.pop_front();
      if (!commands.empty() && (commands.front().command == "endfor"))
        commands.pop_front();
    }

    if (!success)
      return false;
    
  }
  return true;
}


bool Cpx::templates(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    ERR << "<cpx> expected syntax: template template_file.tem";
    return false;
  }
  std::string file(tokens[0]);

  XMLableDB<Metadata>  spectra_templates_("SpectrumTemplates");
  spectra_templates_.read_xml(file);
  if (spectra_templates_.empty()) {
    ERR << "<cpx> bad template file " << file;
    return false;
  }
  LINFO << "<cpx> loading templates from " << file;
  spectra_->clear();
  spectra_->set_prototypes(spectra_templates_);
  return true;
}

bool Cpx::run_mca(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    ERR << "<cpx> expected syntax: run_mca duration";
    return false;
  }

  uint64_t duration = boost::lexical_cast<uint64_t>(tokens[0]);
  if (duration == 0) {
    ERR << "<cpx> bad duration";
    return false;
  }
  
  //double-buffer always
  engine_.getMca(duration, spectra_, interruptor_);
  return true;
}

bool Cpx::save_qpx(std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    ERR << "<cpx> expected syntax: save_qpx filename(.qpx)";
    return false;
  }
  std::string out_name(tokens[0]);
  if (out_name.empty()) {
    //check if exists, if valid
    ERR << "<cpx> bad output file name provided";
    return false;
  }

  std::string full_name = out_name + ".qpx";
  LINFO << "<cpx> writing acquired data to " << full_name;
  spectra_->save_as(full_name);
  return true;
}

bool Cpx::boot(std::vector<std::string> &tokens) {
  if (tokens.size() < 2) {
    ERR << "<cpx> expected syntax: boot [path/profile.set] [path/settingsdir]";
    return false;
  }
  std::string profile(tokens[0]);
  std::string settings_dir(tokens[1]);

  engine_.initialize(profile, settings_dir);

  if (!engine_.boot())
  {
    ERR << "<cpx> couldn't boot";
    return false;
  }

  engine_.get_all_settings();

  std::vector<Detector> dets = engine_.get_detectors();

  XMLableDB<Detector> detectors_("Detectors");
  detectors_.read_xml(settings_dir + "/default_detectors.det");
  if (detectors_.empty()) {
    ERR << "<cpx> bad detector db";
    return false;
  }

  std::map<int, Detector> update;
  for (size_t i=0; i < dets.size(); ++i)
    if (detectors_.has_a(dets[i]))
      update[i] = detectors_.get(dets[i]);

  for (auto &q : update)
    engine_.set_detector(q.first, q.second);

  engine_.load_optimization();
  engine_.write_settings_bulk();
  engine_.get_all_settings();

  return true;
}
