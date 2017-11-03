#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <limits.h>

std::string get_user_name(unsigned int user_id)
{
    struct passwd *password;
    if((password = getpwuid(user_id)) != NULL)
    {
        return std::string(password->pw_name);
    }
    else
    {
        char uid_char_array[80];
        sprintf(uid_char_array, "%d", user_id);
        return std::string(uid_char_array);
    }
}

std::string get_group_name(unsigned int group_id)
{
    struct group *p_group;
    if((p_group = getgrgid(group_id)) != NULL)
    {
        return std::string(p_group->gr_name);
    }
    else
    {
        char gid_char_array[80];
        sprintf(gid_char_array, "%d", group_id);
        return std::string(gid_char_array);
    }
}

void print_file_status(const struct stat status, std::string file_to_ls)
{
    char _date_time[20];
    // Format Date and Time
    strftime(_date_time, 20, "%b %d %H:%M", localtime(&status.st_ctime));
    //Format permissions
    std::string _permissions = "";
    std::string _real_path_to_link = "";
    int _number_of_entries = 1; // by deafault 1 for anything; file, link or directory
    if(S_ISLNK(status.st_mode))
    {
        _permissions += "l";
        char _real_path_buffer[PATH_MAX + 1];
        char *_ret_real_path = realpath(file_to_ls.c_str(), _real_path_buffer);
        _real_path_to_link = std::string(_ret_real_path);
    }
    else if(S_ISREG(status.st_mode))
    {
        _permissions += "-";
    }
    else if(S_ISDIR(status.st_mode))
    {
        _permissions += "d";

        struct dirent *_directory_entry;
        DIR *p_dir = opendir(file_to_ls.c_str());
        if(p_dir)
        {
            while((_directory_entry = readdir(p_dir)) != NULL)
            {
                std::string _filename(_directory_entry->d_name);
                if(!_filename.empty() && _filename.at(0) == '.')
                {
                    continue;
                }
                #ifdef _DIRENT_HAVE_D_TYPE
                if(_directory_entry->d_type == DT_REG)
                #endif
                _number_of_entries += 1;
            }
        }
    }

    if(status.st_mode & S_IRUSR)
    {
        _permissions += "r";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IWUSR)
    {
        _permissions += "w";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IXUSR)
    {
        _permissions += "x";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IRGRP)
    {
        _permissions += "r";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IWGRP)
    {
        _permissions += "w";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IXGRP)
    {
        _permissions += "x";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IROTH)
    {
        _permissions += "r";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IWOTH)
    {
        _permissions += "w";
    }
    else
    {
        _permissions += "-";
    }
    if(status.st_mode & S_IXOTH)
    {
        _permissions += "x";
    }
    else
    {
        _permissions += "-";
    }

    std::string _real_path_to_link_output = "";
    if(!_real_path_to_link.empty())
    {
        _real_path_to_link_output += (" -> " + _real_path_to_link);
    }
    std::cout << _permissions << ". " << _number_of_entries << " " << get_user_name(status.st_uid) << " " << get_group_name(status.st_gid) << " " 
    << status.st_size << " "  << std::string(_date_time) << " " <<  file_to_ls << _real_path_to_link_output << std::endl;
}

int main(int argc, char *argv[])
{
    std::string _file_to_ls = ".";
    if(argc > 2)
    {
        std::cout << "Usage of myls command" << std::endl;
        return EXIT_SUCCESS;
    }
    else if(argc == 2)
    {
        char *_filename = argv[1];
        if(_filename[0] != '/')
        {
            _file_to_ls = _file_to_ls + std::string("/") + std::string(_filename);
        }
        else if(_filename[0] == '/')
        {
            _file_to_ls = std::string(_filename);
        }
    }

    int _total_blocks = 0;

    struct dirent *_directory_entry;
    DIR *p_dir = opendir(_file_to_ls.c_str());

    if(p_dir == NULL)
    {
        struct stat _file_stat;

        if(lstat(_file_to_ls.c_str(), &_file_stat) < 0)
        {
            std::cout<< _file_to_ls << std::endl;
            std::cout << "Could not open the specified directory" << std::endl;
            return EXIT_FAILURE;
        }
        // Print information about file and return success
        print_file_status(_file_stat, _file_to_ls);
        return EXIT_SUCCESS;
    }

    while((_directory_entry = readdir(p_dir)) != NULL)
    {
        std::string _filename(_directory_entry->d_name);

        if(!_filename.empty() && _filename.at(0) == '.')
        {
            continue;
        }

        struct stat _file_stat;

        std::string _file_absolute_path = _file_to_ls + "/" + _filename;
        if(lstat(_file_absolute_path.c_str(), &_file_stat) < 0)
        {
            std::cout << "stat of " << _file_to_ls << "failed!" << std::endl;
            return EXIT_FAILURE;
        }

        _total_blocks += _file_stat.st_blocks;
        print_file_status(_file_stat, _filename);
    }

    _total_blocks /= 2; // to find the number of 1M blocks specified
    std::cout << "total " << _total_blocks << std::endl;
    if(p_dir) closedir(p_dir);

    return EXIT_SUCCESS;
}
