#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <regex>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const std::string ALIAS = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890_";
const std::vector<std::string> reserved_keywords = {
    "chprompt","showpid","pwd","cd","jobs","fg","quit","kill","alias","unalias"
    ,"listdir","whoami","netinfo",
    "sleep","cat"

};
#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}


void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

 bool isNumber(const char *str) {
    // Check if the string is empty or contains non-digit characters
    for (int i = 0; str[i] != '\0'; ++i) {
      if (!isdigit(str[i])) {
        return false;
      }
    }
    return true;
    }

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell():plastPwd(nullptr),prompt("smash") {

}
void SmallShell::printPrompt(){
  if(prompt)
    std::cout <<prompt<<"> ";

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
  
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  for(const auto& pair : aliases){
    if(firstWord.compare(pair.first)==0){
      string result=pair.second+cmd_s.substr(firstWord.length());
      char* new_cmd = new char[result.length() + 1];
      strcpy(new_cmd, result.c_str());
      return CreateCommand(new_cmd);
      }
  } 

  if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line,&prompt);
  }
  
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }

  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line,&plastPwd);
  }

  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line,jobs);
  }

  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line,jobs);
  }

  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line,jobs);
  }

  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line,jobs);
  }

  else if (firstWord.compare("alias") == 0) {
    return new aliasCommand(cmd_line, &aliases);
  }

  else if (firstWord.compare("unalias") == 0) {
    return new unaliasCommand(cmd_line, &aliases);
  }



  else {
    return new ExternalCommand(cmd_line);
  }
  
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  int original_stdout = dup(STDOUT_FILENO);
  if (original_stdout == -1) {
      perror("Failed to duplicate stdout");
      return ;
  }
  
  //todo - search for < or <<, set the cout, and remove the last part from cmd_line
  const std::regex pattern(R"(\s*(>>|>)\s*(\S+)\s*&?$)");
  std::cmatch match;
  std::string str_cmd(cmd_line);
  
  if (std::regex_search(cmd_line, match, pattern)) {
    if (match[1]==">"){
      int file = open(match[2].str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (file == -1) {
        perror("Failed to open file");
        return ;
      } 
      if (dup2(file, STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        close(file);
        return ;
      }  
      close(file);
 
    }
    if (match[1]==">>"){
      int file = open(match[2].str().c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (file == -1) {
        perror("Failed to open file");
        return ;
      }
      if (dup2(file, STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        close(file);
        return ;
      }
      close(file);
  
    }
  }

  str_cmd.erase(match.position(0), match.length(0));
  str_cmd=_trim(str_cmd);
  cmd_line=str_cmd.c_str();
  Command* cmd= CreateCommand(cmd_line);
  cmd->execute();
    // TODO: Add your implementation here
    // for exampl(e:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
  if (dup2(original_stdout, STDOUT_FILENO) == -1) {
      perror("Failed to restore stdout");
      close(original_stdout);
      return ;
  }
  close(original_stdout);


}

Command::Command(const char *cmd_line){
  Command::cmd_line=cmd_line;
}

string Command::printCommand(){
  return cmd_line;
}

/*
bool Command::isAlias(){

}
*/

BuiltInCommand::BuiltInCommand(const char *cmd_line):Command(cmd_line){}

const char * getPwd(){
  const size_t size=1024;
  char buffer[size];
  return getcwd(buffer,size);
}

ChangePromptCommand::ChangePromptCommand(const char *cmd_line, const char **prompt) : BuiltInCommand(cmd_line) , prompt(prompt), newPrompt(nullptr){
  char **args=new char *[COMMAND_MAX_ARGS];
  _parseCommandLine(cmd_line,args);

  if (args[1]) {
    newPrompt = new char[strlen(args[1]) + 1];
    strcpy(const_cast<char*>(newPrompt), args[1]);
    }
  else{
    newPrompt = new char[strlen("smash") + 1];
    strcpy(const_cast<char*>(newPrompt), "smash");
  }

  }

void ChangePromptCommand::execute(){
  if (prompt && newPrompt) {
    *prompt = newPrompt; // Redirect external prompt to newPrompt
  }

  }

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute(){
  std::cout << getPwd()<<endl;
    return;

  
}



ShowPidCommand::ShowPidCommand(char const *cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute(){
  int pid=getpid();
  cout <<"smash pid is " <<pid<<endl;

}



ChangeDirCommand::ChangeDirCommand(char const *cmd_line,  char **plastPwd) : BuiltInCommand(cmd_line),cmd_line(cmd_line), plastPwd(plastPwd){}

void ChangeDirCommand::execute(){
  char **args=new char *[COMMAND_MAX_ARGS];
  _parseCommandLine(cmd_line,args);
  if(args[2]){
    std::cerr << "smash error: cd: too many arguments\n";
  }
  if(!args[1])
    return;
  const char* currentPwd=getPwd();
  pwd = new char[strlen(currentPwd) + 1];
  strcpy(pwd,currentPwd);
  if(strcmp(args[1],"-") == 0){

    if(*plastPwd){
      chdir(*plastPwd);
      *ChangeDirCommand::plastPwd=pwd;
      return;
    }
    else{
      std::cerr << "smash error: cd: OLDPWD not set\n";
      return;
    }

  }
  else if (chdir(args[1])!=-1)
    *ChangeDirCommand::plastPwd=pwd;
}



// JobsList

void JobsList::addJob(Command *cmd, int pid, bool isStopped){
  removeFinishedJobs();

  int jobId=1;

  if(!jobsList.empty()){
    jobId = (jobsList.back() -> jobId) + 1;
  }
  string commandString = cmd -> printCommand();

  if(cmd -> isAlias()){
    string commandString = cmd -> getAlias();
  }
  
  jobsList.push_back(new JobEntry(cmd, jobId, pid, commandString, isStopped));

}

void JobsList::printJobsList(){
  removeFinishedJobs();

  for (JobsList::JobEntry* jobEntry : jobsList ){
    cout << "[" << jobEntry -> jobId << "]" << jobEntry -> commandString << endl;
  }
}



JobsList::~JobsList()
{
  for(auto it=jobsList.begin(); it!=jobsList.end();++it)
  {
    delete *it;
  }
}



JobsCommand::JobsCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){}

void JobsCommand::execute(){
  jobs -> printJobsList();
}


QuitCommand::QuitCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){}

void QuitCommand::execute(){}


KillCommand::KillCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){}

void KillCommand::execute(){}



ForegroundCommand::ForegroundCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){}

