#pragma once

#include <string>
#include <vector>

auto fileConfigMode(std::string filePath, std::string& date, std::string& startTime, std::string& endTime, int& experimentSpeed, std::vector<std::string>& symbols) -> bool;

void inputConfigMode(std::string& date, std::string& startTime, std::string& endTime, int& experimentSpeed, std::vector<std::string>& symbols);
