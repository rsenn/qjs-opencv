#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "util.hpp"

static size_t heap_base = 0;

bool
str_end(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool
str_end(const char* str, const char* suffix) {
  return str_end(std::string(str), std::string(suffix));
}

void*
get_heap_base() {
  if(!heap_base) {
    std::ifstream infile("/proc/self/maps");
    infile >> std::hex;
    for(std::string line; std::getline(infile, line);) {
      if(line.find("[heap]") != std::string::npos) {
        size_t start = line.find("-");
        heap_base = strtoull(line.c_str() + start + 1, nullptr, 16);
      }
    }
    infile >> heap_base;
    //  std::cerr << "heap_base: " << heap_base << std::endl;
  }
  return reinterpret_cast<void*>(heap_base);
}

std::string
js_prop_flags(int flags) {
  std::vector<const char*> names;
  if(flags & JS_PROP_CONFIGURABLE)
    names.push_back("CONFIGURABLE");
  if(flags & JS_PROP_WRITABLE)
    names.push_back("WRITABLE");
  if(flags & JS_PROP_ENUMERABLE)
    names.push_back("ENUMERABLE");
  if(flags & JS_PROP_NORMAL)
    names.push_back("NORMAL");
  if(flags & JS_PROP_GETSET)
    names.push_back("GETSET");
  if(flags & JS_PROP_VARREF)
    names.push_back("VARREF");
  if(flags & JS_PROP_AUTOINIT)
    names.push_back("AUTOINIT");
  return join(names.cbegin(), names.cend(), "|");
}
std::ostream&
operator<<(std::ostream& s, const JSCFunctionListEntry& entry) {
  std::string name(entry.name);
  s << name << std::setw(30 - name.size()) << ' ';
  s << "type = "
    << (std::vector<const char*>{
           "CFUNC", "CGETSET", "CGETSET_MAGIC", "PROP_STRING", "PROP_INT32", "PROP_INT64", "PROP_DOUBLE", "PROP_UNDEFINED", "OBJECT", "ALIAS"})[entry.def_type]
    << ", ";
  switch(entry.def_type) {
    case JS_DEF_CGETSET_MAGIC: s << "magic = " << (unsigned int)entry.magic << ", "; break;
    case JS_DEF_PROP_INT32: s << "value = " << std::setw(9) << entry.u.i32 << ", "; break;
    case JS_DEF_PROP_INT64: s << "value = " << std::setw(9) << entry.u.i64 << ", "; break;
    case JS_DEF_PROP_DOUBLE: s << "value = " << std::setw(9) << entry.u.f64 << ", "; break;
    case JS_DEF_PROP_UNDEFINED:
      s << "value = " << std::setw(9) << "undefined"
        << ", ";
      break;
    case JS_DEF_PROP_STRING: s << "value = " << std::setw(9) << entry.u.str << ", "; break;
  }
  s << "flags = " << js_prop_flags(entry.prop_flags) << std::endl;
  return s;
}
