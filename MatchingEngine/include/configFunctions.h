#pragma once

#include <string>
#include <vector>

bool fileConfigMode(std::string filePath, std::string& date, std::string& startTime, std::string& endTime, int& experimentSpeed, std::vector<std::string>& symbols);

void inputConfigMode(std::string& date, std::string& startTime, std::string& endTime, int& experimentSpeed, std::vector<std::string>& symbols);
