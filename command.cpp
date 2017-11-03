#include "command.h"

command::command(std::vector<std::string>& command_vector)
{
    for (unsigned int _i = 0; _i < command_vector.size(); _i++)
    {
        if(_i == 0)
        {
            name = command_vector.at(_i);
        }
        else
        {
            arguments.push_back(command_vector.at(_i));
        }
    }
}

command::~command()
{
}

piped_command::piped_command()
{
}

piped_command::~piped_command()
{
}

void piped_command::append_command_to_list(command* _command)
{
    commands_list.push_back(_command);
}
