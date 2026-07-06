#pragma once

#include <string>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <thread>
#include <memory>

#include "settings.hpp"

namespace po = boost::program_options;

class parse_arguments{
    private:
        std::shared_ptr<Settings> settings_;
        void parse(int argc, char* argv[]);
        std::string trim_string(const std::string& value);
        void parse_key_value_map(const std::string& input, std::unordered_map<std::string, double>& target);
    public:
        //Constructor
        parse_arguments(int argc, char* argv[], std::shared_ptr<Settings> settings)
            : settings_(std::move(settings)) {parse(argc, argv);}
};

