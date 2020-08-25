#include "gituscommands.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/uuid/detail/sha1.hpp>
using std::string;
using boost::filesystem::path;
using boost::uuids::detail::sha1;


													//////////////////////////
													// ** GITUS COMMANDS ** //
													//////////////////////////
// Commande gitus init
int Init()
{
	auto pathFolderGit = GetGitFolderPath();
	auto pathFolderObject = pathFolderGit / OBJECTS_FOLDER;
	auto pathFileIndex = pathFolderGit / INDEX_FILE;
	auto pathFileHEAD = pathFolderGit / HEAD_FILE;
	boost::system::error_code ec;

	if (!boost::filesystem::exists(pathFolderGit))
	{
		boost::filesystem::create_directory(pathFolderGit, ec);
		if (ec)
		{
			std::cout << ec.message() << std::endl;
		}
		if (!boost::filesystem::exists(pathFolderObject))
		{
			boost::filesystem::create_directory(pathFolderObject, ec);
			if (ec)
			{
				std::cout << ec.message() << std::endl;
			}
		}
		if (!boost::filesystem::exists(pathFileIndex))
		{
			std::ofstream indexFile(pathFileIndex.string());
			if(indexFile.is_open())
			{
				indexFile.close();
			}
		}
		if (!boost::filesystem::exists(pathFileHEAD))
		{
			// CA SPEUT CA EXPLOSE
			std::ofstream headFile(pathFileHEAD.string());
			if(headFile.is_open())
			{
				headFile.close();
			}
		}
		return 0;
	}
	else
	{
		std::cout << "Error: The command " << INIT_COMMAND << " couldn't execute because a .git folder already exist in this directory.\n";
		return -1;
	}
}

// Commande gitus add <pathspec>
int Add(string pathGiven)
{
	path filePath{pathGiven};
	// If the path given by the user doesn't exist
	// Show error message
	if (!boost::filesystem::exists(filePath))
	{
		PathDoesntExistError(filePath.string());
		return -1;
	}
	else
	{
		auto fileContent = GetFileContent(filePath.string());
		auto sha = CreateSha(fileContent);
		std::vector<string> writeInFile{filePath.string(), std::to_string(fileContent.length()), fileContent};
		CreateObject(sha, writeInFile);
		//we add the filePath to the index file
		auto pathFolderGit = GetGitFolderPath() / INDEX_FILE;
		if (boost::filesystem::exists(pathFolderGit))
		{
			AddToIndex(sha, filePath);
		}
		else
		{
			PathDoesntExistError(pathFolderGit.string());
			return -1;
		}
	}
	return 0;
}

// Commande gitus commit <msg> <author>
int Commit(const string msg, const string author)
{
	if(IsIndexEmpty())
	{
		std::cout << "Error: A commit can't be executed because there is no file in the staging area.\n";
		return -1;
	}
	else
	{
		auto isFirstTree = false;
		auto latestCommit = GetLatestCommit();
		if (latestCommit.compare("") == 0)
		{
			isFirstTree = true;
		}
		// Va get le sha de l'arbre cree
		auto treeID = CreateTree(isFirstTree);
		auto content = msg + author + latestCommit + treeID;
		auto commitSha = CreateSha(content);
		std::vector<string> infoToWrite{msg, author, latestCommit, treeID};
		CreateObject(commitSha, infoToWrite);

		// Write the latest commit id in the .git/HEAD file
		WriteLatestCommit(commitSha);
		// Clear the index file
		ClearFile( (GetGitFolderPath().append(INDEX_FILE)).string() );
		return 0;
	}
}

											////////////////////////////////////////////
											// ** FUNCTIONS USED FOR OBJECT TREES ** //
											///////////////////////////////////////////

