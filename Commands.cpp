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

#include <sys/syscall.h>  // Used for listDir command: SYS_getdents syscall
#include <dirent.h>       // Used for listDir command: For directory entries

#include <net/if.h>       // Used for netInfo command:(ifreq)
#include <arpa/inet.h>    // Used for netInfo command: inet_ntoa function
#include <sys/ioctl.h>    // Used for netInfo command: ioctl function


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

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    
    char *cmd_line_copy = strdup(cmd_line); 
    _removeBackgroundSign(cmd_line_copy); 
    std::istringstream iss(_trim(string(cmd_line)).c_str());

    int i = 0;
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundCommand(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
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

JobsList* SmallShell::getJobsList(){
  return jobs;
}

char* SmallShell::getPwd(){
  return plastPwd;
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
  else if (firstWord.compare("listdir") == 0) {
    return new ListDirCommand(cmd_line);
  }

  else {
    return new ExternalCommand(cmd_line, jobs);
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
  std::cout <<"smash pid is " <<pid<<endl;

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

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped){
  removeFinishedJobs();

  int jobId=1;

  if(!jobsList.empty()){
    jobId = (jobsList.back() -> jobId) + 1;
  }
  string commandString = cmd -> printCommand();

  jobsList.push_back(new JobEntry(cmd, jobId, pid, commandString, isStopped));

}

void JobsList::printJobsList(){
  removeFinishedJobs();

  for (JobsList::JobEntry* jobEntry : jobsList ){
    std::cout << "[" << jobEntry -> jobId << "]" << jobEntry -> commandString << std::endl;
  }
}

JobsList::JobEntry *JobsList::getJobById(int jobId){
  for(auto listIt=jobsList.begin(); listIt!=jobsList.end();++listIt)
  {
    if((*listIt) -> jobId == jobId)
    {
      return *listIt;
    }
  }
  return nullptr;
}

void JobsList::removeJobById(int jobId){
  for(auto listIt=jobsList.begin(); listIt!=jobsList.end();++listIt)
  {
    if((*listIt)->jobId==jobId)
    {
      delete *listIt;
      jobsList.erase(listIt);
      return;
    }
  }
}


bool isFinished(JobsList::JobEntry* job)
{
  if(job==nullptr)
  {
    return true;
  }
  int status;
  return waitpid(job->pid, &status, WNOHANG) != 0;
}


void JobsList::removeFinishedJobs(){
  jobsList.remove_if(isFinished);
}

JobsList::JobEntry  *JobsList::getLastJob(){
  if(jobsList.empty())
  {
    return nullptr;
  }
  return jobsList.back();
}

void JobsList::printJobsForQuitFunc(){
  removeFinishedJobs();
  std::cout << "smash: sending SIGKILL signal to "<<jobsList.size()<<" jobs:" << std::endl;

  for(auto listIt=jobsList.begin(); listIt!=jobsList.end();++listIt)
  {
    JobsList::JobEntry* job=*listIt;
    std::cout << job -> pid <<": " << job -> commandString  << std::endl;
  }
}

void JobsList::killAllJobs(){
  for( auto listIt = jobsList.begin() ; listIt != jobsList.end() ; ++listIt){
    kill((*listIt) -> pid, SIGKILL);

    JobEntry* job = *listIt;
    *listIt = nullptr;
    delete job;
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


QuitCommand::QuitCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){}

void QuitCommand::execute(){
  char **args=new char *[COMMAND_MAX_ARGS];
  _parseCommandLine(cmd_line,args);
  _removeBackgroundSign(args[1]);

  if (strcmp(args[1], "kill") == 0) {
    jobs->removeFinishedJobs();
    jobs->printJobsForQuitFunc();
    jobs->killAllJobs();
  }

}


KillCommand::KillCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),jobs(jobs){}

void KillCommand::execute(){

  char **args=new char *[COMMAND_MAX_ARGS];
  int argc = _parseCommandLine(cmd_line, args);

  if( argc != 3 ){
    delete[] args;
    std::cerr <<"smash error: kill: invalid arguments" << std::endl;
    return;
  }
  else{
    if ( args[1][0] != '-'){
      delete[] args;
      std::cerr <<"smash error: kill: invalid arguments" << std::endl;
      return;
    }
    else{ 
      std::string signalStr(args[1]);   // Convert args to string
      try {
        int signalNum = std::stoi(signalStr.substr(1));  // Remove the '-' and convert the rest
        if (signalNum < 1 || signalNum > 31){
          delete[] args;
          perror("smash error: kill failed");
        }
        _removeBackgroundSign(args[2]);
        int jobId = std::stoi(args[2]);

        JobsList::JobEntry* job = jobs -> getJobById(jobId);
        if (job == nullptr){
          std::cerr << "smash error: kill: job-id "<<jobId<<" does not exist" << std::endl;
          delete[] args;
          return;
        }
        else{
          std::cout<< "signal number "<<signalNum<< " was sent to pid "<<job -> pid <<std::endl;
          if(kill(job -> pid,signalNum)!=0)
          {
            delete[] args;
            perror("smash error: kill failed");
          }
        }
      }
      catch (const std::invalid_argument& e) {
       std::cerr << "smash error: kill: invalid arguments" << std::endl;
       delete[] args;
       return;
      }
    }  
  }
}



ForegroundCommand::ForegroundCommand(char const *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){}

void ForegroundCommand::execute(){
  int jobId = 0;
  
  char **args=new char *[COMMAND_MAX_ARGS];
  int argc = _parseCommandLine(cmd_line, args);

  if(argc > 2){
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    delete[] args;
    return;
  }

  if (argc == 1){  // Bring to fg the last job in the list
    if(jobs -> jobsList.empty()){
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      delete[] args;
      return;
    }

    jobId = jobs-> getLastJob() -> jobId;   // Last job id

  }
  else{   // There is a second argument
      try {
        // Convert the second argument (args[1]) to an integer
        jobId = std::stoi(args[1]);
        if (jobId <= 0) {
          std::cerr << "smash error: fg: invalid arguments" << std::endl;
          delete[] args;
          return;
        }
    } catch (const std::exception &e) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        delete[] args;
        return;
    }
  }

  JobsList::JobEntry* job = jobs -> getJobById(jobId);

  if (job == nullptr){
    std::cerr << "smash error: fg: job-id " << jobId << "does not exist" << std::endl;
    delete[] args;
    return;
  }
  int pid = job -> pid;
  std::cout << job -> commandString << " " << pid << std::endl;

  jobs -> removeJobById(jobId);
}


aliasCommand::aliasCommand(char const *cmd_line, map <string,string> *aliases) : BuiltInCommand(cmd_line), aliases(aliases){}


void aliasCommand::execute(){
  if(strcmp(cmd_line,"alias")==0){
    for(const auto& pair : *aliases){
      std::cout << pair.first<<"='"<<pair.second<<"'\n";
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
  if(execv("/bin/bash",args)==-1){
      perror("smash error: exec failed");
      exit(1);  // Exit if exec fails

  }
  exit(1);
}
void executeNoBash(char const *cmd_line){
  char **args=new char *[COMMAND_MAX_ARGS];
  _parseCommandLine(cmd_line,args);
  string path=string("/bin/")+args[0];

  if (args[0][0] == '/' || args[0][0] == '.') {
    if (execv(args[0], args) == -1) {
      perror("smash error: exec failed");
      exit(-1);  // Exit if exec fails
    }
  }

  else{
              cout <<"here\n";

    if (execvp(args[0], args) == -1) {
      perror("smash error: exec failed");
      exit(-1);  // Exit if exec fails
    }
  }
          cout <<"here2\n";

  exit(1);
}


ExternalCommand::ExternalCommand(char const *cmd_line, JobsList *jobs) : Command(cmd_line), jobs(jobs){}
void ExternalCommand::execute(){

  string str_cmd(cmd_line);

  //Complex external command: 
  if(str_cmd.find('?') != std::string::npos || str_cmd.find('*') != std::string::npos){
    //Complex external command in Background
    if (_isBackgroundCommand(cmd_line)){
      size_t length = strlen(cmd_line) + 1; // +1 for the null terminator
      char* buffer = new char[length];
      strcpy(buffer, cmd_line);
      _removeBackgroundSign(buffer);

      pid_t pid=fork();

      if(pid==-1){
        perror("smash error: fork failed");
        return;
      }
      if(pid==0){
        setpgrp();
        executeWithBash(buffer);
        delete(buffer);

      }
      else{ //Parent not wating

        JobsList* jobs = SmallShell::getInstance().getJobsList();
        jobs->addJob(this,pid);
      } 
    }
    //Complex external command in Foreground
    else{
      pid_t pid=fork();

      if(pid==-1){
        perror("smash error: fork failed");
        return;
      }
      if(pid==0){
        setpgrp();
        executeWithBash(cmd_line);
      }
      else{ //Parent
        pid_t pidStatus = waitpid(pid, NULL, 0);
        if (pidStatus == -1)
        {
          perror("smash error: waitpid failed");
          return;
        }
      }
    }
  }

  //Simple external command:
  else{   
    //Simple external command in Background
    if (_isBackgroundCommand(cmd_line)){
      size_t length = strlen(cmd_line) + 1; // +1 for the null terminator
      char* buffer = new char[length];
      strcpy(buffer, cmd_line);
      _removeBackgroundSign(buffer);
      pid_t pid=fork();

      if(pid==-1){
        perror("smash error: fork failed");
        return;
      }
      if(pid==0){
        setpgrp();
        executeNoBash(buffer);
        delete(buffer);

      }
      else{ //Parent not wating
        JobsList* jobs = SmallShell::getInstance().getJobsList();
        jobs->addJob(this,pid);

      }

    }
    //Simple external command in Foreground
    else{
      pid_t pid=fork();
      if(pid==-1){

        perror("smash error: fork failed");
        return;
      }
      if(pid==0){
        setpgrp();
        executeNoBash(cmd_line);
      }
      else{   //Parent
        pid_t pidStatus = waitpid(pid, NULL, 0);
                  cout <<"here3\n";

        if (pidStatus == -1)
        {
          perror("smash error: waitpid failed");
          return;
        }
      }
    }
  }
}

ListDirCommand::ListDirCommand(const char *cmd_line):Command(cmd_line){};

//Helper function to print the directory tree
void ListDirCommand::printTree(const std::string& path, std::vector<std::string>& directories, std::vector<std::string>& files, int level) {
  // Print directories and go Recursively to subdirectories
  for (const auto& dir : directories) {
    std::cout << std::string(level, '\t') << dir << std::endl;
        
    // We will skip '.' and '..'
    if (dir != "." && dir != "..") {
      // Recursively call printTree for subdirectories
      std::string subDirPath = path + "/" + dir;
      std::vector<std::string> subDirectories;
      std::vector<std::string> subFiles;

      // Open the subdirectory to get its contents
      int subFd = open(subDirPath.c_str(), O_RDONLY | O_DIRECTORY);
      if (subFd == -1) {
        perror("smash error: open failed");
        return;
      }

      char bufferRead[BUFFER_SIZE];
      ssize_t sizeToRead;

      while (true) {
        sizeToRead = syscall(SYS_getdents, subFd, bufferRead, BUFFER_SIZE);
        if (sizeToRead == -1) {
          perror("smash error: getdents failed");
            break;
        } 
        else if (sizeToRead == 0) {
          break;
        }

        for (int i = 0; i < sizeToRead;) {
          struct dirent *entry = (struct dirent *)(bufferRead + i);
          std::string name = entry->d_name;

          if (entry->d_type == DT_DIR) {
            subDirectories.push_back(name);  // Add subdirectories
          } 
          else if (entry->d_type == DT_REG) {
            subFiles.push_back(name);  // Add files
          }

          i += entry->d_reclen;  // Move to the next entry
        }
      }

      close(subFd);
      // Sort subdirectories and files alphabetically
      std::sort(subDirectories.begin(), subDirectories.end());
      std::sort(subFiles.begin(), subFiles.end());

      // Print the contents of the subdirectory recursively
      printTree(subDirPath, subDirectories, subFiles, level + 1);
    
    }
  }
  // print files
  for (const auto& file : files) {
    std::cout << std::string(level, '\t') << file << std::endl;
  }
}

void ListDirCommand::execute(){

  std::string pathToDir="";
  std::vector<std::string> directories;
  std::vector<std::string> files;


  char **args=new char *[COMMAND_MAX_ARGS];
  int argc = _parseCommandLine(cmd_line,args);

  if (argc > 2){
    std::cerr << "smash error: listdir: too many arguments" << std::endl;
    return;
  }
  else if(argc == 1){
    pathToDir = SmallShell::getInstance().getPwd();
  }
  else{
    _removeBackgroundSign(args[1]);
    pathToDir = args[1];
  }

  int fd = open(pathToDir.c_str(), O_RDONLY | O_DIRECTORY);
  if (fd == -1) 
  {
    perror("smash error: open failed");
    return;
  }
  
  char bufferRead[BUFFER_SIZE];
  ssize_t sizeToRead;

  while (true) {
    // SYS_getdents - syscall used to read directory entries from a file descriptor.
    sizeToRead = syscall(SYS_getdents, fd, bufferRead, sizeof(bufferRead));
    if (sizeToRead == -1){
      perror("smash error: getdents failed");
      break;
    }
    else if(sizeToRead == 0){
      break;
    }
    
    for (int i = 0 ; i < sizeToRead ;){
      struct dirent *entry = (struct dirent *)(bufferRead + i);
      std::string name = entry->d_name;

      //We will separate files and directories

      if (entry->d_type == DT_DIR) {       //Its directory
        directories.push_back(name);  
      } 
      else if (entry->d_type == DT_REG) {  //Its file
        files.push_back(name);
      }

      i += entry->d_reclen;  // Move to the next entry
    }
  }
  close(fd);

  std::sort(directories.begin(), directories.end());
  std::sort(files.begin(), files.end());

  printTree(pathToDir, directories, files);
}


WhoAmICommand::WhoAmICommand(const char *cmd_line): Command(cmd_line){};

void WhoAmICommand::execute(){

  uid_t uid = getuid();
  int fd = open("/etc/passwd", O_RDONLY);

  if (fd == -1) {
    perror("smash error: open failed");
    return;
  }

  char buffer[BUFFER_SIZE];  // Buffer size = 4096 
    ssize_t bytesRead;
    std::string passwdContent;
        
    //  Read from "/etc/passwd" to buffer
    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
      passwdContent.append(buffer, bytesRead);
    }
    if (bytesRead == -1) {
      perror("smash error: read failed");
      close(fd);
      return;
    }

    close(fd);

    // Split content into lines
    size_t lineStart = 0;
    while (lineStart < passwdContent.size()) {
    size_t lineEnd = passwdContent.find('\n', lineStart);

    // If cant find '\n'
    if (lineEnd == std::string::npos) {
      lineEnd = passwdContent.size();
    }

    std::string currLine = passwdContent.substr(lineStart, lineEnd - lineStart);
    lineStart = lineEnd + 1;

    // Add the fields in the line to vector
    std::vector<std::string> fields;
    size_t fieldStart = 0;

    while (fieldStart < currLine.size()) {
      size_t fieldEnd = currLine.find(':', fieldStart);

      // If cant find ':'
      if (fieldEnd == std::string::npos) {
        fieldEnd = currLine.size();
      }

      fields.push_back(currLine.substr(fieldStart, fieldEnd - fieldStart));
      fieldStart = fieldEnd + 1;
    }
    /*  Lines is typically split into 7 fields in "/etc/passwd"
        Field 0: fields[0] is the username.
        Field 2: fields[2] is the UID.
        Field 5: fields[5] is the home directory.
    */

    // Check if UID matches
    if (fields.size() >= 7) {
      if (std::stoi(fields[2]) == static_cast<int>(uid)) {
        std::string username = fields[0];
        std::string homeDir = fields[5];
        std::cout << username << " " << homeDir << std::endl;
        return;
      }
    }    
  }
}


NetInfo::NetInfo(const char *cmd_line): Command(cmd_line){};


void getInterfaceDetails(const std::string &interface, std::string &ipAddress, std::string &subnetMask) {
  struct ifreq ifr;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock == -1) {
    perror("smash error: socket failed");
    return;
  }

  strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);

  // Get IP address
  if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
    struct sockaddr_in *ipAddr = (struct sockaddr_in *)&ifr.ifr_addr;
    ipAddress = inet_ntoa(ipAddr->sin_addr);
  } 
  else {
    perror("smash error: ioctl failed");
  }

  // Get subnet mask
  if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0) {
    struct sockaddr_in *subnetAddr = (struct sockaddr_in *)&ifr.ifr_netmask;
    subnetMask = inet_ntoa(subnetAddr->sin_addr);
  } 
  else {
    perror("smash error: ioctl failed");  
  }

  close(sock);
}

