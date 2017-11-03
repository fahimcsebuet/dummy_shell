#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include <vector>

#include "command.h"

class utility
{
private:
    static std::string get_current_user_name();
    static std::string get_home_directory();
    static void trim_string(std::string& splitted_command_string);
public:
    static std::vector<std::string> split_string(const std::string& command_string, char delimiter);
    static std::vector<std::string> parse(const std::string& command_string);
    static void print_environ();
    static void print_command_prefix();
    static void print_supported_commands();
    static std::string get_current_directory();
    static std::string get_environment_variable(const std::string& variable_name);
    static bool is_piped_command(std::string command_string);
    static bool is_file_input_command(std::string command_string);
    static bool is_file_output_command(std::string command_string);
    static bool is_file_in_directory(std::string filename, std::string directory_name);
    static void create_argv_from_arguments_vector(const std::vector<std::string> arguments_vector, char** argv);

    // could not show output when in background
    static int execute_command_with_job_control(std::string command_name, std::string command_path, 
        std::vector<std::string> command_arguments, bool is_input_from_file);

    static int execute_command(std::string command_name, std::vector<std::string> command_arguments);

    static int execute_piped_command(int input_fd, int output_fd, command* p_command);
    static int execute_piped_commands(std::vector<command*> commands_list);
    static bool is_valid_pipe_command(std::vector<command*> commands_list);
    static void sigchild_handler(int signal);
    static std::string get_command_path_from_environment_variable(std::string command_name, std::string environment_variable);
    static int init_handle_file_redirection(std::vector<std::string>& command_arguments);
    static int init_handle_file_redirection(std::string& command_string);
    static void close_handle_file_redirection();
    static int input_redirection_file;
    static int output_redirection_file;
    static int std_in_file;
    static int std_out_file;
    static bool background_process_finished;
};

#endif