// Method that creates a tree of .git/objects
string CreateTree(bool isFirstTree)
{
	// l'ID du root de l'arbre qu'on va creer
	string rootID;
	// ** C'est pour notre environnement **
	// Cela fait que l'application est portable
	auto rootPath = GetGitFolderPath().parent_path();
	// List of directories and files we don't want in our tree
	// AKA our version of .gitignore
	std::vector<path> excludedPaths{(rootPath / VSCODE_FOLDER), (rootPath / BUILD_FOLDER), (rootPath / GITINORE_FILE), GetGitFolderPath()};
	auto indexFilePath = GetGitFolderPath().append(INDEX_FILE);

	if (isFirstTree)
	{
		rootID = NewTree(rootPath, excludedPaths);
	}
	else
	{
		std::ifstream indexFile(indexFilePath.string());
		if (indexFile.is_open())
		{
			int lineIndex = 0;
			string addedFileSha;
			string addedFilePath;
			string line;
			// for each added file in index
			// we get the sha and path
			while (getline(indexFile, line))
			{
				//If we are on a pair line
				if (lineIndex % 2 == 0)
				{
					addedFilePath = line;
				}
				else
				{
					addedFileSha = line;
					rootID = IterateAndModifyTree(rootPath, GetTreeRootSha(), addedFilePath, addedFileSha);
				}
				lineIndex++;
			}
			indexFile.close();
		}
		else
		{
			SimpleErrorOccurMessage();
		}
	}
	return rootID;
}

string NewTree(path parentPath, std::vector<path> excludedPaths)
{
	std::vector<string> treeContent;
	string content;

	for (auto entry : boost::filesystem::directory_iterator(parentPath))
	{
		string sha;
		// We don't want to iterate in certain directories and files
		// So, if the current path in the iteration is in the excluded list
		if (std::find(excludedPaths.begin(), excludedPaths.end(), entry.path()) == std::end(excludedPaths))
		{
			if (boost::filesystem::is_directory(entry))
			{
				sha = NewTree(entry.path(), excludedPaths);
			}
			else
			{
				sha = CreateSha(GetFileContent((entry.path()).string()));
			}
			treeContent.push_back(sha);
			content += sha;
		}
	}
	auto treeSha = CreateSha(content);
	CreateObject(treeSha, treeContent);
	return treeSha;
}

// Va dans le file du parentSha
// Pour tout les SHA
// On va lire dans son object file si s'en est un
// Si on lit le meme path que addedFilePath (le file ajoute dans le staging area)
// 		on retourne parentSha (l'ancien sha du file dans le staging area)
// si on retourne PAS le meme SHA que celui lu
//		On va changer l'ancien sha avec le nouveau dans le fichier
// 		Et on refait un nouvel obj avec le nouveau contenu et on retourne le son sha (ModifyFileContent)
// Sinon si on retourne le meme SHA (parentSha)
string IterateAndModifyTree(path parentPath, string parentSha, string addedFilePath, string addedFileSha)
{
	auto rootObjectPath = CreateObjectPath(parentSha);
	std::cout << rootObjectPath.c_str() << std::endl;
	std::ifstream objectFile(rootObjectPath.string());
	string line;

	while (getline(objectFile, line))
	{
		auto leafPath = CreateObjectPath(line);
		// We found the file that was added in the index
		if (line.compare(addedFilePath) == 0)
		{
			return addedFileSha;
		}
		else if (boost::filesystem::exists(leafPath))
		{
			auto leafSha = IterateAndModifyTree(parentPath, line, addedFilePath, addedFileSha);
			// Si on a recu le addedFileSha, c'est qu'on a trouve le bon fichier
			if (leafSha.compare(line) != 0)
			{
				objectFile.close();
				return ModifyFileContent(line, addedFileSha, rootObjectPath.string());
			}
		}
		return parentSha;
	}
}



// Code trouvé sur : https://www.boost.org/doc/libs/1_73_0/boost/compute/detail/sha1.hpp
// Mais ne semble pas donner le même résultat que git hash-object
string CreateSha(const string content)
{
	sha1 sha;
	sha.process_bytes(content.c_str(), content.size());
	unsigned int digest[5];
	sha.get_digest(digest);

	std::ostringstream stream;
	for (int index = 0; index < 5; ++index)
	{
		stream << std::hex << std::setfill('0') << std::setw(8) << digest[index];
	}
	return stream.str();
}

