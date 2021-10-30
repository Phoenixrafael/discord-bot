#include "sleepy_discord/sleepy_discord.h"
#include "IO_file.h"
#include <cstdio>
#include <random>
#include <cstring>
#include <string>
#include <uchar.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <locale>
#if defined(_WIN32) || defined(_WIN64)
#include <codecvt>
#include <Windows.h>
#include <Mmsystem.h>
#include "dirent.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#endif
#include <sstream>
#include <fstream>
#include <set>
#include <map>

#define AppID "861865296037543966"
#define ServerID "842631771367800852"
#define Token "ODYxODY1Mjk2MDM3NTQzOTY2.YOQAmw.7e4kiaL5QZT11iXLiSudPSLaE5Q"
#define GetTextA textManager.GetText
#define DIR_LETTER	'/'
#define Q_ID "640341085297704970"
#define inv "=invalid="
#define nl ""

class MyClientClass;

class TextManager {
private:
	typedef std::map<std::string, std::string> TextCache;
	typedef std::map<std::string, TextCache> TextsCache;
	TextsCache _textCache;
	TextCache* _currentText;
	std::string _currentFile;
	std::vector<std::string> _outputCache;
	std::string _savedFile;
	int _outputOrder;

	bool PostPos(const char* word);
	void ReplacePostPos(std::string& current, const char* word, size_t pos);
	void ReplaceAll(std::string& content, std::string from, std::string to);

public:
	static struct bytemask {
		unsigned char mask;
		unsigned char hit;
	} _masks[6];

	TextManager(int size);
	void Initialize(const char* file, const char* origin = NULL);
	const char* GetText(const char* key, ...);
	int GetLetter(const char* word, unsigned int* ord = NULL);
	void GetFiles(std::string& names);
	void SetFiles(std::string name, std::string& message);
};

namespace ibot {

	bool startsWith(std::string string, std::string compare) {
		for (int i = 0; i < string.size(); i++) {
			if (compare.size() > i) {
				if (compare[i] != string[i]) {
					return 0;
				}
			}
			else {
				return 1;
			}
		}
	}
	struct MultipleAnswer {
		std::string multipleanswer = inv;
		std::string question = inv;
	};

	std::vector<MultipleAnswer> mnl() {
		std::vector<MultipleAnswer> mnlr;
		mnlr.clear();
		return mnlr;
	}

	struct Questioninfo {
		std::string serverid = inv;
		std::string name = inv;
		std::string answer = inv;
		std::vector<MultipleAnswer> multipleanswer;
		std::string channelid = inv;
		std::string roleid = inv;
		std::string hint = inv;
		std::string permission = inv;
		/*std::string invt(std::string S) {
			if (S == inv) {
				return GetTextA("isinvalid");
			}
			return S;
		}
		std::string QuesitonInfo() {
			std::string R;
			if (multipleanswer.size() == 0) {
				R = GetTextA("manswer-non");
			}
			else {
				for (int i = 0; i < multipleanswer.size(); i++) {
					R += GetTextA("questioninfo-manswer", multipleanswer[i].multipleanswer.c_str(), multipleanswer[i].question.c_str());
				}
			}
			std::string S;
			if (permission == inv) {
				S = GetTextA("questioninfo-ev");
			}
			else {
				S = GetTextA("questioninfo-pr", permission.c_str());
			}
			R = GetTextA("questioninfo", invt(name).c_str(), invt(answer).c_str(), R.c_str(), invt(hint).c_str(), invt(channelid).c_str(), invt(roleid).c_str(), S);
		}*/
	};

	struct Serverinfo {
		std::string serverid;
		bool release = 0;
		int hinttime = 30;
		std::string logchannel = inv;
		std::vector<std::string> QuestionList() {
			std::vector<std::string> R;
			DIR* dir;
			struct dirent* ent;
			if ((dir = opendir("database\\questions\\")) != NULL) {
				while ((ent = readdir(dir)) != NULL) {
					std::string S = ent->d_name;
					S = S.substr(0, S.size() - 4);
					if (startsWith(S, serverid)) {
						S = S.substr(19);
						R.push_back(S);
						printf("%s\n", S.c_str());
					}
				}
				closedir(dir);
			}
			else {
				/* could not open directory */
				perror("");
				return R;
			}
			return R;
		}
	};
}