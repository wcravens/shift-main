#pragma once

#include <string>
#include <vector>

void debug_mode(std::vector<std::string>& symbols, std::string& date, std::string& stime, std::string& etime, double& experiment_speed);
bool fileConfig_mode(std::string file_address, std::vector<std::string>& symbols, std::string& date, std::string& stime, std::string& etime, double& experiment_speed);
void inputConfig_mode(std::vector<std::string>& symbols, std::string& date, std::string& stime, std::string& etime, double& experiment_speed);

//function to start one stock exchange matching, for exchange thread
void createStockMarket(std::string symbol);
