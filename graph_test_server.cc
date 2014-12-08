// When the server is running, use the following command in Ubuntu to see the example chart:
// curl -s http://localhost:8080/data | grep -e "^            " | sed "s/,//" | graph -T X -a -g 3 -C -W 0.004 -L "Example"

#include <thread>

#include "cereal/include/archives/json.hpp"
#include "cereal/include/types/vector.hpp"
#include "cereal/include/types/string.hpp"

#include "network/posix_http_server.h"

const int kPort = 8080;

struct Meta {
 public:
  // TODO(sompylasar+dkorolev): Discuss and add more sophisticated parameters, ex., ticks, labels, grid, etc.
  std::string data_url = "http://localhost:8080/data";
  std::string line_color = "blue";
  double line_width = 3.5;
  template <class A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(data_url), CEREAL_NVP(line_color), CEREAL_NVP(line_width));
  }
};

struct DataXY {
 public:
  double x;
  double y;
  
  DataXY(double x, double y) : x(x), y(y) {
  }
  
  template <class A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(x), CEREAL_NVP(y));
  }
};

struct Data {
 public:
  // TODO(sompylasar+dkorolev): Discuss whether value-only is good, or shall we pass in { x, y } pairs.
  // TODO(sompylasar+dkorolev): Discuss whether any metadata should be returned as well.
  std::vector<DataXY> data;
  template <class A>
  void serialize(A& ar) {
    ar(CEREAL_NVP(data));
  }
};

unsigned long long GetNowInMilliseconds() {
  typedef std::chrono::system_clock clk;
  unsigned long long now =
    std::chrono::duration_cast<std::chrono::milliseconds>(clk::now().time_since_epoch()).count();
  return now;
}

inline unsigned long long ConvertSecondsToMilliseconds(time_t seconds) {
  return (seconds * 1000);
}

int main() {
  Socket s(kPort);
  while (true) {
    std::thread([](HTTPConnection c) {
                  HTTPHeadersType extra_headers;
                  extra_headers.push_back(std::make_pair("Access-Control-Allow-Origin", "*"));
                  if (c.Method() == "GET" && (c.URL() == "/meta" || c.URL() == "/meta/")) {
                    // TODO(dkorolev+sompylasar): Discuss multiple URLs for various scales.
                    std::ostringstream os;
                    (cereal::JSONOutputArchive(os))(Meta());
                    c.SendHTTPResponse(os.str(), HTTPResponseCode::OK, "application/json", extra_headers);
                  } else if (c.Method() == "GET" && (c.URL() == "/data" || c.URL() == "/data/")) {
                    // TODO(dkorolev): Add URL parsing to support partial data.
                    Data data;
                    const int data_length = 1000;
                    for (int i = 0, ic = data_length; i < ic; ++i) {
                      data.data.push_back(DataXY(GetNowInMilliseconds() - ConvertSecondsToMilliseconds(ic - i),
                                                 sin(0.01 * i) + sin(0.04 * i) + exp(0.001 * i)));
                    }
                    std::ostringstream os;
                    (cereal::JSONOutputArchive(os))(data);
                    c.SendHTTPResponse(os.str(), HTTPResponseCode::OK, "application/json", extra_headers);
                  } else {
                    c.SendHTTPResponse("Something's wrong.", HTTPResponseCode::NotFound);
                  }
                },
                s.Accept()).detach();
  }
}
