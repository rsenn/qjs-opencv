// exif-find: list pictures by capture month range and GPS position, in parallel.
//
//   c++ -std=c++17 -O2 -pthread -o exif-find exif-find.cpp $(pkg-config --cflags --libs exiv2)
//   ./exif-find [options] DIR...
//
//   --from YYYY[-MM]      first month (inclusive)
//   --to   YYYY[-MM]      last month (inclusive; a bare year means December)
//   --near LAT,LON,KM     within KM kilometres of the point
//   --bbox S,W,N,E        inside the box (degrees)
//   --gps / --nogps       only pictures with / without a position
//   --undated             only pictures without any usable date
//   -l                    long output: path, date, source (e = EXIF, n = file name), lat, lon
//   -j N                  worker threads (default: number of CPUs)
//   -C FILE               cache (path, size, mtime -> metadata), makes repeated queries instant
//
// Date: DateTimeOriginal, DigitizedTime, DateTime; else the "YYYY-MM-DD_HH-MM-SS_" prefix that
// exif-rename-exiv2 adds, else a "YYYYMMDD_HHMMSS" in the name. Never the file mtime (PhotoRec sets
// it to the recovery time). Only JPEG and HEIC carry EXIF here; every other picture type is
// listed as undated without a position.
// Matches go to stdout, one per line; counts and errors to stderr.
#include <exiv2/exiv2.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <strings.h>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

struct Info {
  int ym = 0; // year * 12 + month - 1, 0 = no date
  char src = '-'; // e, n or -
  bool gps = false;
  double lat = 0, lon = 0;
  std::string date; // "YYYY-MM-DD HH:MM:SS" or ""
};

static bool
valid_date(int y, int mo, int d, int h, int mi, int s) {
  return y >= 1990 && y <= 2100 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31 && h >= 0 && h <= 23 && mi >= 0 && mi <= 59 && s >= 0 && s <= 60;
}

static bool
set_date(Info& in, int y, int mo, int d, int h, int mi, int s, char src) {
  if(!valid_date(y, mo, d, h, mi, s))
    return false;

  char b[32];
  snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d:%02d", y, mo, d, h, mi, s);
  in.date = b;
  in.ym = y * 12 + mo - 1;
  in.src = src;
  return true;
}

static bool
date_from_name(Info& in, const std::string& name) {
  int y, mo, d, h, mi, s;

  if(sscanf(name.c_str(), "%4d-%2d-%2d_%2d-%2d-%2d_", &y, &mo, &d, &h, &mi, &s) == 6 && set_date(in, y, mo, d, h, mi, s, 'n'))
    return true;

  for(size_t i = 0; i + 15 <= name.size(); i++)
    if(isdigit((unsigned char)name[i]) && name[i + 8] == '_' && sscanf(name.c_str() + i, "%4d%2d%2d_%2d%2d%2d", &y, &mo, &d, &h, &mi, &s) == 6 && set_date(in, y, mo, d, h, mi, s, 'n'))
      return true;

  return false;
}

static double
gps_coord(const Exiv2::ExifData& ed, const char* key, const char* ref, const char* neg) {
  auto it = ed.findKey(Exiv2::ExifKey(key));

  if(it == ed.end() || it->count() < 3)
    return NAN;

  double v = it->toFloat(0) + it->toFloat(1) / 60 + it->toFloat(2) / 3600;
  auto r = ed.findKey(Exiv2::ExifKey(ref));

  if(r != ed.end() && r->toString().compare(0, 1, neg) == 0)
    v = -v;

  return v;
}

static void
read_exif(const fs::path& path, Info& in) {
  static const char* keys[] = {"Exif.Photo.DateTimeOriginal", "Exif.Photo.DateTimeDigitized", "Exif.Image.DateTime"};

  try {
    auto img = Exiv2::ImageFactory::open(path.string());
    img->readMetadata();
    const Exiv2::ExifData& ed = img->exifData();

    for(const char* k : keys) {
      auto it = ed.findKey(Exiv2::ExifKey(k));
      int y, mo, d, h, mi, s;

      if(it != ed.end() && sscanf(it->toString().c_str(), "%4d:%2d:%2d %2d:%2d:%2d", &y, &mo, &d, &h, &mi, &s) == 6 && set_date(in, y, mo, d, h, mi, s, 'e'))
        break;
    }

    double la = gps_coord(ed, "Exif.GPSInfo.GPSLatitude", "Exif.GPSInfo.GPSLatitudeRef", "S");
    double lo = gps_coord(ed, "Exif.GPSInfo.GPSLongitude", "Exif.GPSInfo.GPSLongitudeRef", "W");

    if(std::isfinite(la) && std::isfinite(lo) && std::fabs(la) <= 90 && std::fabs(lo) <= 180 && !(la == 0 && lo == 0)) {
      in.gps = true;
      in.lat = la;
      in.lon = lo;
    }
  } catch(Exiv2::Error&) {
  }
}

