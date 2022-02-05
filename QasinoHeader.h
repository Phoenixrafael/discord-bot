#include "discord.h"

using std::endl; using std::string;

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
			value[it + 1] = abs(value[it] + c);
			return abs(value[it + 1] + c);
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
		SYS_SPAMTIME = 21
	};

	struct card {
		int icon;
		int number;
		int joker;
		int mark;
		int num() {
			if (joker == 0) {
				return icon * 13 + number;
			}
			else {
				return 52 + joker;
			}
		}
		std::string emoji() {
			std::vector<std::string> FirstRed;
			std::vector<std::string> FirstBlack;
			std::vector<std::string> Second;

			std::string ret;

			Second.push_back("<:card_spade:917005102290702376>");
			Second.push_back("<:card_heart:917005102966005770>");
			Second.push_back("<:card_club:917005102823399434>");
			Second.push_back("<:card_diamond:917005101883850805>");

			FirstRed.push_back("<:card_red_1:917005102051651604>");
			FirstRed.push_back("<:card_red_2:917005102106169344>");
			FirstRed.push_back("<:card_red_3:917005102064218112>");
			FirstRed.push_back("<:card_red_4:917005102060032070>");
			FirstRed.push_back("<:card_red_5:917005102072610816>");
			FirstRed.push_back("<:card_red_6:917005101976141855>");
			FirstRed.push_back("<:card_red_7:917005102232002600>");
			FirstRed.push_back("<:card_red_8:917005102148120626>");
			FirstRed.push_back("<:card_red_9:917005101950967840>");
			FirstRed.push_back("<:card_red_10:917005102127153182>");
			FirstRed.push_back("<:card_red_J:917005102122946570>");
			FirstRed.push_back("<:card_red_Q:917005101762215997>");
			FirstRed.push_back("<:card_red_K:917005102265536552>");

			FirstBlack.push_back("<:card_black_1:917005102563332096>");
			FirstBlack.push_back("<:card_black_2:917005101737050172>");
			FirstBlack.push_back("<:card_black_3:917005102013874176>");
			FirstBlack.push_back("<:card_black_4:917005101774823505>");
			FirstBlack.push_back("<:card_black_5:917005101984538624>");
			FirstBlack.push_back("<:card_black_6:917005101934182450>");
			FirstBlack.push_back("<:card_black_7:917005101611237407>");
			FirstBlack.push_back("<:card_black_8:917005101623816303>");
			FirstBlack.push_back("<:card_black_9:917005101988708372>");
			FirstBlack.push_back("<:card_black_10:917005102164873216>");
			FirstBlack.push_back("<:card_black_J:917005101904855090>");
			FirstBlack.push_back("<:card_black_Q:917005102269751326>");
			FirstBlack.push_back("<:card_black_K:917005101799997461>");

			if (number != -1) {
				if (icon == 0 || icon == 2) {
					ret = FirstBlack[number];
				}
				else {
					ret = FirstRed[number];
				}
			}
			else {
				ret = joker == 0 ?
					"<:card_black_joker1:918462066602418176>" :
					"<:card_red_joker1:918462066749222973>";
			}

			if (number != -1) {
				ret += Second[icon];
			}
			else {
				ret += joker == 0 ?
					"<:card_black_joker2:918462066925371392>" :
					"<:card_red_joker2:918462067843952680>";
			}
			return ret;
		}
	};

	card numberToCard(int a) {
		card C;
		if (a <= 51) {
			C.icon = a / 13;
			C.number = a % 13;
			C.joker = 0;
		}
		else {
			C.joker = a - 51;
			C.icon = -1;
			C.number = -1;
		}
		C.mark = 0;
		return C;
	}

	void ShuffleDeck(std::vector<card>& D) {
		std::vector<card> R;
		int size = D.size();
		for (int i = 0; i < size; i++) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> dis(0, size - 1 - i);
			int r = dis(rd);
			R.push_back(D[r]);
			std::vector<card>::iterator it = D.begin();
			D.erase(it + r);
		}
		D = R;
	}

	std::vector<card> ClassicDeck(bool shuffled = false, bool joker = true) {
		std::vector<card> R;
		for (int i = 0; i < 54 - (1 - joker) * 2; i++) {
			R.push_back(numberToCard(i));
		}
		if (shuffled) {
			ShuffleDeck(R);
		}
		return R;
	}

	card cardPick(std::vector<card>& D, std::vector<card> Restore) {
		if(D.size() == 0){
			D = Restore;
		}
		card C = D.back();
		D.pop_back();
		return C;
	}

	/*--Wrote for test--*/

	void printCard(card C) {
		std::string icon;
		switch (C.icon) {
		case 0:
			icon = "spade";
			break;
		case 1:
			icon = "heart";
			break;
		case 2:
			icon = "club";
			break;
		case 3:
			icon = "diamond";
			break;
		case -1:
			icon = "joker";
		}
		printf("card : %s\n", (icon + " " + std::to_string(C.number + 1)).c_str());
	}

	void printDeck(std::vector<card> V) {
		for (int i = 0; i < V.size(); i++) {
			printCard(V[i]);
		}
	}

	std::string cardBack() {
		return "<:card_back_a:932829578559377428><:card_back_b:932829578542612500>";
	}
}

