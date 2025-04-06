#include <string>
#include <vector>
#include <iostream>

#include "CLI11.hpp"
#include "cache_monitor.h"

std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}


int main(int argc, char **argv) {
    std::string traceFile;
    std::string secretAddressString;
    std::string sampleTimeString;
    std::string thresholdString;
    CLI::App app{"Cache protection block simulator"};
    app.add_option("-a,--address", secretAddressString, "The memory address to protect")->required();
    app.add_option("-s,--sample", sampleTimeString, "The memory address to protect")->required();
    app.add_option("-t,--thres", thresholdString, "The memory address to protect")->required();
    CLI11_PARSE(app, argc, argv);

    //cast the secret address given as argument into an ac_int<32, false>
    ac_int<32, false> secretAddress = std::stoi(secretAddressString, NULL);
    //cast the other needed paramters
    ac_int<32, false> sampleTime = std::stoi(sampleTimeString, NULL);
    ac_int<8, false> threshold = std::stoi(thresholdString, NULL);

    //declare the attack detection flag
    bool detectionFlag;

    //process the memory acess trace file
    ac_int<32, false> address;
    ac_int<32, false> previousCacheMissCycle = 0;
    ac_int<32, false> currentCacheMissCycle = 0;

    //trace line delimiters
    std::string headerDelim = ": ";
    std::string fieldDelim = ", ";
    for (std::string line; std::getline(std::cin, line);) {
        //extract when the next miss occurs and for which address
        std::string data = split(line, ": ")[1];
        currentCacheMissCycle = std::stoi(split(data, ", ")[0], NULL);
        address = std::stoi(split(data, ", ")[1], NULL);

        for(int i=0; i<(currentCacheMissCycle-previousCacheMissCycle); i++) {
            //we don't knwo the address when there's no miss and we tell the monitor that there's no miss detected
            cache_monitor(0, secretAddress, detectionFlag, threshold, sampleTime, false);
        }
        cache_monitor(address, secretAddress, detectionFlag, threshold, sampleTime, true);
        if(detectionFlag == 1) {
            std::cout << "Attack detected on address " << secretAddressString << std::endl;
        }
        previousCacheMissCycle = currentCacheMissCycle;
    }

    return 0;
}


