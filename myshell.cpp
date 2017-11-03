#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include "command.h"
#include "command_executor.h"
#include "utility.h"

int main()
{
    unsigned long _max_command_size = 80;
    std::string _command_string = "";

    utility::print_command_prefix();
    while (true)
    {
        std::getline(std::cin, _command_string);
        std::size_t _first_appearance = _command_string.find_first_of("<");
        bool _is_input_redirection = (_first_appearance != std::string::npos);
        if(!_is_input_redirection && std::cin.eof())
        {
            break;
        }

        if(_command_string.length() > _max_command_size)
        {
            // Add help of myshell
            std::cout << "Invalid command! Please try again." << std::endl;
            utility::print_command_prefix();
            continue;
        }

        int _command_ret = 0;

        if(utility::is_piped_command(_command_string))
        {
            std::vector<std::string> _command_string_vector = utility::split_string(_command_string, '|');
            piped_command* _piped_command = new piped_command();
            for(unsigned int i=0; i < _command_string_vector.size(); i++)
            {
                std::vector<std::string> _command_vector = utility::parse(_command_string_vector.at(i));
                command* _command = new command(_command_vector);
                _piped_command->append_command_to_list(_command);
            }
            command_executor* _command_executor = new command_executor();
            _command_ret = _command_executor->execute(_piped_command);
        }
        else
        {
            int _file_redirection_init_ret = utility::init_handle_file_redirection(_command_string);
            if(_file_redirection_init_ret == -2)
            {
                dup2(utility::std_in_file, STDIN_FILENO);
                std::cout << "Cannot access file" << std::endl;
                utility::print_command_prefix();
                continue;
            }
            if(_file_redirection_init_ret == -3)
            {
                dup2(utility::std_out_file, STDOUT_FILENO);
                std::cout << "Cannot create file" << std::endl;
                utility::print_command_prefix();
                continue;
            }
            std::vector<std::string> _command_vector = utility::parse(_command_string);
            // int _file_redirection_init_ret = utility::init_handle_file_redirection(_command_vector);
            command* _command = new command(_command_vector);
            _command->set_input_from_file(_is_input_redirection);
            command_executor* _command_executor = new command_executor();
            _command_ret = _command_executor->execute(_command);
            if(_file_redirection_init_ret == 0) utility::close_handle_file_redirection();
        }

        if(_command_ret < 0)
        {
             std::cout << "Exiting My Shell...";
             break;
        }
        else if(_command_ret == 2)
        {
            utility::print_supported_commands();
        }
        utility::print_command_prefix();
    }
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