static bool
has_exif(const fs::path& p) {
  std::string e = p.extension().string();
  return !strcasecmp(e.c_str(), ".jpg") || !strcasecmp(e.c_str(), ".jpeg") || !strcasecmp(e.c_str(), ".heic") || !strcasecmp(e.c_str(), ".heif");
}

static bool
is_picture(const fs::path& p) {
  static const char* ext[] = {".jpg", ".jpeg", ".heic", ".heif", ".png", ".tif", ".tiff", ".gif", ".webp", ".bmp", ".dng", ".cr2", ".cr3", ".nef", ".arw", ".orf", ".rw2", ".raf"};
  std::string e = p.extension().string();

  for(const char* x : ext)
    if(!strcasecmp(e.c_str(), x))
      return true;

  return false;
}

static double
haversine_km(double la1, double lo1, double la2, double lo2) {
  const double R = 6371.0088, k = M_PI / 180;
  double a = std::pow(std::sin((la2 - la1) * k / 2), 2) + std::cos(la1 * k) * std::cos(la2 * k) * std::pow(std::sin((lo2 - lo1) * k / 2), 2);

  return 2 * R * std::asin(std::sqrt(a));
}

// "YYYY" or "YYYY-MM"; end: a bare year runs to December
static bool
parse_month(const char* s, bool end, int& out) {
  int y, m = end ? 12 : 1, n = sscanf(s, "%d-%d", &y, &m);

  if(n < 1 || y < 1900 || y > 2200 || m < 1 || m > 12)
    return false;

  out = y * 12 + m - 1;
  return true;
}

