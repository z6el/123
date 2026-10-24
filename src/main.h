#pragma once
#include <src/include.h>
#include "bytes/driver.h"
#include "bytes/mapper.h"

bool WriteBytesToFile(const std::string& filePath, const std::vector<unsigned char>& data)
{
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open())
    {
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outFile.close();
    return true;
}

void map_driver()
{
    std::string driver_path = "C:\\Windows\\System32\\Tasks\\driver.sys";
    std::string mapper_path = "C:\\Windows\\System32\\Tasks\\mapper.exe";

    WriteBytesToFile(driver_path, driver_bytes);
    WriteBytesToFile(mapper_path, mapper_bytes);

    system("C:\\Windows\\System32\\Tasks\\mapper.exe C:\\Windows\\System32\\Tasks\\driver.sys");
}