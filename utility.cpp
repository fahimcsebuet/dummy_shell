#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <dirent.h>
#include <wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "utility.h"

int utility::input_redirection_file = -1;
int utility::output_redirection_file = -1;
int utility::std_in_file = -1;
int utility::std_out_file = -1;
bool utility::background_process_finished = false;

std::string utility::get_current_user_name()
{
    struct passwd *password;
    unsigned int _user_id = geteuid();
    if((password = getpwuid(_user_id)) != NULL)
    {
        return std::string(password->pw_name);
    }
    else
    {
        char uid_char_array[80];
        sprintf(uid_char_array, "%d", _user_id);
        return std::string(uid_char_array);
    }
}

std::string utility::get_home_directory()
{
    struct passwd *password;
    unsigned int _user_id = geteuid();
    if((password = getpwuid(_user_id)) != NULL)
    {
        return std::string(password->pw_dir);
    }
    return "";
}

void utility::trim_string(std::string& splitted_command_string)
{
    size_t _left_trim_first = splitted_command_string.find_first_not_of(' ');
    if(_left_trim_first == std::string::npos)
    {
        splitted_command_string =  "";
        return;
    }

    size_t _right_trim_last = splitted_command_string.find_last_not_of(' ');
    splitted_command_string = splitted_command_string.substr(_left_trim_first, (_right_trim_last -
        _left_trim_first)+1);
}

std::vector<std::string> utility::split_string(const std::string& command_string, char delimiter)
{
    std::vector<std::string> _splitted_string_vector;

    std::stringstream _command_string_stream(command_string);
    std::string _splitted_string;

    while(std::getline(_command_string_stream, _splitted_string, delimiter))
    {
        if(delimiter == ' ' || delimiter == '<' || delimiter == '>') trim_string(_splitted_string);
        if(!_splitted_string.empty())
        {
            _splitted_string_vector.push_back(_splitted_string);
        }
    }

    return _splitted_string_vector;
}

std::vector<std::string> utility::parse(const std::string& command_string)
{
    std::vector<std::string> _parsed_vector = split_string(command_string, ' ');
    return _parsed_vector;
}

void utility::print_environ()
{
    extern char** environ;

    int _i = 0;
    while (environ[_i])
    {
        std::cout << environ[_i++] << std::endl;
    }
}

void utility::print_command_prefix()
{
    std::string _user = get_current_user_name();
    char _hostname_char_array[HOST_NAME_MAX];
    gethostname(_hostname_char_array, HOST_NAME_MAX);
    std::string _hostname(_hostname_char_array);
    std::string _user_home = get_home_directory();
    std::string _current_directory = get_current_directory();
    int _home_position = _current_directory.find(_user_home);
    if(_home_position >= 0) _current_directory.replace(_home_position, _user_home.size(), "~");
    std::string _symbol = std::string("$ ");
    if(_user == "root")
    {
        _symbol = std::string("# ");
    }

    if(background_process_finished)
    {
        std::cout << "[Done] - Background Process" << std::endl;
        background_process_finished = false;
    }
    std::string _command_prefix = std::string("[") + _user + std::string("@") + _hostname + std::string(":")
        + _current_directory + std::string("]") + _symbol;
    std::cout << _command_prefix;
}

void utility::print_supported_commands()
{
    std::cout << "Supported Commands: exit, set, pwd, cd" << std::endl;
}

std::string utility::get_current_directory()
{
    long _absolute_path_size = pathconf(".", _PC_PATH_MAX);
    char *p_absolute_path_buffer;
    char *p_out_get_cwd;
    if((p_absolute_path_buffer = (char *)malloc((size_t)_absolute_path_size)) != NULL)
    {
        p_out_get_cwd = getcwd(p_absolute_path_buffer, (size_t) _absolute_path_size);
        if(p_out_get_cwd != NULL)
        {
            return std::string(p_absolute_path_buffer);
        }
        else
        {
            return "";
        }
    }
    return "";
}

std::string utility::get_environment_variable(const std::string& variable_name)
{
    char* p_variable_value = getenv(variable_name.c_str());
    if(p_variable_value)
    {
        return std::string(p_variable_value);
    }

    return "";
}

bool utility::is_piped_command(std::string command_string)
{
    std::size_t _first_appearance = command_string.find_first_of("|");
    return (_first_appearance != std::string::npos);
}

