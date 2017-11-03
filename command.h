#ifndef COMMAND_H_
#define COMMAND_H_

#include <string>
#include <vector>

class command
{
public:
    command(std::vector<std::string>& command_vector);
    ~command();

    std::string get_name() { return name; }
    inline std::vector<std::string> get_arguments() { return arguments; }
    inline bool is_input_from_file() { return input_from_file; }
    inline void set_input_from_file(bool input_from_file) { this->input_from_file = input_from_file; }

private:
    std::string name;
    std::vector<std::string> arguments;
    bool input_from_file;
};

class piped_command
{
public:
    piped_command();
    ~piped_command();

    inline std::vector<command*> get_commands_list() { return commands_list; }
    void append_command_to_list(command* _command);

private:
    std::vector<command*> commands_list;
};

#endif