// Prend le latestCommitID dans .git/HEAD
// 		S'il y a un latestCommit : retoune son ID
string GetTreeRootSha()
{
	auto latestCommitSha = GetLatestCommit();
	// If there is a latest commit
	if (latestCommitSha.compare("") != 0)
	{
		auto treeRootPath = CreateObjectPath(latestCommitSha);
		if(boost::filesystem::exists(treeRootPath))
		{
			std::ifstream latestCommitFile(treeRootPath.string());
			if (latestCommitFile.is_open())
			{
				int lineIndex = 1;
				string line;
				// for each added file
				// we get the sha and path
				while (getline(latestCommitFile, line))
				{
					//If we are line 4 (line of the sha tree)
					if (lineIndex == 4)
					{
						return line;
					}
					lineIndex++;
				}
				SimpleErrorOccurMessage();
				latestCommitFile.close();
			}
			else
			{
				SimpleErrorOccurMessage();
			}
		}
		else
		{
			PathDoesntExistError(treeRootPath.string());
		}
	}
	return "";
}

											/*		-----------------------  	*\
													Functions used on files  	  
											\*		-----------------------		*/

// Modify the file by switching the oldSha with the newSha
// Creates a git object with the new content and returns his ID (sha)
string ModifyFileContent(string oldSha, string newSha, string filePath)
{
	std::ifstream objFile(filePath);
	std::vector<string> fileLines{};
	string fileContent;
	string line;
	while (getline(objFile, line))
	{
		if (line.compare(oldSha) == 0)
		{
			fileLines.push_back(newSha);
			fileContent += newSha;
		}
		else
		{
			fileLines.push_back(line);
			fileContent += line;
		}
	}
	objFile.close();
	auto newObjSha = CreateSha(fileContent);
	CreateObject(newObjSha, fileLines);
	return newObjSha;
}

// Return the file path contained in the object file
// Arg: The sha of the object file we want to read
// Ret: The path contained in the file
// 		"" if there is no file path with the sha received
string GetFile(string sha)
{
	auto objectPath = GetGitFolderPath().append(OBJECTS_FOLDER);
	objectPath.append(sha.substr(0, 2));
	string filePath;
	if (boost::filesystem::exists(objectPath))
	{
		objectPath.append(sha.substr(2, sha.length()));
		if (boost::filesystem::exists(objectPath))
		{
			std::ifstream objFile(objectPath.string());
			if (objFile)
			{
				getline(objFile, filePath);
			}
			objFile.close();
		}
		else
		{
			PathDoesntExistError(objectPath.string());
		}
	}
	else
	{
		PathDoesntExistError(objectPath.string());
	}

	// If the object file we tried to get wasn't a file containing a path (Commit obj or Tree obj)
	// return an emtpy string
	if (!boost::filesystem::exists(filePath))
	{
		filePath = "";
	}
	//return path
	return filePath;
}

// Retourne un string avec le contenu du fichier
// Arg: Path du fichier qu'on veut le contenu
// Ret: Une string qui contient le contenu du fichier
string GetFileContent(string path)
{
	// lecture du fichier
	string content;
	if (boost::filesystem::exists(path))
	{
		std::ifstream file(path);
		content.assign((std::istreambuf_iterator<char>(file)),
					   (std::istreambuf_iterator<char>()));
		file.close();
	}
	else
	{
		PathDoesntExistError(path);
		content = "";
	}
	return content;
}

void ClearFile(string filePath)
{
	std::ofstream file;
	file.open(filePath, std::ofstream::out | std::ofstream::trunc);
	file.close();
}

void WriteLatestCommit(string latestCommit)
{
	auto headFilePath = GetGitFolderPath().append(HEAD_FILE);
	std::ofstream headFile(headFilePath.string());
	if(headFile.is_open())
	{
		headFile << latestCommit;
		headFile.close();
	}
	else
	{
		SimpleErrorOccurMessage();
	}
}

// Va chercher et retourne l'ID (sha) du latest commit dans le fichier .git/HEAD
string GetLatestCommit()
{
	auto headFilePath = GetGitFolderPath().append(HEAD_FILE);
	string latestCommitID;
	std::ifstream headFile(headFilePath.string());
	if (headFile.is_open())
	{
		getline(headFile, latestCommitID);
		if (latestCommitID.compare("") == 0)
		{
			return "";
		}
		headFile.close();
	}
	return latestCommitID;
}

										/*		--------------------------------  	*\
												Functions for .git/objects paths  	  
										\*		--------------------------------	*/

path CreateObjectPath(string sha)
{
	path rootObjectPath = GetGitFolderPath().append(OBJECTS_FOLDER);
	rootObjectPath.append(sha.substr(0, 2));
	return rootObjectPath.append(sha.substr(2, sha.length()));
}

