/*
 * skeleton_lines.hpp
 *
 * Alternative skeletonization + line tracing pipeline.
 *
 *  - skeletonize_guohall(): Guo-Hall thinning (1989). Like Zhang-Suen it
 *    runs two sub-iterations until idempotent, but the removal predicate
 *    is different — Guo-Hall tends to leave fewer "stair" artefacts and
 *    produces a more uniformly 1-pixel-wide skeleton.
 *
 *  - trace_lines(): topology-aware tracer. Walks edges of the skeleton
 *    graph between "special" pixels (endpoints with degree 1, junctions
 *    with degree >= 3). Every emitted polyline starts and ends at a
 *    special pixel — junctions appear as shared endpoints of all
 *    incident polylines. Isolated closed loops (degree-2 components
 *    with no special pixel) are emitted in a second pass.
 *
 * Differs from algorithms/trace_skeleton.hpp:
 *  - Output is std::vector<std::vector<cv::Point>> (integer pixel coords),
 *    not JSContoursData<double>.
 *  - Cuts at every junction rather than greedily walking past with a
 *    direction-biased heuristic; the resulting set of polylines maps
 *    1:1 onto the edges of the skeleton's topological graph.
 *  - No "simplify" / RDP-lite collapsing; every skeleton pixel is kept.
 */

