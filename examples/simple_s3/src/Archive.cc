/* Copyright 2024- European Centre for Medium-Range Weather Forecasts (ECMWF)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This software was developed as part of the EC H2020 funded project IO-SEA
 * (Project ID: 955811) iosea-project.eu
 */

/// @file   Archive.cc
/// @author Metin Cakircali
/// @date   Feb 2024

#include "Metadata.h"
#include "Utils.h"
#include "dasi/api/Dasi.h"
#include "dasi/api/Key.h"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

constexpr int DATA_SIZE = 24;

/// @brief Data archival using DASI.
int main(int argc, char* argv[]) {
    // user inputs that describe data
    weather::Metadata userInput;

    // the user configuration file
    std::string configFile;

    try {
        // read the user inputs
        const std::vector<std::string> args(argv + 1, argv + argc);
        configFile          = weather::utils::getInputString(args, "-c", "--config", "dasi.yaml");
        userInput.user      = weather::utils::getInputString(args, "-u", "--user", "metin");
        userInput.institute = weather::utils::getInputString(args, "-i", "--institute", "ecmwf");
        userInput.type      = weather::utils::getInputString(args, "-t", "--type");
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    // initialize DASI with user configuration
    dasi::Dasi dasi(configFile.c_str());

    // set of bulk data
    const auto weather = weather::utils::getWeatherData(DATA_SIZE);

    for (auto&& [meta, data] : weather) {
        userInput.parameter = meta.first;
        userInput.level     = meta.second;

        // make a key from userInput
        const dasi::Key key(userInput.print());

        // archive the data with its key
        const auto len = sizeof(double) * data.size();
        dasi.archive(key, data.data(), len);

        std::cout << "Data (length=" << len << ") archived: " << key << "\n";
    }

    return 0;
}