typedef std::vector<qasino::card> deck;

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
	std::string _messageID;

public:
	virtual bool Process(SleepyDiscord::Interaction interaction, bool start);
	std::string GetName() { return _name; };
	std::string GetID() { return _gameID; };
	std::string GetPlace() { return _channel.ID; };
	std::string GameMessage() { return _messageID; };
	void SetMessage(std::string S) { _messageID = S; }
	bool IsPlaying() { return _playing; };
	int GetBetting() { return _betting; };
	qasino::qambler GetPlayer() { return _player; };
	bool OnStart(SleepyDiscord::Interaction interaction);
	bool Clear();
	bool EndGame(int result, int money);
	bool GetLeastBet() { return _leastbet; };
	SoloGame(std::string name, std::string id, int leastbetting = 3) : _name(name), _gameID(id), _leastbet(leastbetting) {
		Clear();
	}
};

/*class TwoPGame {
protected:
	qasino::qambler _player[2];
	int _betting[2];
	int _leastbet;
	SleepyDiscord::Channel _channel;
	bool _playing;
	std::string _name;
	std::string _displayname;
	std::string _gameID;
	std::string _messageID;

public:
	virtual bool Process(SleepyDiscord::Interaction interaction, bool start);
	bool Clear();
	bool OnStart(SleepyDiscord::Interaction interaction);
	bool EndGame(int result, int money);
	std::string GetName() { return _name; };
	std::string GetID() { return _gameID; };
	std::string GameMessage() { return _messageID; };
	void SetMessage(std::string S) { _messageID = S; }
	bool IsPlaying() { return _playing; };
	int GetBetting(int i = -1) { return i==-1?_betting[0]:_betting[i]; };
	qasino::qambler GetPlayer(int i) { return _player[i]; };
	bool GetLeastBet() { return _leastbet; };
	TwoPGame(std::string name, std::string id, int leastbetting = 3) : _name(name), _gameID(id), _leastbet(leastbetting) {
		Clear();
	}
};*/

class DiceBet : public SoloGame {
	std::string _bet;
	int _count;
	int _table;
	SleepyDiscord::Message _dicem;
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

class BlackJack : public SoloGame {
	int _score;
	int _dealer;
	bool _stand;
	bool _doubledown;
	deck _deck;
	deck _playerPacket;
	deck _dealerPacket;

	virtual bool Clear();
	bool Process(SleepyDiscord::Interaction interaction, bool start);
	int CardToScore(qasino::card C);
public:
	BlackJack(std::string id) : SoloGame("blackjack", id, 1) {
		Clear();
	}

};

class AnticipationAndConfirmation : public SoloGame {
	std::vector<int> _answer;
	int _turn;
	int _dif;
	float _rew;
	int _nor;
	std::vector<std::vector<int>> _history;
	virtual bool Clear();
	bool Process(SleepyDiscord::Interaction interaction, bool start);
	SleepyDiscord::Embed AACEmbed();
	std::string Result(std::vector<int> v);
public:
	AnticipationAndConfirmation(std::string id, int dif, float rew, int nor) : SoloGame("aac", id, 20) {
		_rew = rew;
		_nor = nor;
		_dif = dif;
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

	deck _globalDeck;

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
	void DiceEdit(std::string MessageID, std::string ChannelID, SleepyDiscord::Embed E, int result, bool Delete);
	int RollDice(std::string ChannelID, bool iseasteregg, float time, std::string name, bool Delete, std::string editmessage);

	void saveMessage(SleepyDiscord::Message message);

	void onReady(SleepyDiscord::Ready readyData) override;
	void onMessage(SleepyDiscord::Message message) override;
	void onInteraction(SleepyDiscord::Interaction interaction) override;

	void UpdSt1();
	void UpdSt2();
};