void ForegroundCommand::execute(){
  int jobId = 0;
  char *args[2];
  int argc = _parseCommandLine(cmd_line, args);

  if(argc > 2){
    cerr << "smash error: fg: invalid arguments" << endl;
    return;
  }

  if (argc == 1){  // Bring to fg the last job in the list
    if(jobs -> jobsList.empty()){
      cerr << "smash error: fg: jobs list is empty" << endl;
      return;
    }
    else{
      jobId = jobs->jobsList.back() -> jobId;   // Last job id
    }
  }
  else{   // There is a second argument
      if (!isNumber(args[1])) {
      cout << "smash error: fg: invalid arguments" << endl;
      return;
    }
    else{
      
    }
  }

}


aliasCommand::aliasCommand(char const *cmd_line, map <string,string> *aliases) : BuiltInCommand(cmd_line), aliases(aliases){}


void aliasCommand::execute(){
  if(strcmp(cmd_line,"alias")==0){
    for(const auto& pair : *aliases){
      cout << pair.first<<"='"<<pair.second<<"'\n";
    }
    return;
  } 
  
  std::regex pattern(R"(^alias [a-zA-Z0-9_]+='[^']*'$)");
  if (!std::regex_match(string(cmd_line), pattern)) {
    std::cerr << "smash error: alias: invalid alias format\n";
    return;
  } 
  size_t equalSign = (string(cmd_line)).find_first_of("=");
  size_t firstSpace = (string(cmd_line)).find_first_of(WHITESPACE);
  string aliasName= _ltrim((string(cmd_line)).substr(firstSpace,equalSign-firstSpace));
  string aliasMean= _rtrim((string(cmd_line)).substr(equalSign+1));
  for(const auto& pair : *aliases){
    if(aliasName.compare(pair.first)==0){
      std::cerr << "smash error: alias: "<<aliasName<< " already exists or is a reserved command \n";
      return;    }
  } 
  for(const auto& command :reserved_keywords){
    if(aliasName.compare(command)==0){
      std::cerr << "smash error: alias: "<<aliasName<< " already exists or is a reserved command \n";
      return;    }
  }  
  aliasMean=aliasMean.substr(1,aliasMean.length()-2);
  aliases->insert({aliasName,aliasMean});

  
  

}


unaliasCommand::unaliasCommand(char const *cmd_line, map <string,string> *aliases ) : BuiltInCommand(cmd_line), aliases(aliases){}

void unaliasCommand::execute(){
  char **args=new char *[COMMAND_MAX_ARGS];
  _parseCommandLine(cmd_line,args);
  if(!args[1]){
    cerr <<"smash error: unalias: not enough arguments\n";
    return;
  }
  for(int i=1; i<COMMAND_MAX_ARGS&&args[i];i++){
    auto it = aliases->find(args[i]);
    if (it != aliases->end()) {
      aliases->erase(it);
      }
      else {
        std::cerr << "smash error: unalias: " << args[i] << " alias does not exist" << std::endl;
      }    
  }
}





void executeWithBash(char const *cmd_line){
  char **args=new char *[4];
  args[0]=strdup("bash");
  args[1]=strdup("-c");
  char* arguments=new char[COMMAND_MAX_LENGTH];
  strcpy(arguments,cmd_line);
  args[2]=arguments;
  args[3]=nullptr;
  execv("/bin/bash",args);
  exit(1);
}
void executeNoBash(char const *cmd_line){
  char **args=new char *[COMMAND_MAX_ARGS];
  _parseCommandLine(cmd_line,args);
  string path=string("/bin/")+args[0];
  if (args[0][0] == '/' || args[0][0] == '.') {
    execv(args[0], args);  
  } else {
    execvp(args[0], args);
  }

  exit(1);
}


ExternalCommand::ExternalCommand(char const *cmd_line) : Command(cmd_line){}
void ExternalCommand::execute(){

  string str_cmd(cmd_line);
  if(str_cmd.find('?') != std::string::npos || str_cmd.find('*') != std::string::npos){
    if (_isBackgroundComamnd(cmd_line)){

    }
    else{
      int pid=fork();

      if(pid==-1){
        perror("smash error: fork failed");
        return;
      }
      if(pid==0){
        setpgrp();
        executeWithBash(cmd_line);
      }
      else{
        waitpid(pid, nullptr, 0);

      }
    }
  }
  else{
    if (_isBackgroundComamnd(cmd_line)){

    }
    else{
      int pid=fork();
      if(pid==-1){

        perror("smash error: fork failed");
        return;
      }
      if(pid==0){
        setpgrp();
        executeNoBash(cmd_line);
      }
      else{
        waitpid(pid, nullptr, 0);

      }
    }
  }
}