bool utility::is_file_input_command(std::string command_string)
{
    std::size_t _first_appearance = command_string.find_first_of("<");
    return (_first_appearance != std::string::npos);
}

bool utility::is_file_output_command(std::string command_string)
{
    std::size_t _first_appearance = command_string.find_first_of(">");
    return (_first_appearance != std::string::npos);
}

bool utility::is_file_in_directory(std::string filename, std::string directory_name)
{
    struct dirent *_directory_entry;
    DIR *p_dir = opendir(directory_name.c_str());

    if(p_dir == NULL)
    {
        return false;
    }

    while((_directory_entry = readdir(p_dir)) != NULL)
    {
        std::string _filename(_directory_entry->d_name);

        if(!_filename.empty() && _filename.at(0) == '.')
        {
            continue;
        }

        if(_filename == filename)
        {
            return true;
        }
    }

    return false;
}

void utility::create_argv_from_arguments_vector(const std::vector<std::string> arguments_vector, char** argv)
{
    for(unsigned int i=0; i < arguments_vector.size(); i++)
    {
        argv[i+1] = new char[arguments_vector.at(i).size() + 1];
        strcpy(argv[i+1], arguments_vector.at(i).c_str());
    }
}

int utility::execute_command_with_job_control(std::string command_name, std::string command_path, 
    std::vector<std::string> command_arguments, bool is_input_from_file)
{
    bool _background_process = false;

    if(!command_arguments.empty())
    {
        _background_process = (command_arguments.back() == "&");
        if(command_arguments.back() != "&")
        {
            std::string _last_argument = command_arguments.back();
            _background_process = (!_last_argument.empty() && _last_argument.at(_last_argument.size() - 1) == '&');
            if(_background_process && !command_arguments.back().empty()) command_arguments.back().erase(command_arguments.back().size()-1);
        }
        if(_background_process) command_arguments.pop_back();
    }
    else
    {
        _background_process = (!command_name.empty() && command_name.at(command_name.size() - 1) == '&');
        if(_background_process && !command_name.empty()) command_name.erase(command_name.size()-1);
    }

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    // myshell is the process group leader
    if(setpgid(getpid(), getpid()) < 0)
    {
        exit (1);
    }

    // transfer terminal control from linux shell to myshell
    if(!is_input_from_file)
    if(tcsetpgrp(STDIN_FILENO, getpgrp()) < 0)
    {
        return 1;
    }
    int _execution_status = 0;
    pid_t _child_process_id = fork();
    if(_child_process_id == 0)
    {
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, SIG_DFL);

        // child process is the process group leader
        if(setpgid(0,0) < 0)
        {
            return 1;
        }

        // transfer terminal control to child process
        if(!is_input_from_file)
        if(tcsetpgrp (STDIN_FILENO, getpgrp()) < 0)
        {
            return 1;
        }

        std::string command_full_path = command_path + "/" + command_name;
        unsigned int _argv_size = command_arguments.size() + 2;
        char** _argv = new char*[_argv_size];
        _argv[0] = new char[command_full_path.size() + 1];
        strcpy(_argv[0], command_full_path.c_str());
        create_argv_from_arguments_vector(command_arguments, _argv);
        _argv[_argv_size - 1] = NULL;
        if(execv(_argv[0], &_argv[0]) < 0)
        {
            std::cout << "Error: command " << command_name << " failed" << std::endl;
            for(unsigned int i=0; i < _argv_size; i++)
            {
                delete [] _argv[i];
            }
            delete [] _argv;
            return 1;
        }

        for(unsigned int i=0; i < _argv_size; i++)
        {
            delete [] _argv[i];
        }
        delete [] _argv;

        std::cout << "Unsupported Command" << std::endl;
        return 1;
    }
    else if(_child_process_id > 0)
    {
        signal(SIGCHLD, sigchild_handler);
        signal(SIGINT, SIG_DFL);

        // make child process its own process group leader for avoiding race conditions
        if(setpgid(_child_process_id, _child_process_id) < 0)
        {
            if(errno != EACCES)
            {
                return 1;
            }
        }

        if(!_background_process)
        {
            // transfer controlling terminal
            if(!is_input_from_file)
            if (tcsetpgrp(STDIN_FILENO, getpgid(_child_process_id)) < 0)
            {
                return 1;
            }

            kill(_child_process_id, SIGCONT);

            wait(&_execution_status);

            // put the shell back in the foreground
            if(!is_input_from_file)
            if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0)
            {
                return 1;
            }
        }
        if(_execution_status >> 8 == 0xff)
        {
            std::cout << "Error: command " << command_name << " failed" << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "Error: Failed to fork" << std::endl;
        return 1;
    }

    return 0;
}