int
main(int argc, char* argv[]) {
  int from = 0, to = 0, nthreads = std::max(1u, std::thread::hardware_concurrency());
  bool near_on = false, bbox_on = false, want_gps = false, want_nogps = false, undated = false, longout = false;
  double nlat = 0, nlon = 0, nkm = 0, bs = 0, bw = 0, bn = 0, be = 0;
  std::string cachefile;
  std::vector<std::string> dirs;

  for(int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto need = [&](const char* what) -> const char* {
      if(i + 1 >= argc) {
        fprintf(stderr, "%s needs %s\n", a.c_str(), what);
        exit(2);
      }
      return argv[++i];
    };

    if(a == "--from") {
      if(!parse_month(need("YYYY[-MM]"), false, from))
        return fprintf(stderr, "bad --from\n"), 2;
    } else if(a == "--to") {
      if(!parse_month(need("YYYY[-MM]"), true, to))
        return fprintf(stderr, "bad --to\n"), 2;
    } else if(a == "--near") {
      near_on = sscanf(need("LAT,LON,KM"), "%lf,%lf,%lf", &nlat, &nlon, &nkm) == 3;
      if(!near_on)
        return fprintf(stderr, "bad --near\n"), 2;
    } else if(a == "--bbox") {
      bbox_on = sscanf(need("S,W,N,E"), "%lf,%lf,%lf,%lf", &bs, &bw, &bn, &be) == 4;
      if(!bbox_on)
        return fprintf(stderr, "bad --bbox\n"), 2;
    } else if(a == "--gps")
      want_gps = true;
    else if(a == "--nogps")
      want_nogps = true;
    else if(a == "--undated")
      undated = true;
    else if(a == "-l")
      longout = true;
    else if(a == "-j")
      nthreads = std::max(1, atoi(need("N")));
    else if(a == "-C")
      cachefile = need("FILE");
    else if(a.size() > 1 && a[0] == '-') {
      fprintf(stderr, "unknown option %s\n", a.c_str());
      return 2;
    } else
      dirs.push_back(a);
  }

  if(dirs.empty()) {
    fprintf(stderr, "usage: %s [--from YYYY[-MM]] [--to YYYY[-MM]] [--near LAT,LON,KM] [--bbox S,W,N,E] [--gps|--nogps] [--undated] [-l] [-j N] [-C CACHE] DIR...\n", argv[0]);
    return 2;
  }

  Exiv2::enableBMFF();
  Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);

  // initialise exiv2's static tag tables before the threads start
  { Info warm; read_exif("/nonexistent", warm); Exiv2::ExifKey k("Exif.Photo.DateTimeOriginal"); }

  struct Entry { fs::path path; uintmax_t size; long long mtime; Info info; bool cached = false; };
  std::vector<Entry> files;
  std::error_code ec;

  for(const std::string& d : dirs)
    for(fs::recursive_directory_iterator it(d, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
      if(ec) {
        fprintf(stderr, "%s: %s\n", d.c_str(), ec.message().c_str());
        ec.clear();
        continue;
      }

      std::error_code e2;

      if(it->is_regular_file(e2) && !it->is_symlink(e2) && is_picture(it->path())) {
        Entry en;
        en.path = it->path();
        en.size = it->file_size(e2);
        en.mtime = std::chrono::duration_cast<std::chrono::seconds>(it->last_write_time(e2).time_since_epoch()).count();
        files.push_back(std::move(en));
      }
    }

  // cache: path <TAB> size <TAB> mtime <TAB> ym <TAB> src <TAB> gps <TAB> lat <TAB> lon <TAB> date
  std::map<std::string, Entry*> byname;

  if(!cachefile.empty()) {
    for(Entry& e : files)
      byname[e.path.string()] = &e;

    std::ifstream cf(cachefile);
    std::string line;

    while(std::getline(cf, line)) {
      std::vector<std::string> f;
      std::stringstream ss(line);
      std::string x;

      while(std::getline(ss, x, '\t'))
        f.push_back(x);

      if(f.size() < 8)
        continue;

      auto it = byname.find(f[0]);

      if(it == byname.end() || (uintmax_t)atoll(f[1].c_str()) != it->second->size || atoll(f[2].c_str()) != it->second->mtime)
        continue;

      Info& in = it->second->info;
      in.ym = atoi(f[3].c_str());
      in.src = f[4][0];
      in.gps = f[5] == "1";
      in.lat = atof(f[6].c_str());
      in.lon = atof(f[7].c_str());
      in.date = f.size() > 8 ? f[8] : "";
      it->second->cached = true;
    }
  }

  std::atomic<size_t> next{0}, scanned{0};
  auto worker = [&] {
    for(size_t i; (i = next++) < files.size();) {
      Entry& e = files[i];

      if(e.cached)
        continue;

      if(has_exif(e.path))
        read_exif(e.path, e.info);

      if(!e.info.ym)
        date_from_name(e.info, e.path.filename().string());

      scanned++;
    }
  };

  std::vector<std::thread> pool;

  for(int t = 0; t < nthreads; t++)
    pool.emplace_back(worker);

  for(auto& t : pool)
    t.join();

  if(!cachefile.empty()) {
    std::ofstream cf(cachefile, std::ios::trunc);

    for(const Entry& e : files)
      cf << e.path.string() << '\t' << e.size << '\t' << e.mtime << '\t' << e.info.ym << '\t' << e.info.src << '\t' << e.info.gps << '\t' << e.info.lat << '\t' << e.info.lon << '\t' << e.info.date << '\n';
  }

  size_t hits = 0, nodate = 0, nogps = 0;

  for(const Entry& e : files) {
    const Info& in = e.info;

    if(!in.ym)
      nodate++;
    if(!in.gps)
      nogps++;

    if(undated ? in.ym != 0 : ((from || to) && (!in.ym || (from && in.ym < from) || (to && in.ym > to))))
      continue;
    if(want_gps && !in.gps)
      continue;
    if(want_nogps && in.gps)
      continue;
    if((near_on || bbox_on) && !in.gps)
      continue;
    if(near_on && haversine_km(nlat, nlon, in.lat, in.lon) > nkm)
      continue;
    if(bbox_on && (in.lat < bs || in.lat > bn || in.lon < bw || in.lon > be))
      continue;

    hits++;

    if(longout)
      printf("%s\t%s\t%c\t%s\t%s\n", e.path.c_str(), in.date.empty() ? "-" : in.date.c_str(), in.src, in.gps ? std::to_string(in.lat).c_str() : "-", in.gps ? std::to_string(in.lon).c_str() : "-");
    else
      printf("%s\n", e.path.c_str());
  }

  fprintf(stderr, "%zu pictures, %zu read (%zu from cache), %zu without date, %zu without position, %zu matches\n", files.size(), (size_t)scanned, files.size() - scanned, nodate, nogps, hits);
  return 0;
}
