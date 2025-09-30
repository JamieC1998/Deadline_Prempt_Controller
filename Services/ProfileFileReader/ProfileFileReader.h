//
// Created by Jamie Cotter on 21/07/2025.
//

#ifndef CONTROLLER_PROFILEFILEREADER_H
#define CONTROLLER_PROFILEFILEREADER_H

#include <cpprest/json.h>
#include "../../model/data_models/SimEvents/SimEvents.h"

namespace services {

    web::json::value read_experiment_log(const std::string& profileInputPath);

	std::pair<std::vector<std::string>, std::vector<std::shared_ptr<model::SimEvent>>> parse_experiment_log(web::json::value jObj);

} // services

#endif //CONTROLLER_PROFILEFILEREADER_H
