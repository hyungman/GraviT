
#ifndef MISC_TIMER
#define MISC_TIMER

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

namespace gvt {
namespace core {
namespace time {

#if GVT_USE_TIMING

struct timer {

  std::chrono::time_point<std::chrono::high_resolution_clock> t_start;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_end;
  double total_elapsed;
  bool running;
  std::string text;

  inline timer(bool running = true, std::string text = "") : text(text), total_elapsed(0), running(running) {
    if (running) {
      t_end = std::chrono::high_resolution_clock::now();
      t_start = std::chrono::high_resolution_clock::now();
    }
  }

  inline timer(const timer &other) {
    text = other.text;
    total_elapsed = other.total_elapsed;
  }

  inline void operator=(const timer &other) {
    // text = other.text;
    total_elapsed = other.total_elapsed;
  }

  inline void settext(std::string str) { text = str; }
  inline ~timer() {
    auto end = std::chrono::high_resolution_clock::now();
    if (!text.empty()) print();
  }

  inline void start() {
    if (!running) {
      t_start = std::chrono::high_resolution_clock::now();
      t_end = std::chrono::high_resolution_clock::now();
      running = true;
    }
  }
  inline void stop() {
    if (running) {
      t_end = std::chrono::high_resolution_clock::now();
      total_elapsed += std::chrono::duration<double, std::milli>(t_end - t_start).count();
      running = false;
    }
  }
  inline void resume() {
    if (!running) {
      start();
    }
  }
  inline std::string format() const {
    double elapsed = total_elapsed;
    if (running)
      elapsed += std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count();

    std::ostringstream os;
    os << elapsed << " ms";
    return os.str();
  }

  inline void print() { std::cout << text << " :" << format() << std::endl; }

  inline timer operator+=(timer &other) {
    timer ret(false);
    ret.total_elapsed += other.total_elapsed;
    return std::move(ret);
  }

  inline timer operator-=(timer &other) {
    timer ret(false);
    ret.total_elapsed -= other.total_elapsed;
    return std::move(ret);
  }

  friend inline timer operator+(const timer &a, const timer &b) {
    timer ret(false);
    ret.total_elapsed = a.total_elapsed + b.total_elapsed;
    return std::move(ret);
  }

  friend inline timer operator-(const timer &a, const timer &b) {
    timer ret(false);
    ret.total_elapsed = a.total_elapsed - b.total_elapsed;
    return std::move(ret);
  }

  // inline double format() const {
  //   double elapsed = total_elapsed;
  //   if (running)
  //     elapsed += std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() -
  //     t_start).count();
  //   return elapsed;
  // }
  // float elapse() {
  // 	auto t_end = std::chrono::high_resolution_clock::now();
  // 	return std::chrono::duration<double,
  // std::milli>(t_end-t.t_start)).count();
  // }

  friend std::ostream &operator<<(std::ostream &os, const timer &t) { return os << t.text << " :" << t.format(); }
};
#else
struct timer {

  timer(bool running = true, std::string text = "") {}

  ~timer() {}
  inline void settext(std::string str) {}
  inline void start() {}
  inline void stop() {}
  inline void resume() {}
  inline std::string format() const { return std::string(); }

  inline timer operator+=(timer &other) { return timer(); }

  inline timer operator-=(timer &other) { return timer(); }

  friend inline timer operator+(const timer &a, const timer &b) { return timer(); }

  friend inline timer operator-(const timer &a, const timer &b) { return timer(); }

  // inline double format() const {
  //   double elapsed = total_elapsed;
  //   if (running)
  //     elapsed += std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() -
  //     t_start).count();
  //   return elapsed;
  // }
  // float elapse() {
  // 	auto t_end = std::chrono::high_resolution_clock::now();
  // 	return std::chrono::duration<double,
  // std::milli>(t_end-t.t_start)).count();
  // }

  friend std::ostream &operator<<(std::ostream &os, const timer &t) { return os; }
};
#endif
}
}
}

#endif
