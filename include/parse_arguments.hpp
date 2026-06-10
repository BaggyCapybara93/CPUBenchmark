#pragma once

#include <string>
#include <boost/program_options.hpp>
#include <thread>

enum class mode{
    single_threaded,
    multi_threaded,
    both,
    invalid,
    none,
};

namespace po = boost::program_options;

class parse_arguments{
    private:
        mode mode_ = mode::invalid;
        bool save_results_ = true;
        std::string save_method_ = "txt"; //Make this into an enum when more methods are added
        int thread_count_ = static_cast<int>(std::thread::hardware_concurrency());
        int iterations_ = 999999;
        int repeat_count_ = 1;
        int matrix_multiply_size_ = 128;
        int intensity_multiplier_ = 50;
        int num_runs_ = 5;

        void parse(int argc, char* argv[]);
    public:
        //Constructor
        parse_arguments(int argc, char* argv[]);

        //Getters
        mode get_mode() const;
        const std::string get_save_method() const {return save_method_;}
        int get_thread_count() const;
        int get_iterations() const;
        int get_repeat_count() const;
        int get_matrix_multiply_size() const;
        int get_intensity_multiplier() const;
        bool should_save_results() const;
        int get_num_runs() const;

};