void CreateObject(string sha, std::vector<string> infoToWrite)
{
	auto pathToObjFolder = GetGitFolderPath() / OBJECTS_FOLDER;
	pathToObjFolder.append(sha.substr(0, 2));
	boost::system::error_code ec;
	if (!boost::filesystem::exists(pathToObjFolder))
	{
		boost::filesystem::create_directory(pathToObjFolder, ec);
		if (ec)
		{
			std::cout << ec.message() << std::endl;
		}
	}
	auto pathToObjFile = pathToObjFolder.append(sha.substr(2, sha.length()));
	if (!boost::filesystem::exists(pathToObjFile))
	{
		std::ofstream objFile;
		objFile.open(pathToObjFile.string(), std::ios::app);
		if (objFile.is_open())
		{
			for (string line : infoToWrite)
			{
				// Need to write with c_str() otherwise it adds quotes ""
				objFile << line.c_str() << std::endl;
			}
			objFile.close();
		}
		else
		{
			SimpleErrorOccurMessage();
		}
	}
	else
	{
		PathDoesntExistError(pathToObjFile.string());
	}
}

// Overkill, mais
// C'est portable si on utilise l'application dans des environnements differents
path GetGitFolderPath()
{
	auto path = boost::filesystem::current_path();
	while (path.filename() != GITUS_FOLDER)
	{
		path = path.parent_path();
	}
	return path.append(GIT_FOLDER);
}

// Write in the index file the added file path with its sha
void AddToIndex(string addedFileSha, path filePath)
{
	std::ofstream indexFile;
	auto pathFolderGit = GetGitFolderPath();
	auto pathFileIndex = pathFolderGit.append(INDEX_FILE);
	// If the file was already in index
	// Overrite with the new sha
	if (AlreadyInIndex(pathFileIndex.string(), filePath.string()))
	{
		ClearFile(pathFileIndex.string());
	}
	indexFile.open(pathFileIndex.string(), std::ios::app);
	if (indexFile.is_open())
	{
		indexFile << filePath.c_str() << std::endl;
		indexFile << addedFileSha.c_str() << std::endl;
		indexFile.close();
	}
	else
	{
		SimpleErrorOccurMessage();
	}
}

// Verify if the file path was already added
bool AlreadyInIndex(string indexFilePath, string addedFilePath)
{
	std::ifstream indexFile(indexFilePath);
	string line;
	if (indexFile.is_open())
	{
		getline(indexFile, line);
		// If the file was already in the index file
		if (line.compare(addedFilePath) == 0)
		{
			return true;
		}
		indexFile.close();
	}
	else
	{
		SimpleErrorOccurMessage();
	}
	return false;
}

bool IsIndexEmpty()
{
	auto indexFilePath = GetGitFolderPath().append(INDEX_FILE);
	std::ifstream indexFile(indexFilePath.string());
	if(indexFile.is_open())
	{
		string line;
		getline(indexFile, line);
		return line.compare("") == 0 ? true : false;
	}
	return false;
}

// Print help message depending on the command
string PrintHelp(const string command)
{
	string message;
	if (command.compare(INIT_COMMAND) == 0)
	{
		message = "Usage: gitus " + INIT_COMMAND;
	}
	else if (command.compare(ADD_COMMAND) == 0)
	{
		message = "Usage: gitus " + ADD_COMMAND + " <pathspec>\n";
	}
	else if (command.compare(COMMIT_COMMAND) == 0)
	{
		message = "Usage: gitus " + COMMIT_COMMAND + " <msg> <author>\n";
	}
	else if (command.compare(HELP_COMMAND) == 0)
	{
		message = "Usage: gitus <command> [<args>]\n\nThese are common gitus commands used in various situations:\ninit\t Create an empty Git repository or reinitialize an existing one\nadd\t Add file contents to the index\ncommit\t Record changes to the repository\n";
	}
	std::cout << message << std::endl;
	return message;
}

// ** Error messages methods ** //
void SimpleErrorOccurMessage()
{
	std::cout << "Sorry, an error occur while executing..\n";
}

void PathDoesntExistError(string path)
{
	std::cout << "Error:\nThe path : " << path << " doesn't exist.\n";
}