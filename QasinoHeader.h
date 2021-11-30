#include "discord.h"

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
			return value[it + 1] + c;
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
				grid[i - 1] = min + (max - min) * (i - 1) / (height - 1);
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
		SYS_IS_PLAYING,
		SYS_GAMEID,
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

class QasinoBot;

class SoloGame {
protected:
	qasino::qambler _player;
	int _betting;
	int _leastbet;
	SleepyDiscord::Channel _channel;
	bool _playing;
	std::string _name;
	std::string _displayname;
	std::string _gameID;

public:
	virtual bool Process(SleepyDiscord::Interaction interaction, bool start);
	std::string GetName() { return _name; };
	std::string GetID() { return _gameID; };
	bool IsPlaying() { return _playing; };
	int GetBetting() { return _betting; };
	qasino::qambler GetPlayer() { return _player; };
	bool OnStart(SleepyDiscord::Interaction interaction);
	bool Clear();
	bool GetLeastBet() { return _leastbet; };
	SoloGame(std::string name, std::string id, int leastbetting = 3) : _name(name), _gameID(id), _leastbet(leastbetting){
		Clear();
	}
};

class DiceBet : public SoloGame {
	std::string _bet;
	int _count;
	int _table;
	virtual bool Clear();
	bool Process(SleepyDiscord::Interaction interaction, bool start);
	SleepyDiscord::Embed Success(int result);
	SleepyDiscord::Embed Fail(int result);
	SleepyDiscord::Embed Stop();
public:
	DiceBet(std::string id) : SoloGame("dice-bet", id, 10) {
		Clear();
	}
};

class QasinoBot : public SleepyDiscord::DiscordClient {
public:
	using SleepyDiscord::DiscordClient::DiscordClient;

	std::vector<SleepyDiscord::Server> _sl;
	time_t _uptimet;
	long long _uptime;
	SleepyDiscord::Server QasinoServer;
	std::vector<qasino::stock> stocks;
	void Clear();
	static long long CurrentTime();

	DiceBet* dicebet;

	std::vector<SoloGame*> _sologames;

	static std::string GetTextL(const char* key, ...);
	static std::string GetTextR(const char* key, ...);

	SleepyDiscord::ObjectResponse<SleepyDiscord::Message> sendPrintf(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, const char* format, ...);

	int sendDM(std::string UserID, std::string Content);

	qasino::qambler readQamblerInfo(std::string ID);
	bool writeQamblerInfo(qasino::qambler Qb);
	void readStock(qasino::stock& S);
	void writeStock(qasino::stock S);
	SleepyDiscord::Embed qamblerProfile(qasino::qambler Qb);
	std::vector<std::string> split(std::string input, char delimiter);
	void UpdStk();
	void UpdNick(std::string ID, std::string nick, bool IsBeggar);
	void DiceEdit(std::string MessageID, std::string ChannelID, SleepyDiscord::Embed E, int result);
	int RollDice(std::string ChannelID, bool iseasteregg, float time, std::string name);

	void onReady(SleepyDiscord::Ready readyData) override;
	void onMessage(SleepyDiscord::Message message) override;
	void onInteraction(SleepyDiscord::Interaction interaction) override;

};