void getDefaultGateway(std::string &gateway) {

  int fd = open("/proc/net/route", O_RDONLY);
  if (fd == -1) {
    perror("smash error: open failed");  
    return;
  }

  char buffer[BUFFER_SIZE];
  ssize_t bytesRead = read(fd, buffer, sizeof(buffer));

  if (bytesRead == -1) {
    perror("smash error: read failed");  
    close(fd);
    return;
  }

  close(fd);

  // We want treat buffer as a string
  buffer[bytesRead] = '\0';
    
  std::string content(buffer);
  std::string line;
  std::istringstream ss(content);

  // Skip the first line (headers)
  std::getline(ss, line);

  while (std::getline(ss, line)) {
    std::istringstream lineStream(line);
    std::string iface, destination, gatewayHex;
    unsigned int dest, gw;

    lineStream >> iface >> destination >> gatewayHex;

    if (iface != "Iface" && destination == "00000000") {  // Default
      std::stringstream ssGateway;
      ssGateway << std::hex << gatewayHex;
      ssGateway >> gw;
      struct in_addr addr;
      addr.s_addr = gw;
      gateway = inet_ntoa(addr);
      break;  
    }
  }
}

void getDNSServers(std::vector<std::string>& dnsServers) {
  const char* filePath = "/etc/resolv.conf";

  char buffer[BUFFER_SIZE];  // Buffer to hold data read from the file
  ssize_t bytesRead;

  int fd = open(filePath, O_RDONLY);
  if (fd == -1) {
    perror("smash error: open failed");  
    return;
  }

  // Read the file content
  while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytesRead] = '\0';  // Null-terminate the buffer

    // Process the data to extract DNS server entries
    char* line = strtok(buffer, "\n");

    while (line != nullptr) {
      // Look for lines starting with "nameserver"
      if (strncmp(line, "nameserver", 10) == 0) {
        // Extract the DNS server address after "nameserver "
        std::string dnsServer(line + 11);
        dnsServers.push_back(dnsServer);
      }

      line = strtok(nullptr, "\n");  // Get the next line
    }
  }
  close(fd);

  if (dnsServers.empty()) {
    std::cerr << "No DNS servers found in /etc/resolv.conf." << std::endl;
    return;
  }
}

