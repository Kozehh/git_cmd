#include <iostream>

#include <gituscommands.h>

int main(int argc, char *argv[])
{
    // If there is a command following the call of the app (./gitus)
    if (argc >= 2)
    {
        string command(argv[1]);
        if (command.compare(INIT_COMMAND) == 0)
        {
            if (argc == 2)
            {
                Init();
            }
            else if (argc == 3)
            {
                string thirdArg(argv[2]);
                if (thirdArg.compare(HELP_COMMAND) == 0)
                {
                    PrintHelp(INIT_COMMAND);
                }
                else
                {
                    UnknownCommandError(thirdArg, INIT_COMMAND);
                }
            }
        }
        else if (command.compare(ADD_COMMAND) == 0)
        {
            if (argc == 3)
            {
                string thirdCommand(argv[2]);
                if (thirdCommand.compare(HELP_COMMAND) == 0)
                {
                    PrintHelp(ADD_COMMAND);
                }
                else
                {
                    Add(thirdCommand);
                }
            }
            else
            {
                WrongNumberArgsError(ADD_COMMAND);
                PrintHelp(ADD_COMMAND);
            }
        }
        else if (command.compare(COMMIT_COMMAND) == 0)
        {
            if (argc == 3 || argc == 4)
            {
                string thirdArg(argv[2]);
                if (argc == 3)
                {
                    if (thirdArg.compare(HELP_COMMAND) == 0)
                    {
                        PrintHelp(COMMIT_COMMAND);
                    }
                    else
                    {
                        UnknownCommandError(thirdArg, COMMIT_COMMAND);
                    }
                }
                else
                {
                    Commit(thirdArg, string(argv[3]));
                }
            }
            else
            {
                WrongNumberArgsError(COMMIT_COMMAND);
                PrintHelp(COMMIT_COMMAND);
            }
        }
        else if (argc == 2 && command.compare(HELP_COMMAND) == 0)
        {
            PrintHelp(HELP_COMMAND);
        }
        else
        {
            std::cout << "Error: Unknown command : " << command << std::endl;
            PrintHelp(HELP_COMMAND);
        }
    }
    else
    {
        WrongNumberArgsError(string(argv[0]));
        PrintHelp(HELP_COMMAND);
    }
    return 0;
}

void WrongNumberArgsError(string command)
{
    std::cout << "Error: Wrong number of arguments for the command '" << command << "'\n";
}

void UnknownCommandError(string commandArg, string command)
{
    std::cout << "Unknown command '" << commandArg << "'... \nTry '" << command << " " << HELP_COMMAND << "'\n";
}