int utility::execute_command(std::string command_name, std::vector<std::string> command_arguments)
{
    bool _background_process = false;
    signal(SIGCHLD, SIG_IGN);

    if(!command_arguments.empty())
    {
        // handle when without space
        _background_process = (command_arguments.back() == "&");
        if(command_arguments.back() != "&")
        {
            std::string _last_argument = command_arguments.back();
            _background_process = (!_last_argument.empty() && _last_argument.at(_last_argument.size() - 1) == '&');
            if(_background_process && !command_arguments.back().empty()) command_arguments.back().erase(command_arguments.back().size()-1);
        }
        else
        {
            _background_process = true;
            command_arguments.pop_back();
        }
    }
    else
    {
        _background_process = (!command_name.empty() && command_name.at(command_name.size() - 1) == '&');
        if(_background_process && !command_name.empty()) command_name.erase(command_name.size()-1);
    }

    std::string _environment_variable = "PATH";
    if(command_name == "myls") _environment_variable = "MYPATH";
    std::string _command_path = utility::get_command_path_from_environment_variable(command_name, _environment_variable);
    if(_command_path.empty())
    {
        std::cout << "Could not find "<< command_name << " in the computer" << std::endl;
        return 1;
    }

    int _execution_status = 0;
    pid_t _child_process_id = fork();
    if(_child_process_id == 0)
    {
        signal(SIGCHLD, SIG_DFL);
        std::string command_full_path = _command_path + "/" + command_name;
        unsigned int _argv_size = command_arguments.size() + 2;
        char** _argv = new char*[_argv_size];
        _argv[0] = new char[command_full_path.size() + 1];
        strcpy(_argv[0], command_full_path.c_str());
        create_argv_from_arguments_vector(command_arguments, _argv);
        _argv[_argv_size - 1] = NULL;
        if(execv(_argv[0], &_argv[0]) < 0)
        {
            std::cout << "Error: command " << command_name << " failed" << std::endl;
            for(unsigned int i=0; i < _argv_size; i++)
            {
                delete [] _argv[i];
            }
            delete [] _argv;
            return 1;
        }

        for(unsigned int i=0; i < _argv_size; i++)
        {
            delete [] _argv[i];
        }
        delete [] _argv;

        std::cout << "Unsupported Command" << std::endl;
        return 1;
    }
    else if(_child_process_id > 0)
    {
        if(!_background_process)
        {
            wait(&_execution_status);
            signal(SIGCHLD, SIG_DFL);
        }
        else
        {
            signal(SIGCHLD, sigchild_handler);
        }

        if(_execution_status >> 8 == 0xff)
        {
            std::cout << "Error: command " << command_name << " failed" << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "Error: Failed to fork" << std::endl;
        return 1;
    }

    return 0;
}

int utility::execute_piped_command(int input_fd, int output_fd, command* p_command)
{
    std::string _command_name = p_command->get_name();
    pid_t _process_id = fork();

    if (_process_id == 0)
    {
        if (input_fd != STDIN_FILENO)
        {
            dup2 (input_fd, STDIN_FILENO);
            close (input_fd);
        }

        if (output_fd != STDOUT_FILENO)
        {
            dup2 (output_fd, STDOUT_FILENO);
            close (output_fd);
        }

        std::vector<std::string> _command_arguments = p_command->get_arguments();
        std::string _environment_variable = "PATH";
        if(_command_name == "myls") _environment_variable = "MYPATH";
        std::string _command_path = utility::get_command_path_from_environment_variable(_command_name, _environment_variable);
        if(_command_path.empty())
        {
            std::cout << "Could not find "<< _command_name << " in the computer" << std::endl;
            return 1;
        }
        std::string _command_full_path = _command_path + "/" + _command_name;
        unsigned int _argv_size = _command_arguments.size() + 2;
        char** _argv = new char*[_argv_size];
        _argv[0] = new char[_command_full_path.size() + 1];
        strcpy(_argv[0], _command_full_path.c_str());
        create_argv_from_arguments_vector(_command_arguments, _argv);
        _argv[_argv_size - 1] = NULL;

        if(execv(_argv[0], &_argv[0]) < 0)
        {
            std::cout << "Error: command " << _command_name << " failed" << std::endl;
            for(unsigned int i=0; i < _argv_size; i++)
            {
                delete [] _argv[i];
            }
            delete [] _argv;
            return 1;
        }

        for(unsigned int i=0; i < _argv_size; i++)
        {
            delete [] _argv[i];
        }
        delete [] _argv;

        std::cout << "Unsupported Command" << std::endl;
        return 1;
    }
    else if(_process_id > 0)
    {
        int _execution_status;
        wait(&_execution_status);
        if(_execution_status >> 8 == 0xff)
        {
            std::cout << "Error: command " << _command_name << " failed" << std::endl;
            return 1;
        }
    }
    else if(_process_id < 0)
    {
        std::cout << "Error: Failed to fork" << std::endl;
        return 1;
    }

    return _process_id;
}

int utility::execute_piped_commands(std::vector<command*> commands_list)
{
    if(!is_valid_pipe_command(commands_list)) return 1;
    unsigned int _number_of_commands = commands_list.size();
    int _process_id;
    int _input_fd = STDIN_FILENO; // initial input file
    int _fds[2];
    unsigned int i;
    for(i = 0; i < _number_of_commands - 1; ++i)
    {
        pipe(_fds);
        _process_id = execute_piped_command(_input_fd, _fds[1], commands_list.at(i));
        close(_fds[1]);
        _input_fd = _fds[0];
    }

    _process_id = fork();

    command* p_command = commands_list.back();
    std::string _command_name = p_command->get_name();
    if(_process_id == 0)
    {
        if(_input_fd != STDIN_FILENO)
        {
            dup2(_input_fd, STDIN_FILENO);
        }
        std::vector<std::string> _command_arguments = p_command->get_arguments();
        std::string _environment_variable = "PATH";
        if(_command_name == "myls") _environment_variable = "MYPATH";
        std::string _command_path = utility::get_command_path_from_environment_variable(_command_name, _environment_variable);
        if(_command_path.empty())
        {
            std::cout << "Could not find "<< _command_name << " in the computer" << std::endl;
            return 1;
        }
        std::string _command_full_path = _command_path + "/" + _command_name;
        unsigned int _argv_size = _command_arguments.size() + 2;
        char** _argv = new char*[_argv_size];
        _argv[0] = new char[_command_full_path.size() + 1];
        strcpy(_argv[0], _command_full_path.c_str());
        create_argv_from_arguments_vector(_command_arguments, _argv);
        _argv[_argv_size - 1] = NULL;

        if(execv(_argv[0], &_argv[0]) < 0)
        {
            std::cout << "Error: command " << _command_name << " failed" << std::endl;
            for(unsigned int i=0; i < _argv_size; i++)
            {
                delete [] _argv[i];
            }
            delete [] _argv;
            return 1;
        }

        for(unsigned int i=0; i < _argv_size; i++)
        {
            delete [] _argv[i];
        }
        delete [] _argv;

        std::cout << "Unsupported Command" << std::endl;
        return 1;
    }
    else if(_process_id > 0)
    {
        int _execution_status;
        wait(&_execution_status);
        if(_execution_status >> 8 == 0xff)
        {
            std::cout << "Error: command " << _command_name << " failed" << std::endl;
            return 1;
        }
    }
    else if(_process_id < 0)
    {
        std::cout << "Error: Failed to fork" << std::endl;
        return 1;
    }
    return 0;
}

bool utility::is_valid_pipe_command(std::vector<command*> commands_list)
{
    for(unsigned int i=0; i < commands_list.size(); i++)
    {
        std::string _command_name = commands_list.at(i)->get_name();
        std::string _environment_variable = "PATH";
        if(_command_name == "myls") _environment_variable = "MYPATH";
        std::string _command_path = utility::get_command_path_from_environment_variable(_command_name, _environment_variable);
        if(_command_path.empty())
        {
            std::cout << "Invalid Piped Command" << std::endl;
            return false;
        }
    }

    return true;
}

void utility::sigchild_handler(int signal)
{
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0);
    background_process_finished = true;
    std::cout << "[Finished] - Background Process" << std::endl;
}

