#pragma once

#include <string>
#include <vector>

bool fileConfigMode(std::string file_address, std::string& date, std::string& stime, std::string& etime, int& experiment_speed, std::vector<std::string>& symbols);
void inputConfigMode(std::string& date, std::string& stime, std::string& etime, int& experiment_speed, std::vector<std::string>& symbols);

//function to start one stock exchange matching, for exchange thread
void createStockMarket(std::string symbol);
