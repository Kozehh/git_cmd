#pragma once

#include <boost/filesystem.hpp>
using std::string;
using boost::filesystem::path;


// ** COMMANDES GITUS ** //
int Init();
int Add(string pathGiven);
int Commit(const string msg, const string author);

// ** METHODES POUR COMMANDES ** //
void AddToIndex(string addedFileSha, path filePath);
path GetGitFolderPath();
string GetLatestCommit();
void WriteLatestCommit(string latestCommit);
string CreateSha(const string content);
void CreateObject(string sha, std::vector<string> infoToWrite);
string CreateTree(bool isFirstTree);
string GetFile(string sha);
string GetTreeRootSha();
string GetFileContent(string path);
string NewTree(path parentPath, std::vector<path> excludedPaths);
string IterateAndModifyTree(path parentPath, string parentSha, string addedFilePath, string addedFileSha);
string ModifyFileContent(string oldSha, string newSha, string filePath);
path CreateObjectPath(string sha);
void ClearFile(string filePath);
bool AlreadyInIndex(string indexFilePath, string addedFilePath);
bool IsIndexEmpty();

// ** HELP/ERROR METHODS ** //
void WrongNumberArgsError(string command);
void UnknownCommandError(string commandArg, string command);
string PrintHelp(const string command);
void SimpleErrorOccurMessage();
void PathDoesntExistError(string path);

// ** CONSTANTES ** //
const string HELP_COMMAND = "--help";
const string ADD_COMMAND = "add";
const string INIT_COMMAND = "init";
const string COMMIT_COMMAND = "commit";
const string HEAD_FILE = "HEAD";
const string INDEX_FILE = "index";
const string OBJECTS_FOLDER = "objects";
const string GITINORE_FILE = ".gitignore";
const string BUILD_FOLDER = "build";
const string VSCODE_FOLDER = ".vscode";
const string GITUS_FOLDER = "gitus";
const string GIT_FOLDER = ".git";
