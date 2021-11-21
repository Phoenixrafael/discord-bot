#include "discord.h"
TextManager textManager(10);

using namespace SleepyDiscord;

/*class MultiGameInterface {
protected:
	bool _play; // playing
	int _betting; //betting money player1, 2
	std::vector<SleepyDiscord::User> _player;
	std::vector<std::string> _reactions;

	bool IsPlayer(std::string id);

public:
	virtual void Clear(void);

	virtual const char* GetGameName(void) = 0;

	bool CommandStarter(int betting);
	virtual bool Is(void);
	virtual bool Permit(SleepyDiscord::Message& message);
	virtual bool LocalPermit() = 0;
	virtual bool Process(SleepyDiscord::Message& message);
	virtual void Set(std::string str, SleepyDiscord::Message message);
	virtual bool OnJoin(SleepyDiscord::Message& message) = 0;
};

bool MultiGameInterface::CommandStarter(int betting) {

	_betting = betting;
}*/
struct TextManager::bytemask TextManager::_masks[6] = {
	{ (unsigned char)0x80, (unsigned char)0x00 }, // 1byte
	{ (unsigned char)0xE0, (unsigned char)0xC0 }, // 2byte
	{ (unsigned char)0xF0, (unsigned char)0xE0 }, // 3byte
	{ (unsigned char)0xF8, (unsigned char)0xF0 }, // 4byte
	{ (unsigned char)0xFC, (unsigned char)0xF8 }, // 5byte
	{ (unsigned char)0xFE, (unsigned char)0xFC }, // 6byte
};

int TextManager::GetLetter(const char* word, unsigned int* ord)
{
	if (ord) *ord = 0;

	for (int i = 0; i < (int)sizeof(_masks); i++) {
		if ((*word & _masks[i].mask) == _masks[i].hit) {
			if (ord) {
				for (int j = 0; j < i + 1; j++) {
					if (j == 0) {
						*ord = ~(_masks[i].mask) & *word & 0xFF;
					}
					else {
						*ord <<= 6;
						*ord |= *(word + j) & 0x3F;
					}
					//printf("GetLetter loop j %d ord %08x\n", j, *ord);
				}
			}
			//printf("GetLetter (%x)%x, %d, ord %08x\n", (unsigned char)*word, *((unsigned int *)word), i + 1, ord ? *ord : 0);
			return (i + 1);
		}
	}

	return 1; // cannot be happened...
}

bool TextManager::PostPos(const char* word)
{
	unsigned int ord;
	bool retval = false;
	for (int i = 0; i < (int)strlen(word); ) {
		i += GetLetter(word + i, &ord);
		if (ord >= 0xac00 && ord <= 0xd7ac) {
			retval = (ord - 0xac00) % 28 > 0;
			//printf("ORD checked!\n");
		}
	}

	//printf("PostPos %x, %c\n", *((unsigned int *)word), retval ? 'O' : 'X');

	return retval;
}

void TextManager::ReplaceAll(std::string& content, std::string from, std::string to)
{
	size_t pos;
	while ((pos = content.find(from)) != std::string::npos) {
		content.replace(pos, from.length(), to);
	}
}

TextManager::TextManager(int size) : _currentText(NULL)
{
	_textCache.clear();
	_outputCache.clear();
	_outputCache.resize(size);
}