void NetInfo::execute(){
  char **args=new char *[COMMAND_MAX_ARGS];
  int argc = _parseCommandLine(cmd_line,args);

  if (argc == 1){
    std::cerr<<"smash error: netinfo: interface not specified" << std::endl;
    return;
  }

  std::string interface = args[1];

  struct ifreq ifr;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    perror("smash error: socket failed");
    return;
  }

  //  IFNAMSIZ - defines the maximum length of a network interface name
  strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);

  if (ioctl(sock, SIOCGIFADDR, &ifr) == -1) {
    std::cerr << "smash error: netinfo: interface " << interface << " does not exist" << std::endl;
    close(sock);
    return;
  }

  std::string ipAddress, subnetMask, gateway;
  std::vector<std::string> dnsServers;

  // Get IP address and subnet mask
  getInterfaceDetails(interface, ipAddress, subnetMask);

  // Get default gateway
  getDefaultGateway(gateway);

  // Get DNS servers
  getDNSServers(dnsServers);


  std::cout << "IP Address: " << ipAddress << std::endl;
  std::cout << "Subnet Mask: " << subnetMask << std::endl;
  std::cout << "Default Gateway: " << gateway << std::endl;
  std::cout << "DNS Servers: ";

  for (size_t i = 0; i < dnsServers.size(); ++i) {
    std::cout << dnsServers[i];
    if (i != dnsServers.size() - 1) {
      std::cout << ", ";
    }
  }

  std::cout << std::endl;

  close(sock);
}







