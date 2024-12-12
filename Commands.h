#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;
class Command {
protected:
    const char *cmd_line;
public:
    Command(const char *cmd_line);

    virtual ~Command()=default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand()=default;

};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() =default;

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
private:
    const char **prompt;
    const char *newPrompt;
public:
    ChangePromptCommand(const char *cmd_line, const char **prompt);

    virtual ~ChangePromptCommand() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    const char *cmd_line;
    char **plastPwd;
    char * pwd;

public:
    ChangeDirCommand(const char *cmd_line,  char **plastPwd);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members 
public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {
    }

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    };

    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class ListDirCommand : public Command {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members
public:
    NetInfo(const char *cmd_line);

    virtual ~NetInfo() {
    }

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
    map <string,string> *aliases;
public:
    aliasCommand(const char *cmd_line, map <string,string>  *alias);

    virtual ~aliasCommand() {
    }

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
    map <string,string> *aliases;

public:
    unaliasCommand(const char *cmd_line, map <string,string>  *alias );

    virtual ~unaliasCommand() {
    }

    void execute() override;
};


class SmallShell {
private:
    const int MAX_ARGS=100;
    char *plastPwd;
    JobsList * jobs;
    const char *prompt;
    map <string,string>  aliases;
    SmallShell();



public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    
    void printPrompt();
    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_