void TextManager::Initialize(const char* file, const char* origin)
{
	if (_savedFile.length() == 0) {
		std::ifstream t("current_text.txt");
		_savedFile = std::string((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());
	}

	FILE* fp = fopen(file, "r");
	char buf[1024];
	std::string key = "";
	std::string content;

	TextCache textCache;
	if (origin) {
		TextsCache::iterator it = _textCache.find(origin);
		if (it != _textCache.end()) {
			textCache = it->second;
			printf("%s launch, origin from %s\n", file, origin);
		}
		else {
			textCache.clear();
			printf("%s launch, origin %s not found\n", file, origin);
		}
	}
	else {
		textCache.clear();
		printf("%s launch\n", file);
	}

	std::function<void(std::string&, std::string)> OnePara = [this, &textCache](std::string& key, std::string content) {
		if (key != "") {
			//printf("%s : %s\n", key.c_str(), content.c_str());
			textCache[key] = content;
			key = "";
		}
	};

	while (!feof(fp)) {
		if (fgets(buf, 1024, fp) == NULL) break;

		if (*buf == ' ') {
			content.append(buf + 1);
			continue;
		}
		else {
			ReplaceAll(content, "\\n", "\n");
			OnePara(key, content);
		}

		char* p = strchr(buf, ' ');
		if (p) {
			*p = '\0'; p++;
			key = std::string(buf);
			content = std::string(p);
		}
	}
	ReplaceAll(content, "\\n", "\n");
	OnePara(key, content);

	std::pair<TextsCache::iterator, bool> ret = _textCache.insert(std::make_pair(file, textCache));
	if (ret.second && (!_currentText || _savedFile == file)) {
		_currentText = &ret.first->second;
		_currentFile = file;

		if (_savedFile.length() <= 0) {
			_savedFile = file;
		}
	}
}

void TextManager::GetFiles(std::string& names)
{
	names = GetTextA("text_file_current", _currentFile.c_str());
	for (TextsCache::iterator it = _textCache.begin(); it != _textCache.end(); it++) {
		names.append(" : " + it->first);
	}
}

void TextManager::SetFiles(std::string name, std::string& message)
{
	TextsCache::iterator it = _textCache.find(name);
	if (it != _textCache.end()) {
		_currentText = &it->second;
		_currentFile = it->first;

		message = GetTextA("text_file_apply", _currentFile.c_str());

		std::ofstream out("current_text.txt");
		out << _currentFile;
		out.close();
	}
	else {
		message = GetTextA("text_file_failed");
	}
}

void TextManager::ReplacePostPos(std::string& current, const char* word, size_t pos)
{
	size_t p, p2 = 0;
	p = current.find(';', pos + 1);
	if (p == std::string::npos || (p - pos) > 4) {
		return;
	}
	p2 = current.find(';', p + 1);
	if (p2 == std::string::npos || (p2 - p) > 4) {
		return;
	}
	bool postPos = PostPos(word);
	std::string posWord;
	if (postPos) {
		posWord = current.substr(pos, p - pos);
	}
	else {
		posWord = current.substr(p + 1, (p2 - p - 1));
	}
	current.replace(pos, p2 - pos + 1, posWord);
}

const char* TextManager::GetText(const char* key, ...)
{
	std::string& current = _outputCache[_outputOrder];
	_outputOrder++;

	if (_outputOrder >= (int)_outputCache.size()) {
		_outputOrder = 0;
	}

	std::string keyConv;
	if (strchr(key, ' ')) {
		keyConv = key;
		ReplaceAll(keyConv, " ", "_");
		key = keyConv.c_str();
	}

	std::map<std::string, std::string>::iterator it = _currentText->find(key);
	if (it == _currentText->end()) {
		current = "***'" + std::string(key) + "'not found ***\n";
		return current.c_str();
	}

	current = it->second;
	char token[4];

	//printf("GetText length %d\n", current.length());
	if (current.length() <= 1) {
		return "\n";
	}

	va_list ap;
	va_start(ap, key);
	for (int i = 1; ; i++) {
		sprintf(token, "$%d", i);
		size_t pos = current.find(token);
		if (pos == std::string::npos) {
			break;
		}

		const char* word = va_arg(ap, const char*);
		//printf("token %s, word %x, pos %lld\n", token, *((unsigned int *)word), pos);
		ReplacePostPos(current, word, pos + strlen(token));
		current.replace(pos, strlen(token), word);
	}
	va_end(ap);

	return current.c_str();
}

class QasinoBot : public SleepyDiscord::DiscordClient {
public:
	using SleepyDiscord::DiscordClient::DiscordClient;

	std::string GetTextL(const char* key, ...) {
		std::string ret = GetTextA(key);
		ReplaceAll(ret, "\n", "");
		ReplaceAll(ret, "\r", "");
		return ret;
	}
	std::string GetTextR(const char* key, ...) {
		std::string ret = GetTextA(key);
		ReplaceAll(ret, "\r", "");
		return ret;
	}

	std::vector<Server> SL = getServers().vector();
	time_t uptimet = time(0);
	long long uptime = uptimet;

	Server QasinoServer = getServer(QasinoServerID);


	std::vector<qasino::stock> stocks;

	SleepyDiscord::ObjectResponse<SleepyDiscord::Message> sendPrintf(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, const char* format, ...)
	{
		char szBuf[1024] = { 0, };

		va_list lpStart;
		va_start(lpStart, format);
		vsprintf(szBuf, format, lpStart);
		va_end(lpStart);

		return sendMessage(channelID, szBuf);
	}

	std::string hexprint(std::string str)
	{
		std::string output;
		char buf[1000];
		const char* p = str.c_str();
		for (int i = 0; i < (int)str.length(); i++) {
			//printf("i:%d, buf:%s, p:%x\n", i, buf, *(p + i));
			sprintf(buf + ((unsigned long long)i * 2), "%02x", *((unsigned char*)p + i));
			buf[i * 2 + 2] = '\0';
		}
		buf[127] = '\0';
		output = buf;
		return output;
	}

	std::string hexdeprint(std::string inpstr)
	{
		std::string output;
		char str[1000];
		strcpy(str, inpstr.c_str());
		char buf[1000];
		int a;
		for (int i = 0; i < (int)inpstr.length(); i++) {
			a = 0;
			if ('0' <= str[i] && '9' >= str[i]) {
				a += 16 * (str[i] - '0');
			}
			else if ('a' <= str[i] && 'f' >= str[i]) {
				a += 16 * (str[i] - 'a' + 10);
			}
			else {
				output = "ERROR!";
				return "";
			}
			i++;
			if ('0' <= str[i] && '9' >= str[i]) {
				a += str[i] - '0';
			}
			else if ('a' <= str[i] && 'f' >= str[i]) {
				a += str[i] - 'a' + 10;
			}
			else {
				output = "ERROR!";
				return "";
			}
			buf[(i - 1) / 2] = a;
			buf[(i - 1) / 2 + 1] = '\0';
		}
		buf[999] = '\0';
		output = buf;
		return output;
	}

	int sendDM(std::string UserID, std::string Content) {
		Channel C = createDirectMessageChannel(UserID);
		sendMessage(C, Content);
		return 0;
	}

	std::string ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
		return str;
	}

	std::string ToText(std::string inp) {
		ReplaceAll(inp, std::string("`"), std::string("'"));
		inp = "`" + inp + "`";
		return inp;
	}

	qasino::qambler readQamblerInfo(std::string ID) {
		std::ifstream readFile;
#ifdef _WIN32
		readFile.open("database\\qamblers\\" + ID + ".txt");
#endif
#ifdef linux
		printf("linux is detected!\n");
		readFile.open("/home/phoenix/discord-bot/build/database/qamblers/" + ID + ".txt");
#endif
		qasino::qambler Qb;
		Qb.ID = ID;
		if (readFile.is_open()) {
			std::string str;
			int i = 1;
			while (readFile) {
				getline(readFile, str);
				ReplaceAll(str, "\r", "");
				if (str == div_ch) {
					i++;
					if (i >= 100) {
						break;
					}
				}
				else {
					if (i == 1) {
						Qb.nick = str;
					}
					else {
						Qb.info[i - 2] = str;
					}
				}
			}
		}
		else {
			Qb.nick = "";
			for (int i = 0; i < 100; i++) {
				Qb.info[i] = "0";
			}
			Qb.SetInt(qasino::SYS_MONEY, 30000);
			writeQamblerInfo(Qb);
		}
		if (Qb.nick == "") {
			User U = getUser(ID);
			Qb.nick = U.username;
		}
		return Qb;
	}

	bool writeQamblerInfo(qasino::qambler Qb) {
		std::ofstream ofile;
#ifdef _WIN32
		ofile.open("database\\qamblers\\" + Qb.ID + ".txt");
#endif
#ifdef linux
		ofile.open("/home/phoenix/discord-bot/build/database/qamblers/" + Qb.ID + ".txt");
#endif
		std::string out;
		out = Qb.nick + "\n" + div_ch + "\n";
		for (int i = 0; i < 100; i++) {
			out += Qb.info[i] + "\n" + div_ch + "\n";
		}
		ofile << out;
		return true;
	}

	void readStock(qasino::stock& S) {
		std::ifstream readFile;
#ifdef _WIN32
		readFile.open("database\\stocks\\" + S.name + ".txt");
#endif
#ifdef linux
		printf("linux is detected!\n");
		readFile.open("/home/phoenix/discord-bot/build/database/stocks/" + S.name + ".txt");
#endif
		if (readFile.is_open()) {
			std::string str;
			int i = 0;
			while (readFile) {
				getline(readFile, str);
				ReplaceAll(str, "\r", "");
				std::stringstream SS(str);
				int a;
				SS >> a;
				S.value[i] = a;
				i++;
				if (i >= 100) {
					break;
				}
			}
		}
		else {
			S.refresh();
		}
	}

	void writeStock(qasino::stock S) {
		std::ofstream ofile;
#ifdef _WIN32
		ofile.open("database\\stocks\\" + S.name + ".txt");
#endif
#ifdef linux
		printf("linux is detected!\n");
		ofile.open("/home/phoenix/discord-bot/build/database/stocks/" + S.name + ".txt");
#endif

		std::string out= "";
		for (int i = 0; i < 100; i++) {
			out += std::to_string(S.value[i]) + "\n";
		}

		ofile << out;
	}


	Embed QamblerProfile(qasino::qambler Qb) {
		Embed E;
		EmbedField EF;
		E.title = GetTextA("profile-title", Qb.nick.c_str());
		EF.name = GetTextA("profile-nick");
		EF.value = Qb.nick;
		EF.isInline = true;
		E.fields.push_back(EF);
		EF.name = GetTextA("profile-money");
		EF.value = Qb.info[qasino::SYS_MONEY];
		EF.isInline = true;
		E.fields.push_back(EF);
		EF.name = GetTextA("profile-stock");
		EF.value = "";
		for (int i = 0; i < 4; i++) {
			if (Qb.GetInt(qasino::ECO_STOCK + 2 * i) != 0) {
				std::string S = "stock-";
				S += i == 0 ? "sStock" : "";
				S += i == 1 ? "Inverse" : "";
				S += i == 2 ? "Triple" : "";
				S += i == 3 ? "Bitcoin" : "";
				EF.value += GetTextL(S.c_str()) + " **" + std::to_string(Qb.GetInt(qasino::ECO_STOCK + 2 * i)) + GetTextL("profile-s") + "**\n";
			}
		}
		if (EF.value == "") {
			EF.value = GetTextA("profile-null");
		}
		EF.isInline = true;
		E.fields.push_back(EF);
		User U = getUser(Qb.ID);
		E.thumbnail.url = "https://cdn.discordapp.com/avatars/" + U.ID + "/" + U.avatar + ".png";
		E.color = 9983;
		return E;
	
	}

	std::vector<std::string> split(std::string input, char delimiter) {
		std::vector<std::string> answer;
		std::stringstream ss(input);
		std::string temp;

		while (std::getline(ss, temp, delimiter)) {
			answer.push_back(temp);
		}

		return answer;
	}

	void UpdStk() {
		for (int i = 0; i < stocks.size(); i++) {
			if (i != 1) {
				readStock(stocks[i]);
				stocks[i].update();
			}
		}

		for (int i = 0; i < 100; i++) {
			stocks[1].value[i] = 10000 - stocks[0].value[i];
		}
		for (int i = 0; i < stocks.size(); i++) {
			writeStock(stocks[i]);
		}

		schedule([this]() {this->UpdStk(); }, 60000);
	}


	
	void UpdNick(std::string ID, std::string nick, bool IsBeggar = false) {
		qasino::qambler Qb = readQamblerInfo(ID);
		Qb.nick = nick;
		writeQamblerInfo(Qb);
		if (IsBeggar) {
			Qb.info[qasino::ECO_ISBEGGAR] = "false";
			writeQamblerInfo(Qb);
		}
		if (true) {
			printf("%s | %s | %s", QasinoServerID, hexprint(ID).c_str(), hexprint(nick).c_str());
			editMember(QasinoServerID, ID, nick);
		}
	}

	void DiceEdit(std::string MessageID, std::string ChannelID, Embed E, int result) {
		EditMessageParams Ep;
		Ep.content = "_ _";
		Ep.channelID = ChannelID;
		Ep.messageID = MessageID;
		Ep.embed = E;
		std::vector<std::string> Numbers;
		Numbers.push_back("1");
		Numbers.push_back("2");
		Numbers.push_back("3");
		Numbers.push_back("4");
		Numbers.push_back("5");
		Numbers.push_back("6");
		Numbers.push_back("2.5");
		std::vector<std::string> Images;
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910127289457586236/dice1.png");
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910127293215678464/dice2.png");
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910127297225429032/dice3.png");
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910127299922378772/dice4.png");
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910127302434762832/dice5.png");
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910127310970187828/dice6.png");
		Images.push_back("https://media.discordapp.net/attachments/843653570158657547/910288731452346399/Sdice2.5.png");
		Ep.embed.description = GetTextA("dice-end-dsc", Numbers[result-1].c_str());
		Ep.embed.thumbnail.url = Images[result-1];
		editMessage(Ep);
	}

	int RollDice(std::string ChannelID, bool iseasteregg = false, float time = 1, std::string name = "~~~") {
		if (name == "~~~") {
			GetTextL("dice-title");
		}
		Embed E;
		EmbedField EF;
		E.title = name;
		E.description = GetTextL("dice-roll-dsc");
		E.thumbnail.url = "https://media.discordapp.net/attachments/843653570158657547/910283898699800616/diceroll.gif";
		SendMessageParams Sp;
		Sp.embed = E;
		Sp.content = "_ _";
		Sp.channelID = ChannelID;
		Message Msg = sendMessage(Sp);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(1, 6);
		std::uniform_int_distribution<int> esdis(1, 12);
		std::uniform_int_distribution<int> es(1, 1);
		int result = dis(rd);
		if (iseasteregg && esdis(rd) == 1) {
			result = 6 + es(rd);
		}
		schedule([this, result, E, ChannelID, Msg]() {this->DiceEdit(Msg.ID.string(), ChannelID, E, result); }, 1000 * time);
		return result;
	}

	long long CurrentTime() {
		time_t now = time(0);
		long long t = now;
		return t;
	}

	void onReady(Ready readyData) override {
		std::array<AppCommand::Option, 1> option;
		std::array<AppCommand::Option, 2> option2;
		std::array<AppCommand::Option, 3> option3;
		AppCommand::Option::Choice choice;
		std::string choiceVal;

		/*option.at(0).type = AppCommand::Option::Type::USER;
		option.at(0).name = "qambler";
		option.at(0).description = GetTextA("profile-qambler");
		createGlobalAppCommand(QasinoAppID, "profile", GetTextA("profile"), option);

		createGlobalAppCommand(QasinoAppID, "help", GetTextA("help"));

		option3.at(0).type = AppCommand::Option::Type::STRING;
		option3.at(0).name = "to-do";
		choice.value = "buy";
		choice.name = GetTextL("stock-buy");
		option3.at(0).choices.push_back(std::move(choice));
		choice.value = "sell";
		choice.name = GetTextL("stock-sell");
		option3.at(0).choices.push_back(std::move(choice));
		choice.value = "chart";
		choice.name = GetTextL("stock-chart");
		option3.at(0).choices.push_back(std::move(choice));
		option3.at(0).description = GetTextL("stock-to-do");
		option3.at(0).isRequired = true;

		option3.at(1).type = AppCommand::Option::Type::STRING;

		choice.value = "Stock";
		choice.name = GetTextL("stock-sStock");
		option3.at(1).choices.push_back(std::move(choice));

		choice.value = "Bitcoin";
		choice.name = GetTextL("stock-Bitcoin");
		option3.at(1).choices.push_back(std::move(choice));

		choice.value = "Triple";
		choice.name = GetTextL("stock-Triple");
		option3.at(1).choices.push_back(std::move(choice));

		choice.value = "Inverse";
		choice.name = GetTextL("stock-Inverse");
		option3.at(1).choices.push_back(std::move(choice));

		option3.at(1).name = "stock";
		option3.at(1).description = GetTextL("stock-stock");
		option3.at(1).isRequired = true;

		option3.at(2).type = AppCommand::Option::Type::INTEGER;
		option3.at(2).name = "count";
		option3.at(2).description = GetTextL("stock-count");
		option3.at(2).isRequired = true;
		createGlobalAppCommand(QasinoAppID, "stock", GetTextL("stock"), option3);

		option.at(0).type = AppCommand::Option::Type::STRING;
		option.at(0).name = "nickname";
		option.at(0).description = GetTextA("nick-nickname");
		option.at(0).isRequired = true;
		createGlobalAppCommand(QasinoAppID, "nick", GetTextA("nick"), option);

		option.at(0).type = AppCommand::Option::Type::STRING;
		option.at(0).name = "areyousure";
		option.at(0).description = GetTextA("getAdmin-are-you-sure");
		option.at(0).isRequired = true;

		choice.value = "Yes";
		choice.name = GetTextA("getAdmin-yes");
		option.at(0).choices.push_back(std::move(choice));

		choice.value = "No";
		choice.name = GetTextL("getAdmin-no");
		option.at(0).choices.push_back(std::move(choice));

		choice.value = "YeNo";
		choice.name = GetTextL("getAdmin-yeno");
		option.at(0).choices.push_back(std::move(choice));

		createGlobalAppCommand(QasinoAppID, "getadmin", GetTextA("getAdmin"), option);

		option.at(0).type = AppCommand::Option::Type::INTEGER;
		option.at(0).name = "money";
		option.at(0).description = GetTextA("beg-money");
		option.at(0).isRequired = true;
		createGlobalAppCommand(QasinoAppID, "beg", GetTextA("beg"), option);

		createGlobalAppCommand(QasinoAppID, "dice", GetTextA("dice"));*/

		stocks.push_back(qasino::CreateStock("Stock", 100, 20, 1, 3000));
		stocks.push_back(qasino::CreateStock("Inverse", 100, 20, 1, 3000));
		stocks.push_back(qasino::CreateStock("Triple", 900, 60, 0, 9000));
		stocks.push_back(qasino::CreateStock("Bitcoin", 3000, 100, 0, 50000));
		
		UpdStk();
	}

	void onMessage(SleepyDiscord::Message message) override {
		if (message.author.ID.string() == Q_ID) {
			std::vector<std::string> input;
			input = split(message.content, ' ');
			if (input.size() != 0) {
				if (input[0] == "||nick") {
					std::string nickID = input[1].substr(3, input[1].size() - 4);
					std::string nick = input[2];
					UpdNick(nickID, nick);
				}
				if (input[0] == "||money") {
					std::string nickID = input[1].substr(3, input[1].size() - 4);
					std::stringstream SS(input[2]);
					int i;
					SS >> i;
					qasino::qambler Qb = readQamblerInfo(nickID);
					Qb.SetInt(qasino::SYS_MONEY, i);
					writeQamblerInfo(Qb);
				}
				if (input[0] == "||encode") {
					sendMessage(message.channelID, hexprint(input[1]));
				}
				if (input[0] == "||decode") {
					sendMessage(message.channelID, hexdeprint(input[1]));
				}
				if (input[0] == "||dicetest") {
					RollDice(message.channelID, false, 3);
				}
			}
		}
	}

	void onInteraction(Interaction interaction) override {
		std::srand(static_cast<int>(std::time(0)));
		SleepyDiscord::Interaction::Response response;
		SleepyDiscord::WebHookParams followUp;
		std::string customid = interaction.data.customID;

		Channel GC = getChannel(interaction.channelID);
		if (GC.type == Channel::DM) {
			response.data.content = GetTextA("DM_X");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
			return;
		}
		qasino::qambler iQB = readQamblerInfo(interaction.member.ID);
		if (CurrentTime() - iQB.GetLLong(qasino::ECO_BEGGAR) <= 7200) {
			response.data.content = GetTextA("beggar-response", iQB.nick.substr(0, 6).c_str(), std::to_string(iQB.GetLLong(qasino::ECO_BEGGAR)).c_str());
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			createInteractionResponse(interaction, interaction.token, response);
			return;
		}
		else if (iQB.info[qasino::ECO_ISBEGGAR] == "true"){
			UpdNick(iQB.ID, iQB.nick.substr(7), true);
		}

		if (interaction.type == Interaction::Type::MessageComponent) {

		}
		else if (interaction.data.name == "help") {
			response.data.content = GetTextA("botcommand");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
		}
		else if (interaction.data.name == "profile") {
			std::string UID;
			if(interaction.data.options.size() == 0) {
				UID = interaction.member.ID;
			}
			else {
				UID = interaction.data.options.at(0).value.GetString();
			}
			qasino::qambler Qb = readQamblerInfo(UID);
			response.data.content = "_ _";
			response.data.embeds.push_back(QamblerProfile(Qb));
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			createInteractionResponse(interaction, interaction.token, response);
		}
		else if (interaction.data.name == "stock") {
			std::string todo = interaction.data.options.at(0).value.GetString();
			std::string stock = interaction.data.options.at(1).value.GetString();
			int stn;
			for (int i = 0; i < stocks.size(); i++) {
				if (stocks.at(i).name == stock) {
					stn = i;
					break;
				}
			}
			qasino::qambler Qb = readQamblerInfo(interaction.member.ID);
			if (todo == "buy") {
				int count = interaction.data.options.at(2).value.GetInt();
				if (Qb.GetInt(qasino::SYS_MONEY) + (-1 * count * stocks.at(stn).value[99]) < 0 || count <= 0) {
					response.data.content = GetTextA("stock_buy_fail");
					response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
					response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
					createInteractionResponse(interaction, interaction.token, response);
					return;
				}

				Qb.ChangeInt(qasino::SYS_MONEY, -1 * count * stocks.at(stn).value[99]);
				Qb.ChangeInt(qasino::ECO_STOCK + 2 * stn, count);
				Qb.ChangeInt(qasino::ECO_STOCK_MONEY + 2 * stn, count * stocks.at(stn).value[99]);
				writeQamblerInfo(Qb);

				response.data.content = GetTextA("stock_buy_success");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
			if (todo == "sell") {
				int count = interaction.data.options.at(2).value.GetInt();
				if (Qb.GetInt(qasino::ECO_STOCK + 2 * stn) + (-1 * count) < 0 || count <= 0) {
					response.data.content = GetTextA("stock_sell_fail");
					response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
					response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
					createInteractionResponse(interaction, interaction.token, response);
					return;
				}

				Qb.ChangeInt(qasino::SYS_MONEY, count * stocks.at(stn).value[99] * 97/100);
				Qb.ChangeInt(qasino::ECO_STOCK + 2 * stn, -1 * count);
				Qb.ChangeInt(qasino::ECO_STOCK_MONEY + 2 * stn, -1 * count * stocks.at(stn).value[99]);
				writeQamblerInfo(Qb);

				response.data.content = GetTextA("stock_sell_success");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
			if (todo == "chart") {
				int count = interaction.data.options.at(2).value.GetInt();
				if (count <= 0) {
					count = 40;
				}
				if (Qb.GetInt(qasino::ECO_STOCK + 2 * stn) != 0) {
					stocks.at(stn).print(count, Qb.GetInt(qasino::ECO_STOCK_MONEY + 2 * stn) / Qb.GetInt(qasino::ECO_STOCK + 2 * stn)* 100/97);
				}
				else {
					stocks.at(stn).print(count);
				}
				
				response.data.content = GetTextA("stock_chart_loading");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);

				SendMessageParams Sp;
				Sp.content = GetTextA("stock_chart_success", Qb.nick.c_str(), stocks.at(stn).name.c_str(), std::to_string(stocks.at(stn).value[99]).c_str());
				Sp.channelID = interaction.channelID;
				uploadFile(Sp, "database\\stocks\\" + stocks.at(stn).name + "Chart.md");
			}
		}
		else if (interaction.data.name == "nick") {
			std::string nick = interaction.data.options.at(0).value.GetString();
			qasino::qambler Qb = readQamblerInfo(interaction.member.ID);
			if (Qb.GetInt(qasino::SYS_MONEY) >= 100000) {
				UpdNick(std::move(interaction.data.ID), nick);
				response.data.content = GetTextA("nick-success", nick.c_str());
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
			else {
				response.data.content = GetTextA("qmoney-needed", "100000");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
		}
		else if (interaction.data.name == "getadmin") {
			std::string choice = interaction.data.options.at(0).value.GetString();
			if (choice == "Yes") {
				response.data.content = GetTextA("getadmin-error");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
			else if (choice == "No") {
				response.data.content = GetTextA("getadmin-fail");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
			else if (choice == "YeNo") {
				response.data.content = GetTextA("getadmin-yeno");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
		}
		else if (interaction.data.name == "beg") {
			qasino::qambler Qb = readQamblerInfo(interaction.member.ID);
			int money = interaction.data.options.at(0).value.GetInt();
			long long t = CurrentTime();
			Qb.info[qasino::ECO_BEGGAR] = std::to_string(t);
			Qb.info[qasino::ECO_ISBEGGAR] = "true";
			writeQamblerInfo(Qb);
			if (money < 10 || money > 20000) {
				response.data.content = GetTextA("beg-fail-over", Qb.nick.c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), Qb.nick.c_str(), Qb.nick.c_str());
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				createInteractionResponse(interaction, interaction.token, response);
				UpdNick(std::move(interaction.member.ID), GetTextL("beg-fool") + " " + Qb.nick);
			}
			else {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<int> dis(0, 20000 / money + 3);
				if (dis(rd) <= 3) {
					response.data.content = GetTextA("beg-fail", Qb.nick.c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), Qb.nick.c_str(), Qb.nick.c_str());
					response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
					createInteractionResponse(interaction, interaction.token, response);
					UpdNick(std::move(interaction.member.ID), GetTextL("beg-beggar") + " " + Qb.nick);
				}
				else {
					response.data.content = GetTextA("beg-success", Qb.nick.c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), std::to_string(money).c_str(), Qb.nick.c_str(), Qb.nick.c_str());
					response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
					createInteractionResponse(interaction, interaction.token, response);
					UpdNick(std::move(interaction.member.ID), GetTextL("beg-beggar") + " " + Qb.nick);
					Qb.ChangeInt(qasino::SYS_MONEY, money);
				}
			}
			std::string iID = interaction.member.ID;
			schedule([this, iID, Qb]() {this->UpdNick(iID, Qb.nick, true); }, 1000 * 7200);
		}
		else if (interaction.data.name == "dice") {
			response.data.content = GetTextA("dice-process");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
			RollDice(interaction.channelID, false, 3);
		}
		else {

		}
	}
};


int main()
{
	printf("QasinoBot execute\n");
	textManager.Initialize("qasino.txt");

	QasinoBot qasinoClient(QasinoToken, SleepyDiscord::Mode::USER_CONTROLED_THREADS);
	qasinoClient.setIntents(Intent::SERVER_MESSAGES, Intent::DIRECT_MESSAGES, Intent::SERVER_MESSAGE_REACTIONS);
	qasinoClient.run();
}

// QasinoBot server script

// Made by Q_
// Q¤Ñ#0283
