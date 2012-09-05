#include "config.h"

namespace NLP { namespace config {

OpGroup::OpGroup(OpGroup &group, const std::string &name,
    const std::string &desc) : OptionBase(name, desc), _children() {
  group.add(this);
}

void OpGroup::help(std::ostream &out, const std::string &prefix, const unsigned int depth) const {
  const std::string me = prefix + _name + "-";
  if (depth != 0)
    out << std::endl;
  out << port::BOLD << prefix << _name << port::OFF << ": " << _desc << std::endl;

  for (std::vector<OptionBase *>::const_iterator child = _children.begin(); child != _children.end(); ++child)
    (*child)->help(out, me, depth + 1);
  out << std::endl;
}

OptionBase *OpGroup::process(const std::string &orig_key, const std::string &key) {
  const size_t pos = key.find(_name);

  if (pos != 0)
    return NULL;
  else if (key.size() == _name.size())
    return this;
  else if (key[_name.size()] != '-')
    throw ConfigException("Invalid option specified", orig_key);

  std::string sub_key = key.substr(_name.size() + 1);
  if (sub_key.empty())
    throw ConfigException("Invalid option specified", orig_key);

  for (std::vector<OptionBase *>::iterator child = _children.begin(); child != _children.end(); ++child) {
    OptionBase *const p = (*child)->process(orig_key, sub_key);
    if (p)
      return p;
  }
  return NULL;
}

void OpGroup::set(const std::string &value) {
  throw ConfigException("Cannot set the value of an OpGroup", _name, value);
}

void OpGroup::validate(void) {
  for (std::vector<OptionBase *>::iterator child = _children.begin(); child != _children.end(); ++child)
    (*child)->validate();
}

void Config::help(std::ostream &out, const std::string &prefix, const unsigned int depth) const {
  out << port::BOLD << _name << port::OFF << ": " << _desc << '\n' << std::endl;
  for (std::vector<OptionBase *>::const_iterator child = _children.begin(); child != _children.end(); ++child)
    (*child)->help(out, prefix, depth + 1);
  out << std::endl;
}

OptionBase *Config::process(const std::string &orig_key, const std::string &key) {
  for (std::vector<OptionBase *>::iterator child = _children.begin(); child != _children.end(); ++child) {
    OptionBase *const p = (*child)->process(orig_key, key);
    if (p)
      return p;
  }
  return NULL;
}

bool Config::process(const int argc, const char *const argv[], std::ostream &out) {
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "--help") == 0) {
      help(out);
      return false;
    }
    else if (strcmp(argv[i], "--version") == 0) {
      out << argv[0] << " " << VERSION << std::endl;
      return false;
    }
  }

  for (int i = 1; i < argc; ++i) {
    const std::string key(argv[i]);
    if (key.size() < 2 || !(key[0] == '-' && key[1] == '-'))
      throw ConfigException("Invalid option specified", key);

    const std::string opt = key.substr(2);
    OptionBase *const p = process(key, opt);
    if (!p)
      throw ConfigException("Option not found", key);

    if (p->requires_arg() && (i+1) < argc)
      p->set(std::string(argv[++i]));
    else
      p->set(key);
  }
  validate();
  return true;
}

} }