#ifndef SKELETON_LINES_HPP
#define SKELETON_LINES_HPP

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace skeleton_lines {

/* -------------------------------------------------------------------------
 * Guo-Hall thinning
 * ------------------------------------------------------------------------- */

/**
 * One Guo-Hall sub-iteration. `im` must be CV_8UC1 with foreground = 1.
 *
 * Neighbour labelling (matches Zhang-Suen for easy comparison):
 *     p9 p2 p3
 *     p8 p1 p4
 *     p7 p6 p5
 */
static inline void
guohall_iteration(cv::Mat& im, int iter) {
  cv::Mat marker = cv::Mat::zeros(im.size(), CV_8UC1);

  for(int y = 1; y < im.rows - 1; ++y) {
    const uchar* prev = im.ptr<uchar>(y - 1);
    const uchar* curr = im.ptr<uchar>(y);
    const uchar* next = im.ptr<uchar>(y + 1);
    uchar* mark = marker.ptr<uchar>(y);

    for(int x = 1; x < im.cols - 1; ++x) {
      if(!curr[x])
        continue;

      uchar p2 = prev[x];
      uchar p3 = prev[x + 1];
      uchar p4 = curr[x + 1];
      uchar p5 = next[x + 1];
      uchar p6 = next[x];
      uchar p7 = next[x - 1];
      uchar p8 = curr[x - 1];
      uchar p9 = prev[x - 1];

      int C = (!p2 && (p3 || p4)) + (!p4 && (p5 || p6)) + (!p6 && (p7 || p8)) + (!p8 && (p9 || p2));
      int N1 = (p9 || p2) + (p3 || p4) + (p5 || p6) + (p7 || p8);
      int N2 = (p2 || p3) + (p4 || p5) + (p6 || p7) + (p8 || p9);
      int N = N1 < N2 ? N1 : N2;
      int m = iter == 0 ? ((p6 || p7 || !p9) && p8) : ((p2 || p3 || !p5) && p4);

      if(C == 1 && (N >= 2 && N <= 3) && m == 0)
        mark[x] = 1;
    }
  }

  im &= ~marker;
}

/**
 * In-place Guo-Hall thinning. Input must be CV_8UC1, 0/255.
 * Output is 0/255 with foreground pixels reduced to a 1-pixel skeleton.
 */
static inline void
guohall_thinning(cv::Mat& im) {
  im /= 255;

  cv::Mat prev = cv::Mat::zeros(im.size(), CV_8UC1);
  cv::Mat diff;

  do {
    guohall_iteration(im, 0);
    guohall_iteration(im, 1);
    cv::absdiff(im, prev, diff);
    im.copyTo(prev);
  } while(cv::countNonZero(diff) > 0);

  im *= 255;
}

/**
 * Full pipeline: any-channel input -> grayscale -> Otsu -> Guo-Hall.
 * Returns a fresh 0/255 CV_8UC1 skeleton; the input is not modified.
 */
template<class InputArray>
static cv::Mat
skeletonize_guohall(InputArray src) {
  cv::Mat out;

  if(src.channels() == 1)
    src.copyTo(out);
  else
    cv::cvtColor(src, out, cv::COLOR_BGR2GRAY);

  cv::threshold(out, out, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
  guohall_thinning(out);

  return out;
}

/* -------------------------------------------------------------------------
 * Topology-aware line tracer
 * ------------------------------------------------------------------------- */

/* 8-neighbour offsets. The layout is required to satisfy
 *   N8[i] + N8[(i + 4) & 7] == (0, 0)
 * so opposite directions differ by 4. */
static constexpr std::array<cv::Point, 8> N8 = {
    cv::Point(-1, -1),
    cv::Point(0, -1),
    cv::Point(1, -1),
    cv::Point(1, 0),
    cv::Point(1, 1),
    cv::Point(0, 1),
    cv::Point(-1, 1),
    cv::Point(-1, 0),
};

/**
 * Compute the 8-connected degree of every foreground pixel.
 * Border pixels (first/last row/column) are reported as degree 0.
 */
static inline cv::Mat
degree_map(const cv::Mat& skel) {
  cv::Mat deg = cv::Mat::zeros(skel.size(), CV_8UC1);

  for(int y = 1; y < skel.rows - 1; ++y) {
    const uchar* prev = skel.ptr<uchar>(y - 1);
    const uchar* curr = skel.ptr<uchar>(y);
    const uchar* next = skel.ptr<uchar>(y + 1);
    uchar* drow = deg.ptr<uchar>(y);

    for(int x = 1; x < skel.cols - 1; ++x) {
      if(!curr[x])
        continue;

      int n = (prev[x - 1] != 0) + (prev[x] != 0) + (prev[x + 1] != 0) + (curr[x - 1] != 0) + (curr[x + 1] != 0) + (next[x - 1] != 0) + (next[x] != 0) + (next[x + 1] != 0);

      drow[x] = static_cast<uchar>(n);
    }
  }

  return deg;
}

/**
 * Trace a thinned skeleton into a set of polylines.
 *
 * Each polyline is a sequence of cv::Point that:
 *   - starts and ends at a "special" pixel (degree != 2), OR
 *   - is a closed loop with no special pixel anywhere on it.
 *
 * Special pixels are shared: a junction with degree k will appear as
 * the start or end point of exactly k polylines (one per incident edge).
 */
static inline std::vector<std::vector<cv::Point>>
trace_lines(const cv::Mat& skel) {
  std::vector<std::vector<cv::Point>> lines;

  const cv::Mat deg = degree_map(skel);

  /* Bit i of used(y,x) = the edge leaving (x,y) along N8[i] has been
   * walked. Per-direction marking lets junctions correctly seed several
   * polylines without re-emitting the same edge from both ends. */
  cv::Mat used = cv::Mat::zeros(skel.size(), CV_8UC1);

  auto in_bounds = [&](const cv::Point& p) { return p.x >= 0 && p.y >= 0 && p.x < skel.cols && p.y < skel.rows; };
  auto is_fg = [&](const cv::Point& p) { return skel.at<uchar>(p.y, p.x) != 0; };

  /* -- Pass 1: every edge incident to at least one special pixel ------- */

  for(int y = 0; y < skel.rows; ++y) {
    for(int x = 0; x < skel.cols; ++x) {
      uchar d = deg.at<uchar>(y, x);
      if(d == 0 || d == 2)
        continue; /* background or chain interior */

      const cv::Point sp(x, y);

      for(int i = 0; i < 8; ++i) {
        if(used.at<uchar>(sp.y, sp.x) & (1 << i))
          continue;

        const cv::Point first = sp + N8[i];
        if(!in_bounds(first) || !is_fg(first))
          continue;

        std::vector<cv::Point> chain;
        chain.push_back(sp);
        used.at<uchar>(sp.y, sp.x) |= (1 << i);

        cv::Point prev = sp;
        cv::Point cur = first;
        int dir = i;

        while(true) {
          chain.push_back(cur);

          if(deg.at<uchar>(cur.y, cur.x) != 2) {
            /* Reached another special pixel. Mark the reverse direction
             * so neither end re-emits this edge. */
            int rev = (dir + 4) & 7;
            used.at<uchar>(cur.y, cur.x) |= (1 << rev);
            break;
          }

          /* Step forward along the chain — exactly one non-prev fg
           * neighbour is expected (deg == 2). */
          cv::Point next;
          int next_dir = -1;
          for(int j = 0; j < 8; ++j) {
            const cv::Point q = cur + N8[j];
            if(!in_bounds(q) || !is_fg(q))
              continue;
            if(q == prev)
              continue;
            next = q;
            next_dir = j;
            break;
          }

          if(next_dir < 0)
            break;

          prev = cur;
          cur = next;
          dir = next_dir;
        }

        if(chain.size() >= 2)
          lines.push_back(std::move(chain));
      }
    }
  }

  /* -- Pass 2: pure-loop components (no special pixel) ----------------- */

  for(int y = 0; y < skel.rows; ++y) {
    for(int x = 0; x < skel.cols; ++x) {
      if(!skel.at<uchar>(y, x))
        continue;
      if(deg.at<uchar>(y, x) != 2)
        continue;
      if(used.at<uchar>(y, x))
        continue;

      const cv::Point start(x, y);
      std::vector<cv::Point> loop;
      cv::Point prev(-1, -1);
      cv::Point cur = start;

      do {
        loop.push_back(cur);
        used.at<uchar>(cur.y, cur.x) = 0xFF;

        cv::Point next;
        int next_dir = -1;
        for(int j = 0; j < 8; ++j) {
          const cv::Point q = cur + N8[j];
          if(!in_bounds(q) || !is_fg(q))
            continue;
          if(q == prev)
            continue;
          if(q != start && used.at<uchar>(q.y, q.x))
            continue;
          next = q;
          next_dir = j;
          break;
        }

        if(next_dir < 0)
          break;

        prev = cur;
        cur = next;
      } while(cur != start);

      if(cur == start)
        loop.push_back(start); /* close the polyline */

      if(loop.size() >= 2)
        lines.push_back(std::move(loop));
    }
  }

  return lines;
}

/* -------------------------------------------------------------------------
 * One-shot convenience
 * ------------------------------------------------------------------------- */

/**
 * Image -> Guo-Hall skeleton -> polylines.
 * If `skeleton_out` is non-null, the intermediate 0/255 skeleton is copied
 * there. The input is not modified.
 */
template<class InputArray>
static inline std::vector<std::vector<cv::Point>>
skeletonize_and_trace(InputArray src, cv::Mat* skeleton_out = nullptr) {
  cv::Mat skel = skeletonize_guohall(src);
  auto lines = trace_lines(skel);

  if(skeleton_out)
    *skeleton_out = skel;

  return lines;
}

} /* namespace skeleton_lines */

#endif /* SKELETON_LINES_HPP */
