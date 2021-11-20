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
#include <stdexcept>
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
#define QasinoAppID "900757278590894090"
#define ServerID "842631771367800852"
#define QasinoServerID "842631771367800852"
#define LabyToken "ODYxODY1Mjk2MDM3NTQzOTY2.YOQAmw.7e4kiaL5QZT11iXLiSudPSLaE5Q"
#define QasinoToken "OTAwNzU3Mjc4NTkwODk0MDkw.YXF9lg.7wcHoTZAm4_oOpECURxC-ITDK9s"
#define GetTextA textManager.GetText
#define DIR_LETTER	'/'
#define Q_ID "640341085297704970"
#define inv "=invalid="
#define nl ""
#define div_ch "~~~~~~~~~~"

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
			if ((dir = opendir("database/questions/")) != NULL) {
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

namespace qasino {
	struct qambler {
		std::string ID = inv;
		std::string nick = inv;
		std::string info[100];

		int GetInt(int index) {
			std::stringstream SS(info[index]);
			int r;
			SS >> r;
			return r;
		}
		bool SetInt(int index, int value) {
			info[index] = std::to_string(value);
			return true;
		}
		int ChangeInt(int index, int change) {
			SetInt(index, GetInt(index) + change);
			return GetInt(index);
		}

		long long GetLLong(int index) {
			std::stringstream SS(info[index]);
			long long r;
			SS >> r;
			return r;
		}
		bool SetLLong(int index, long long value) {
			info[index] = std::to_string(value);
			return true;
		}
		long long ChangeLLong(int index, long long change) {
			SetInt(index, GetInt(index) + change);
			return GetLLong(index);
		}
	};
	
	struct stock {
		std::string name;
		int value[100];
		int Dev;
		int Rand;
		int Add;
		int startvalue;

		int update(int it = 98, bool shift = true) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dis(-1 * Dev, Dev);
			std::normal_distribution<double> dist(0, Dev / 4);
			
			if (shift) {
				for (int i = 0; i <= 98; i++) {
					value[i] = value[i + 1];
				}
			}

			int c = (100 - Rand) * dist(rd) / 100 + Rand * dis(rd) / 100 + Add;
			if (c > Dev) {
				c = Dev;
			}
			if (c < -1 * Dev) {
				c = -1 * Dev;
			}
			printf("%s : %d\n", name.c_str(), c);
			value[it + 1] = value[it] + c;
			return value[it+1] + c;
		}

		bool refresh() {
			value[0] = startvalue;
			value[1] = startvalue;
			for (int i = 0; i <= 98; i++) {
				update(i, false);
			}
			return false;
		}

		std::string print(int height = 10, int highlight = -100) {
			if (height == 1) {
				height = 40;
			}
			std::ofstream ofile;
#ifdef _WIN32
			ofile.open("database\\stocks\\" + name + "Chart.md");
#endif
#ifdef linux
			ofile.open("/home/phoenix/discord-bot/build/database/stocks/" + name + "Chart.md");
#endif
			std::string out = "";
			int Mark[100][100];
			for (int i = 0; i < 100; i++) {
				for (int j = 0; j < 100; j++) {
					Mark[i][j] = 0;
				}
			}
			int grid[100];
			int max = -1, min = INT_MAX;
			for (int i = 0; i < 100; i++) {
				if (value[i] < min) {
					min = value[i];
				}
				if (value[i] > max) {
					max = value[i];
				}
			}
			for (int i = 1; i <= height; i++) {
				grid[i-1] = min + (max - min) * (i - 1) / (height - 1);
			}

			highlight = std::round((double)((double)highlight - (double)min) / (double)((double)max - (double)min) * (double)((double)height - 1));
			int a = -1;
			int b;
			for (int i = 0; i < 100; i++) {
				b = std::round((double)((double)value[i] - (double)min) / (double)((double)max - (double)min) * (double)((double)height - 1));
				if (a != -1) {
					if (b > a) {
						for (int x = a + 1; x < b; x++) {
							Mark[x][i] = 2;
						}
					}
					else {
						for (int x = a - 1; x > b; x--) {
							Mark[x][i] = 2;
						}
					}
				}
				Mark[b][i] = 1;
				a = b;
			}
			for (int i = 0; i < height; i++) {
				if (height - i - 1 == highlight) {
					out += "[!]: ";
				}
				else {
					out += std::to_string(grid[height - 1 - i]) + " ";
				}
				for (int j = 0; j < 100; j++) {
					if (Mark[height - i - 1][j] == 0) {
						if (height - i - 1 != highlight) {
							out += " ";
						}
						else {
							if (j % 2 == 1) {
								out += "~";
							}
							else {
								out += " ";
							}
						}
					}
					else if (Mark[height - i - 1][j] == 1) {
						out += ".";
					}
					else {
						out += "|";
					}
					
				}
				out += "\n";
			}
			ofile << out;
		return out;
		}
		
	};

	stock CreateStock(std::string name, int Dev, int Rand, int Add, int startvalue) {
			stock S;
			S.name = name;
			S.Dev = Dev;
			S.Rand = Rand;
			S.Add = Add;
			S.startvalue = startvalue;
			return S;
	}
	
	enum InfoHeaders {
		SYS_MONEY = 0,
		SYS_GAME_CHIP,
		ECO_BEGGAR,
		ECO_IS_WORKING,
		ECO_WORK_STACK,
		ECO_STOCK, ECO_STOCK_MONEY,
		ECO_INVERSE,
		ECO_INVERSE_MONEY,
		ECO_TRIPLE_LEVERAGE,
		ECO_TRIPLE_LEVERAGE_MONEY,
		ECO_BITCOIN,
		ECO_BITCOIN_MONEY,
		ECO_ISBEGGAR = 20,
		
	};
}