std::string utility::get_command_path_from_environment_variable(std::string command_name, std::string environment_variable)
{
    std::string _command_path = "";
    std::string _environment_variable_value = utility::get_environment_variable(environment_variable);
    if(_environment_variable_value.empty())
    {
        std::cout << "Environment variable " << environment_variable << " not set. Please set the path to myls in MYPATH" << std::endl;
        return "";
    }
    std::vector<std::string> _splitted_values = utility::split_string(_environment_variable_value, ':');
    unsigned int _splitted_values_size = _splitted_values.size();
    if(_splitted_values_size > 0)
    {
        for(unsigned int i = 0; i < _splitted_values.size(); i++)
        {
            if(utility::is_file_in_directory(command_name, _splitted_values.at(i)))
            {
                _command_path = _splitted_values.at(i);
                break;
            }
        }
    }
    else
    {
        if(utility::is_file_in_directory(command_name, _environment_variable_value))
        {
            _command_path = _environment_variable_value;
        }
    }

    return _command_path;
}

int utility::init_handle_file_redirection(std::vector<std::string>& command_arguments)
{
    if(command_arguments.size() < 2)
    {
        return 1;
    }

    bool _file_redirection_input = false;
    bool _file_redirection_output = false;
    std::string _input_file_full_path = "";
    std::string _output_file_full_path = "";
    // handle when without space
    int _second_last_index = command_arguments.size() - 2;
    _file_redirection_input = (command_arguments.at(_second_last_index) == "<");
    _file_redirection_output = (command_arguments.at(_second_last_index) == ">");
    if(_file_redirection_input)
    {
        _input_file_full_path = command_arguments.back();
        std_in_file = dup(STDIN_FILENO);
        close(STDIN_FILENO);
        input_redirection_file = open(_input_file_full_path.c_str(), O_RDWR, 0777);
        if(input_redirection_file < 0)
        {
            return -1;
        }
    }
    if(_file_redirection_output)
    {
        _output_file_full_path = command_arguments.back();
        std_out_file = dup(STDOUT_FILENO);
        close(STDOUT_FILENO);
        output_redirection_file = open(_output_file_full_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
        if(output_redirection_file < 0)
        {
            return -1;
        }
    }
    if(_file_redirection_input || _file_redirection_output)
    {
        // Remove ">" or "<"
        command_arguments.pop_back();
        // Remove file name
        command_arguments.pop_back();
    }

    return 0;
}

int utility::init_handle_file_redirection(std::string& command_string)
{
    bool _file_redirection_input = is_file_input_command(command_string);
    bool _file_redirection_output = is_file_output_command(command_string);

    if((!_file_redirection_input && !_file_redirection_output) || (_file_redirection_input && _file_redirection_output))
    {
        return 1;
    }

    std::string _input_file_full_path = "";
    std::string _output_file_full_path = "";

    std::vector<std::string> _command_string_vector;
    if(_file_redirection_input) _command_string_vector = split_string(command_string, '<');
    if(_file_redirection_output) _command_string_vector = split_string(command_string, '>');
    if(_command_string_vector.size() != 2) return 1;

    if(_file_redirection_input)
    {
        _input_file_full_path = _command_string_vector.back();
        std_in_file = dup(STDIN_FILENO);
        close(STDIN_FILENO);
        input_redirection_file = open(_input_file_full_path.c_str(), O_RDONLY, 0777);
        if(input_redirection_file < 0)
        {
            return -2;
        }
    }
    if(_file_redirection_output)
    {
        _output_file_full_path = _command_string_vector.back();
        std_out_file = dup(STDOUT_FILENO);
        close(STDOUT_FILENO);
        output_redirection_file = open(_output_file_full_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
        if(output_redirection_file < 0)
        {
            return -3;
        }
    }
    if(_file_redirection_input || _file_redirection_output)
    {
        command_string = _command_string_vector.front();
    }

    return 0;
}

void utility::close_handle_file_redirection()
{
    if(input_redirection_file >= 0)
    {
        close(input_redirection_file);
        dup2(std_in_file, STDIN_FILENO);
        input_redirection_file = -1;
        std_in_file = -1;
    }
    if(output_redirection_file >= 0)
    {
        close(output_redirection_file);
        dup2(std_out_file, STDOUT_FILENO);
        output_redirection_file = -1;
        std_out_file = -1;
    }
}
