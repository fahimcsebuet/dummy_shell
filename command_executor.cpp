#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "command_executor.h"
#include "utility.h"

command_executor::command_executor()
{
    this->p_command = p_command;
}

command_executor::~command_executor()
{
}

int command_executor::execute(command* p_command)
{
    std::string _command_name = p_command->get_name();
    std::vector<std::string> _command_arguments = p_command->get_arguments();
    if(_command_name == "exit")
    {
        return -1;
    }
    else if(_command_name == "set")
    {
        if(_command_arguments.empty())
        {
            utility::print_environ();
        }
        else if(_command_arguments.size() > 1)
        {
            std::cout << "Invalid Arguments!" << std::endl;
            return 1;
        }
        else // _command_arguments.size() == 1
        {
            std::string _argument = _command_arguments.at(0);
            std::vector<std::string> _splitted_arguments = utility::split_string(_argument, '=');
            if(_splitted_arguments.size() != 2)
            {
                std::cout << "Invalid Arguments!" << std::endl;
                return 1;
            }
            else // exactly one '=' sign in the argument
            {
                setenv(_splitted_arguments.at(0).c_str(), _splitted_arguments.at(1).c_str(), 1);
            }
        }
    }
    else if(_command_name == "pwd")
    {
        if(!_command_arguments.empty())
        {
            std::cout << "Invalid Arguments!" << std::endl;
            return 1;
        }
        std::string _current_directory = utility::get_current_directory();
        if(!_current_directory.empty())
        {
            std::cout << _current_directory << std::endl;
        }
        else
        {
            std::cout << "Error: pwd command" << std::endl;
        }
    }
    else if(_command_name == "cd")
    {
        std::string _home_directory = utility::get_environment_variable("HOME");
        std::string _target_directory;
        if(_command_arguments.empty())
        {
            _target_directory = _home_directory;
        }
        else if(_command_arguments.size() > 1)
        {
            std::cout << "Error: Invalid arguments. See man cd" << std::endl;
            return 1;
        }
        else
        {
            _target_directory = _command_arguments.at(0);
            std::string _tilt_string = "~";
            std::size_t _tilt_position = _target_directory.find(_tilt_string);
            if(_tilt_position != std::string::npos)
            {
                _target_directory.replace(_tilt_position, _tilt_string.size(), _home_directory);
            }
            if(_target_directory.empty())
            {
                std::cout << "Error: Invalid arguments. See man cd" << std::endl;
                return 1;
            }
        }
        int _out_chdir = chdir(_target_directory.c_str());
        if(_out_chdir < 0)
        {
            std::cout << "Error: cd command" << std::endl;
            return 1;
        }
    }
    else if(!_command_name.empty()) // handles all commands and myls
    {
        return utility::execute_command(_command_name, _command_arguments);
    }

    return 0;
}

int command_executor::execute(piped_command* p_piped_command)
{
    return utility::execute_piped_commands(p_piped_command->get_commands_list());
}
