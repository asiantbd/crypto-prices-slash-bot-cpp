#include <quickchart.h>

using json = nlohmann::json;

// This callback function is called by curl_easy_perform() to write the response
// data into a string
size_t qchart::write_callback(char* ptr, size_t size, size_t nmemb,
                              std::string* data) {
  data->append(ptr, size * nmemb);
  return size * nmemb;
}

std::string qchart::generate_chart(std::vector<long> data1, std::string label,
                                   std::vector<double> data2) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Error: could not initialize libcurl" << std::endl;
    return "";
  }
  json datasets = json::array();
  datasets.push_back({{"label", label}, {"data", data2}});
  json req_body = {{"backgroundColor", "#fff"},
                   {"width", 500},
                   {"height", 500},
                   {"devicePixelRatio", 1.0},
                   {"chart",
                    {{"type", "line"},
                     {"data", {{"labels", data1}, {"datasets", datasets}}}}}};
  std::string request = req_body.dump();
  std::cout << request << std::endl;
  std::string url = "https://quickchart.io/chart/create";
  std::string response_data;
  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "charset: utf-8");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "Error: curl_easy_perform() failed: "
              << curl_easy_strerror(res) << std::endl;
    curl_easy_cleanup(curl);
    return "";
  }

  try {
    json response_json = json::parse(response_data);
    std::cout << "=============================" << std::endl;
    std::cout << ">> quickchart response: " << response_json << std::endl;
    return response_json["url"];
  } catch (const std::exception& e) {
    std::cerr << "Error: failed to parse JSON response: " << e.what()
              << std::endl;
    curl_easy_cleanup(curl);
    return "";
  }

  curl_easy_cleanup(curl);
  return "";
}
