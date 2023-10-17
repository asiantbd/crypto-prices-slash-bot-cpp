#include <quickchart.h>

using json = nlohmann::json;

// This callback function is called by curl_easy_perform() to write the response
// data into a string
size_t qchart::write_callback(char* ptr, size_t size, size_t nmemb,
                              std::string* data) {
  data->append(ptr, size * nmemb);
  return size * nmemb;
}

std::string qchart::generate_chart(std::string label1, std::vector<long> data1,
                                   std::string label2,
                                   std::vector<long> data2) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Error: could not initialize libcurl" << std::endl;
    return "";
  }
  json datasets = json::array();
  datasets.push_back({{"label", label2}, {"data", data2}});
  json req_body = {{"backgroundColor", "#fff"},
                   {"width", 500},
                   {"height", 300},
                   {"devicePixelRatio", 1.0},
                   {"chart",
                    {{"type", "bar"},
                     {"data", {{"labels", data1}, {"datasets", datasets}}}}}};
  std::cout << to_string(req_body).c_str() << std::endl;
  std::string url = "https://quickchart.io/chart/create";
  struct curl_slist* slist1 = NULL;
  slist1 = curl_slist_append(slist1, "Content-Type: application/json");
  slist1 = curl_slist_append(slist1, "Accept: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, to_string(req_body).c_str());
  std::string response_data;
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
