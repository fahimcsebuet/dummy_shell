#ifndef COMMAND_EXECUTOR_H_
#define COMMAND_EXECUTOR_H_

#include "command.h"

class command_executor
{
public:
    command_executor();
    ~command_executor();

    int execute(command* p_command);
    int execute(piped_command* p_piped_command);
private:
    command* p_command;
};

#endif

