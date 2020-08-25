#define CATCH_CONFIG_MAIN

// RTFM catch2:
// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#include "catch.hpp"
#include <gituscommands.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/uuid/detail/sha1.hpp>

bool InitCreatedCorrectly();
string GetStagedFile();
void RemoveGitFolder();
bool VerifyCommitInfo(string path, string msg, string author);

TEST_CASE("Init Tests")
{
	RemoveGitFolder();
	
	// premier init
	// verifier si tout creer correctement
	Init();
	REQUIRE(InitCreatedCorrectly());

	// si on a deja init et refait un init
	//verifier qu'on ne cree/ovewrite pas nos fichier/directory
	REQUIRE(Init() == -1);

	// init help
	REQUIRE((PrintHelp(INIT_COMMAND)).compare("Usage: gitus " + INIT_COMMAND) == 0);
}

TEST_CASE("Add Tests")
{
	RemoveGitFolder();
	Init();
	//si premier add
	auto rootPath = (GetGitFolderPath().parent_path()) / "gitus.cpp";
	//verifie que executer sans problem
	REQUIRE(Add(rootPath.string()) == 0);

	string stagedFilePath = GetStagedFile();
	REQUIRE(boost::filesystem::exists(stagedFilePath));

	// verifier si add un fichier inexistant
	auto wrongFile = boost::filesystem::current_path() / "notActuallyAFile.cpp";
	REQUIRE(Add(wrongFile.string()) == -1);

	// verifier add help
	REQUIRE((PrintHelp(ADD_COMMAND)).compare("Usage: gitus " + ADD_COMMAND + " <pathspec>\n") == 0);
}

TEST_CASE("Commit Tests")
{
	RemoveGitFolder();
	Init();

	// Essayer de commit sans avoir ajoute de fichier dans le staging area
	REQUIRE(Commit("Non-authorized", "commit") == -1);
	
	string msg = "Authorized";
	string author = "commit";
	auto rootPath = (GetGitFolderPath().parent_path()) / "gitus.cpp";
	Add(rootPath.string());
	REQUIRE(Commit(msg, author) == 0);

	// Verifie que le staging area (fichier index) se vide apres avoir commit
	REQUIRE(IsIndexEmpty());

	// Verifie qu'il y a un latest commit id dans le fichier .git/HEAD
	REQUIRE(GetLatestCommit().compare("") != 0);

	// Verifie s'il existe un commit object dans .git/objects avec le latest commit id
	path objPath = CreateObjectPath(GetLatestCommit());
	REQUIRE(boost::filesystem::exists(objPath.string()));

	// Verifie que l'information du commit object (msg et author)
	REQUIRE(VerifyCommitInfo(objPath.string(), msg, author));

}

bool InitCreatedCorrectly()
{
	auto allCreatedCorretly = true;
	auto pathFolderGit = GetGitFolderPath();
	auto pathFolderObject = pathFolderGit / OBJECTS_FOLDER;
	auto pathFileIndex = pathFolderGit / INDEX_FILE;
	auto pathFileHEAD = pathFolderGit / HEAD_FILE;

	// verifie if .git directory exist
	if (!boost::filesystem::exists(pathFolderGit))
	{
		allCreatedCorretly = false;
		std::cout << ".git was not created correctly" << std::endl;
	}
	// verifie if index file exist
	if (!boost::filesystem::exists(pathFileIndex))
	{
		allCreatedCorretly = false;
		std::cout << "index was not created correctly" << std::endl;
	}
	// verifie if HEAD file exist
	if (!boost::filesystem::exists(pathFileHEAD))
	{
		allCreatedCorretly = false;
		std::cout << "HEAD was not created correctly" << std::endl;
	}
	// verifie if objects directory exist
	if (!boost::filesystem::exists(pathFolderObject))
	{
		allCreatedCorretly = false;
		std::cout << "objects was not created correctly" << std::endl;
	}
	return allCreatedCorretly;
}

string GetStagedFile()
{
	auto pathFolderGit = GetGitFolderPath();
	std::ifstream indexFile((pathFolderGit.append(INDEX_FILE)).string());
	string addedFilePath = "";
	int lineIndex = 0;
	if (indexFile.is_open())
	{
		getline(indexFile, addedFilePath); 
		indexFile.close();
	}
	return addedFilePath;
}

// Used to start the tests on a fresh repo
void RemoveGitFolder()
{
	// Suprime le .git s'il existe deja
	auto pathFolderGit = GetGitFolderPath();
	if (boost::filesystem::exists(pathFolderGit))
	{
		boost::filesystem::remove_all(pathFolderGit);
	}
}

// Verify if the msg and the author in the commit file is the same that was commited
bool VerifyCommitInfo(string path, string msg, string author)
{
	std::ifstream commitFile(path);
	size_t iterations = 0;
	if (commitFile.is_open())
	{
		string line;
		while(iterations != 2)
		{
			getline(commitFile, line);
			if(iterations == 0 && msg.compare(line) != 0)
			{
				return false;
			}
			else if(iterations == 1 && author.compare(line) != 0)
			{
				return false;
			}
			iterations++;
		}
		commitFile.close();
		return true;
	}
	return false;	
}
