#pragma comment (lib, "Ws2_32.lib")
extern "C" {
#include "mongoose.h"
}
#include<iostream>
#include<map>
#include<string>
#include"json.hpp"  

using namespace std;
using json = nlohmann::json;

static const string POST = "POST";
static const string PUT = "PUT";
static const string DEL = "DELETE";
static const string GET = "GET";
static int id = 1;
static map<int, json> jsons;
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {

	struct mg_http_serve_opts opts = { opts.root_dir = "." };   
	if (ev == MG_EV_HTTP_MSG) {
		struct mg_http_message *hm = (struct mg_http_message *) ev_data;
		if (mg_http_match_uri(hm, "/api")) {
			string method = hm->method.ptr;
			if (POST.compare(method.substr(0, hm->method.len)) == 0) {
				json body = json::parse(((string)hm->body.ptr).substr(0, hm->body.len));
				jsons[id++] = body;
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", to_string(id - 1).c_str());
			}
			else if (GET.compare(method.substr(0, hm->method.len)) == 0) {
				if (hm->query.len > 0) {
					string query = ((string)hm->query.ptr).substr(0, hm->query.len);
					string key = "";
					string value = "";
					int indexEq = query.find("key=");
					if (query.find('&')) {
						int index = query.find('&');
						int indexVal = query.find("value=");
						key = query.substr(indexEq + 4, index - indexEq - 4);
						value = query.substr(indexVal + 6, query.length() - indexVal - 6);
					}
					else {
						key = query.substr(indexEq + 4, query.length() - indexEq - 4);
					}
					string result = "[";
					for (auto it = jsons.cbegin(); it != jsons.cend(); ++it)
					{
						json current = it->second;
						if (current.contains(key)) {
							result = result.append(current.dump());
							if (next(it, 1) != jsons.cend()) {
								result = result.append(",");
							}
						}
						else {
							if (value != "") {
								for (auto itCurr : current)
								{
									string currValue = itCurr.dump();
									string toReplace("\"");
									size_t pos = currValue.find_first_of(toReplace);
									if (pos != std::string::npos) {
										currValue = currValue.replace(pos, toReplace.length(), "");
									}
									pos = currValue.find_last_of(toReplace);
									if (pos != std::string::npos) {
										currValue = currValue.replace(pos, toReplace.length(), "");
									}

									if (currValue.compare(value) == 0) {
										result = result.append(current.dump());
										if (next(it, 1) != jsons.cend()) {
											result = result.append(",");
										}
										break;
									}
								}

							}
						}

					}
					result = result.append("]");
					mg_http_reply(c, 200, "Content-Type: application/json\r\n", result.c_str());
				}
				else {
					string result = "[";
					for (auto it = jsons.cbegin(); it != jsons.cend(); ++it)
					{
						if (it->second.is_null()) {
							continue;
						}
						
						result = result.append(it->second.dump());
						if (next(it, 1) != jsons.cend()) {
							result = result.append(",");
						}
					}
					result = result.append("]");

					mg_http_reply(c, 200, "Content-Type: application/json\r\n", result.c_str());
				}
			}
		}
		else if (mg_http_match_uri(hm, "/api/*")) {
			string method = hm->method.ptr;
			if (PUT.compare(method.substr(0, hm->method.len)) == 0) {
				json body = json::parse(((string)hm->body.ptr).substr(0, hm->body.len));
				int uriLen = hm->uri.len;
				int idToUpdate = atoi(((string)hm->uri.ptr).substr(0, uriLen).substr(5).c_str());
				json existing = jsons[idToUpdate];
				if (existing.is_null()) {
					mg_http_reply(c, 200, "Content-Type: application/json\r\n", "not found");
				}
				else {
					jsons[idToUpdate] = body;
					mg_http_reply(c, 200, "Content-Type: application/json\r\n", "successfully updated");
				}
			}
			else if (DEL.compare(method.substr(0, hm->method.len)) == 0) {
				int uriLen = hm->uri.len;
				int idToDel = atoi(((string)hm->uri.ptr).substr(0, uriLen).substr(5).c_str());
				jsons.erase(jsons.find(idToDel));
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", "successfully deleted");
			}
			else if (GET.compare(method.substr(0, hm->method.len)) == 0) {
				int uriLen = hm->uri.len;
				int idToGet = atoi(((string)hm->uri.ptr).substr(0, uriLen).substr(5).c_str());
				json res = jsons[idToGet];
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", res.is_null() ? "not found" : res.dump().c_str());
			}

		}
		mg_http_serve_dir(c, (mg_http_message*)ev_data, &opts);
	}
}


int main() {

	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_http_listen(&mgr, "0.0.0.0:8000", fn, NULL);    
	for (;;) mg_mgr_poll(&mgr, 1000);                  
}