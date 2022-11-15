//
// Created by Jamie Cotter on 27/10/2022.
//

#include <string>
#include <cstring>
#include <cpprest/json.h>
#include "IPerfTest.h"
extern "C" {
#include "iperf_api.h"
}
#include "../../Constants/CLIENT_DETAILS.h"

namespace utils {
    std::pair<double, double> iPerfTest(std::vector<std::string> hostNames) {
        std::vector<double> bits_per_second;
        for (auto host: hostNames) {
            struct iperf_test *test;
            test = iperf_new_test();
            if (test == NULL) {
                fprintf(stderr, " failed to create test, argv0");
                exit(EXIT_FAILURE);
            }

            iperf_defaults(test);
            iperf_set_test_role(test, 'c');
            char* ccx = new char[host.length() + 1];
            std::copy(host.begin(), host.end(), ccx);

            iperf_set_test_server_hostname(test, ccx);
            iperf_set_test_server_port(test, atoi(IPERF_PORT));

            iperf_set_test_omit(test, 3);
            iperf_set_test_duration(test, 5);
            iperf_set_test_reporter_interval(test, 1);
            iperf_set_test_stats_interval(test, 1);
            iperf_set_test_json_output( test, 1 );

            if (iperf_run_client(test) < 0) {
                fprintf(stderr, "error - %s\n", iperf_strerror(i_errno));
                exit(EXIT_FAILURE);
            }

            auto bimp = iperf_get_test_json_output_string(test);

            web::json::value ret = web::json::value::parse(bimp);

            if (iperf_get_test_json_output_string(test)) {
                fprintf(iperf_get_test_outfile(test), "%zd bytes of JSON emitted\n",
                        strlen(iperf_get_test_json_output_string(test)));
            }

            iperf_free_test( test );

            for(auto i: ret["intervals"].as_array()){
                bits_per_second.push_back(i["streams"][0]["bits_per_second"].as_double());
            }
        }

        double sum = 0.0, mean = 0.0, variance = 0.0, stdDeviation = 0.0;

        for(auto bps: bits_per_second)
            sum += bps;
        mean = sum / bits_per_second.size();
        for(double i : bits_per_second)
            variance += pow(i - mean, 2);

        variance = variance / bits_per_second.size();
        stdDeviation = sqrt(variance);

        return std::make_pair(sum / static_cast<double>(bits_per_second.size()), stdDeviation);
    }
} // utils