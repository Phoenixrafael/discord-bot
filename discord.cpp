#include "discord.h"
TextManager textManager(10);

using namespace SleepyDiscord;

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

class MyClientClass : public SleepyDiscord::DiscordClient {
public:
	using SleepyDiscord::DiscordClient::DiscordClient;

	std::vector<Server> SL = getServers().vector();
	time_t uptimet = time(0);
	long long uptime = uptimet;

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

	long long ReadHintTime(std::string serverid, std::string questionname) {
		std::ifstream readFile;
#ifdef _WIN32
		readFile.open("database\\hint\\" + serverid + "-" + questionname + ".txt");
#endif
#ifdef linux
		readFile.open("/home/phoenix/discord-bot/build/database/hint/" + serverid + "-" + questionname + ".txt");
#endif

		if (readFile.is_open()) {
			std::string str;
			while (readFile) {
				getline(readFile, str);
				ReplaceAll(str, "\r", "");
			}
			std::stringstream SS(str);
			long long i;
			SS >> i;
			return i;
		}
		return -1;
	}

	bool WriteHintTime(std::string serverid, std::string questionname, int hintday) {
		std::ofstream ofile;
#ifdef _WIN32
		ofile.open("database\\hint\\" + serverid + "-" + questionname + ".txt");
#endif
#ifdef linux
		ofile.open("/home/phoenix/discord-bot/build/database/hint/" + serverid + "-" + questionname + ".txt");
#endif

		ofile << std::to_string(hintday);
		return true;
	}

	bool WriteQuestion(ibot::Questioninfo Qi) {
		std::ofstream ofile;
#ifdef _WIN32
		ofile.open("database\\questions\\" + Qi.serverid + "-" + hexprint(Qi.name) + ".txt");
#endif
#ifdef linux
		ofile.open("/home/phoenix/discord-bot/build/database/questions/" + Qi.serverid + "-" + hexprint(Qi.name) + ".txt");
#endif
		std::string out;
		out = "{{answer}}\n";
		out += Qi.answer + "\n";
		out += "{{channelid}}\n";
		out += Qi.channelid + "\n";
		out += "{{roleid}}\n";
		out += Qi.roleid + "\n";
		out += "{{hint}}\n";
		out += Qi.hint + "\n";
		out += "{{manswer}}\n";
		for (int i = 0; i < Qi.multipleanswer.size(); i++) {
			out += Qi.multipleanswer.at(i).multipleanswer + "\n";
			out += Qi.multipleanswer.at(i).question + "\n";
		}
		out += "{{permission}}\n";
		out += Qi.permission + "\n";
		ofile << out;
		return true;
	}

	ibot::Questioninfo ReadQuestion(std::string serverid, std::string questionname) {
		std::ifstream readFile;
		printf("ReadQuestion is called.\n");
#ifdef _WIN32
		readFile.open("database\\questions\\" + serverid + "-" + hexprint(questionname) + ".txt");
#endif
#ifdef linux
		printf("linux is detected!\n");
		readFile.open("/home/phoenix/discord-bot/build/database/questions/" + serverid + "-" + hexprint(questionname) + ".txt");
#endif
		ibot::Questioninfo Qi;
		printf("\"%s, %s\"\n", serverid.c_str(), hexprint(questionname).c_str());
		if (readFile.is_open()) {
			printf("readfile is detected.\n");
			Qi.serverid = serverid;
			Qi.name = questionname;
			std::string str;
			std::string it;
			int mit = 0;
			ibot::MultipleAnswer M;
			while (readFile) {
				getline(readFile, str);
				ReplaceAll(str, "\r", "");
				printf("- %s -\n", str.c_str());
				printf("- %s -\n", hexprint(str).c_str());
				if (str == "{{answer}}"
					|| str == "{{channelid}}"
					|| str == "{{roleid}}"
					|| str == "{{hint}}"
					|| str == "{{manswer}}"
					|| str == "{{permission}}") {
					it = str;
				}
				else {
					if (it == "{{answer}}") {
						if (Qi.answer == inv) {
							Qi.answer = str;
						}
						else {
							Qi.answer += str;
						}
					}
					if (it == "{{channelid}}") {
						if (Qi.channelid == inv) {
							Qi.channelid = str;
						}
						else {
							Qi.channelid += str;
						}
					}
					if (it == "{{roleid}}") {
						if (Qi.roleid == inv) {
							Qi.roleid = str;
						}
						else {
							Qi.roleid += str;
						}
					}
					if (it == "{{hint}}") {
						if (Qi.hint == inv) {
							Qi.hint = str;
						}
						else {
							Qi.hint += "\n" + str;
						}
					}
					if (it == "{{manswer}}") {
						if (mit == 0) {
							M.multipleanswer = str;
							mit = 1;
						}
						else {
							M.question = str;
							Qi.multipleanswer.push_back(M);
							mit = 0;
						}
					}
					if (it == "{{permission}}") {
						if (Qi.permission == inv) {
							Qi.permission = str;
							it = "END";
						}
						else {
							Qi.permission += str;
						}
					}
				}
				str == "";
			}
		}
		printf("ReadQuestion returned Qi{name:%s, answer:%s}\n", Qi.name.c_str(), Qi.answer.c_str());
		return Qi;
	}

	bool ChangeQuestion(std::string serverid, 
		std::string questionname, 
		std::string answer, 
		std::string channelid, 
		std::string roleid, 
		std::string hint, 
		std::vector<ibot::MultipleAnswer> manswer,
		std::string permission) {
		ibot::Questioninfo Qi = ReadQuestion(serverid, questionname);
		if (Qi.name == inv)
		if (answer == inv && channelid == inv && roleid == inv && hint == inv && manswer.size() == 0) {
#ifdef _WIN32
			remove(("database\\questions\\" + serverid + "-" + hexprint(questionname) + ".txt").c_str());
#endif
#ifdef linux
			remove(("/home/phoenix/discord-bot/build/database/questions/" + serverid + "-" + hexprint(questionname) + ".txt").c_str());
#endif
		}
		Qi.serverid = serverid;
		Qi.name = questionname;
		if (answer != inv) {
			Qi.answer = answer;
		}
		if (channelid != inv) {
			Qi.channelid = channelid;
		}
		if (roleid != inv) {
			Qi.roleid = roleid;
		}
		if (hint != inv) {
			Qi.hint = hint;
		}
		if (manswer.size() != 0) {
			Qi.multipleanswer = manswer;
		}
		if (permission != inv) {
			Qi.permission = permission;
		}
		WriteQuestion(Qi);
		return true;
	}

	int IsAnswer(std::string serverid, std::string questionname, std::string answer) {
		if (ReadQuestion(serverid, questionname).answer == answer) {
			return 1;
		}
		if (ReadQuestion(serverid, questionname).answer == inv) {
			return -1;
		}
		return 0;
	}

	std::string MultipleAnswerChannel(std::string serverid, std::string questionname, std::string manswer) {
		std::vector<ibot::MultipleAnswer> Ml = ReadQuestion(serverid, questionname).multipleanswer;
		for (int i = 0; i < Ml.size(); i++) {
			if (Ml[i].multipleanswer == manswer) {
				return Ml[i].question;
			}
		}
		return inv;
	}

	bool WriteServerInfo(ibot::Serverinfo Si) {
		std::ofstream ofile;
#ifdef _WIN32
		ofile.open("database\\serverinfo\\" + Si.serverid + ".txt");
#endif
#ifdef linux
		ofile.open("/home/phoenix/discord-bot/build/database/serverinfo/" + Si.serverid + ".txt");
#endif
		std::string out;
		out = Si.release?"1":"0";
		out += "\n" + std::to_string(Si.hinttime) + "\n" + Si.logchannel + "\n";
		ofile << out;
		return true;
	}

	ibot::Serverinfo ReadServerInfo(std::string serverid) {
		std::ifstream readFile;
#ifdef _WIN32
		readFile.open("database\\serverinfo\\" + serverid + ".txt");
#endif
#ifdef linux
		readFile.open("/home/phoenix/discord-bot/build/database/serverinfo/" + serverid + ".txt");
#endif
		ibot::Serverinfo Si;
		if (readFile.is_open()) {
			Si.serverid = serverid;
			std::string str;
			int i = 0;
			int in;
			while (readFile) {
				getline(readFile, str);
				ReplaceAll(str, "\r", "");
				std::stringstream SS(str);
				switch (i) {
					case 0:
						SS >> in;
						Si.release = in;
						break;
					case 1:
						SS >> Si.hinttime;
						break;
					case 2:
						Si.logchannel = str;
						break;
				}
				i++;
				if (i == 3) {
					break;
				}
			}
		}
		return Si;
	}

	bool ChangeServerInfo(std::string serverid, int release, int hinttime, std::string logchannel) {
		ibot::Serverinfo Si = ReadServerInfo(serverid);
		Si.serverid = serverid;
		if (release != -1) {
			Si.release = release;
		}
		if (hinttime != -1) {
			Si.hinttime = hinttime;
		}
		if (logchannel != inv) {
			Si.logchannel = logchannel;
		}
		WriteServerInfo(Si);
		return true;
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

	std::string invt(std::string S) {
		if (S == inv) {
			std::string ret = GetTextA("isinvalid");
			ret.substr(0, ret.size()-1);
			return ret;
		}
		return S;
	}

	void onMessage(SleepyDiscord::Message message) override {
		if (message.content == "!!hellothisisverification") {
			sendMessage(message.channelID, GetTextA("hellothisisverification"));
			return;
		}
		Server server;

		if (message.serverID.empty()) {
			if (message.startsWith(".91updown ")) {
				std::string Content = message.content.substr(10);
				int type = 1; //1:N 2:Q/ 3:Q. 4:inv
				int updown = 0; //1:UP 2:DOWN 3:COR 0:inv
				std::string fr = "";
				std::string ls = "";
				for (int i = 0; i < Content.size(); i++) {
					if (!('0' <= Content[i] && Content[i] <= '9')) {
						if (type != 1) {
							type = 4;
						}
						else if (Content[i] == '/') {
							type = 2;
						}
						else if (Content[i] == '.') {
							type = 3;
						}
						else {
							type = 4;
						}
					}
					else {
						if (type == 1) {
							fr += Content[i];
						}
						else {
							ls += Content[i];
						}
					}
				}
				if ((fr == "" || ls == "") && type != 1) {
					type = 4;
				}
				std::stringstream SS(Content);
				int i;
				SS >> i;
				std::stringstream FS(fr);
				int f;
				FS >> f;
				std::stringstream LS(ls);
				int l;
				LS >> l;
				switch (type) {
				case 1:
					if (i <= 4) {
						updown = 1;
					}
					else {
						updown = 2;
					}
					break;
				case 2:
					if (f * 7 > l * 31) {
						updown = 2;
					}
					if (f * 7 < l * 31) {
						updown = 1;
					}
					if (f * 7 == l * 31) {
						updown = 3;
					}
					break;
				case 3:
					if (f > 4) {
						updown = 2;
					}
					if (f < 4) {
						updown = 1;
					}
					std::string c = "428571";
					if (f == 4) {
						for (int i = 0; i < ls.size(); i++) {
							if (ls[i] < c[i % 6]) {
								updown = 1;
								break;
							}
							if (ls[i] > c[i % 6]) {
								updown = 2;
								break;
							}
						}
						if (updown == 0) {
							updown = 1;
						}
					}
				}
				switch (updown) {
				case 0:
					sendMessage(message.channelID, GetTextA("updown-inv"));
					break;
				case 1:
					sendMessage(message.channelID, GetTextA("updown-up"));
					break;
				case 2:
					sendMessage(message.channelID, GetTextA("updown-dw"));
					break;
				case 3:
					sendMessage(message.channelID, GetTextA("updown-cr"));
				}
			}
			if (message.startsWith(".14updown ")) {
				std::string r2 = "4142135623730950488016887242096980785696718753769480731766797379907324784621070388503875343276415727350138462309122970249248360558507372126441214970999358314132226659275055927557999505011527820605714701095599716059702745345968620147285174186408891986095523292304843087143214508397626036279952514079896872533965463318088296406206152583523950547457502877599617298355752203375318570113543746034084988471603868999706990048150305440277903164542478230684929369186215805784631115966687130130156185689872372352885092648612494977154218334204285686060146824720771435854874155657069677653720226485447015858801620758474922657226002085584466521458398893944370926591800311388246468157082630100594858704003186480342194897278290641045072636881313739855256117322040245091227700226941127573627280495738108967504018369868368450725799364729060762996941380475654823728997180326802474420629269124859052181004459842150591120249441341728531478105803603371077309182869314710171111683916581726889419758716582152128229518488472089694633862891562882765952635140542267653239694617511291602408715510135150455381287560052631468017127402653969470240300517495318862925631385188163478001569369176881852378684052287837629389214300655869568685964595155501644724509836896036887323114389415576651040883914292338113";
				std::string Content = message.content.substr(10);
				int type = 1; //1:N 2:Q/ 3:Q. 4:inv
				int updown = 0; //1:UP 2:DOWN 3:COR 0:inv
				std::string fr = "";
				std::string ls = "";
				for (int i = 0; i < Content.size(); i++) {
					if (!('0' <= Content[i] && Content[i] <= '9')) {
						if (type != 1) {
							type = 4;
						}
						else if (Content[i] == '/') {
							type = 2;
						}
						else if (Content[i] == '.') {
							type = 3;
						}
						else {
							type = 4;
						}
					}
					else {
						if (type == 1) {
							fr += Content[i];
						}
						else {
							ls += Content[i];
						}
					}
				}
				if ((fr == "" || ls == "") && type != 1) {
					type = 4;
				}
				std::stringstream SS(Content);
				int i;
				SS >> i;
				std::stringstream FS(fr);
				int f;
				FS >> f;
				std::stringstream LS(ls);
				int l;
				LS >> l;
				switch (type) {
				case 1:
					if (i <= 1) {
						updown = 1;
					}
					else {
						updown = 2;
					}
					break;
				case 2:
					for (int i = 0; i < 1000; i++) {
						int a = f / l;
						f = f - a*l;
						int c;
						if (i == 0) {
							c = 1;
						}
						else {
							c = r2[i - 1] - '0';
						}
						if (c > a) {
							updown = 1;
							break;
						}
						if (c < a) {
							updown = 2;
							break;
						}
						f = f * 10;
					}
					break;
				case 3:
					if (f > 1) {
						updown = 2;
					}
					if (f < 1) {
						updown = 1;
					}
					if (f == 1) {
						for (int i = 0; i < ls.size(); i++) {
							if (ls[i] < r2[i]) {
								updown = 1;
								break;
							}
							if (ls[i] > r2[i]) {
								updown = 2;
								break;
							}
						}
						if (updown == 0) {
							updown = 1;
						}
					}
				}

				if (Content + "\n" == GetTextA("updown-r21") || Content + "\n" == GetTextA("updown-r22") || Content + "\n" == GetTextA("updown-r23") || Content + "\n" == GetTextA("updown-r24") || Content + "\n" == GetTextA("updown-r25")) {
					updown = 3;
				}

				switch (updown) {
				case 0:
					sendMessage(message.channelID, GetTextA("updown-inv"));
					break;
				case 1:
					sendMessage(message.channelID, GetTextA("updown-up"));
					break;
				case 2:
					sendMessage(message.channelID, GetTextA("updown-dw"));
					break;
				case 3:
					sendMessage(message.channelID, GetTextA("updown-cr2"));
				}
			}
		}
		else {
			server = getServer(message.serverID);
		}
		if (server.ownerID == message.author.ID || message.author.ID == Q_ID) {
			if (message.startsWith("!!")) {
				time_t now = time(0);
				long long T = now;
				if (message.author.ID != 640341085297704970) {
					sendMessage("883621405797781534", GetTextA("log-master", std::to_string(T).c_str(), message.author.ID.string().c_str(), ToText(server.name).c_str(), ToText(message.content).c_str()));
					if (!message.startsWith("!!logChannel") && ReadServerInfo(message.serverID).logchannel == inv) {
						sendDM(message.author.ID, GetTextA("logchannelrequired"));
					}
				}
			}
			if (message.startsWith("!!setHintTime")) {
				if (message.content.size() < 14) {
					sendDM(message.author.ID, GetTextA("failhntt"));
					return;
				}
				std::string Content = message.content.substr(14);
				std::stringstream SS(Content);
				int i;
				SS >> i;
				ChangeServerInfo(message.serverID, -1, i, inv);
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!verifyMessage")) {
				if (message.content.size() < 16) {
					sendDM(message.author.ID, GetTextA("failverify"));
					return;
				}
				std::string Role = message.content.substr(16);

				if (Role.size() == 22) {
					std::string RoleID = Role.substr(3, 18);
					SendMessageParams P;
					P.content = GetTextA("verification_role", Role.c_str());
					P.channelID = message.channelID;
					auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
					auto button = std::make_shared<SleepyDiscord::Button>();
					button->style = SleepyDiscord::ButtonStyle::Danger;
					button->label = GetTextA("verification_button");
					button->customID = "giverole_" + RoleID;
					actionRow->components.push_back(button);
					P.components.push_back(actionRow);
					sendMessage(P);
					deleteMessage(message.channelID, message);
				}
				else {
					sendDM(message.author.ID, GetTextA("failverify"));
				}
			}
			else if (message.startsWith("!!say")) {
				if (message.content.size() < 6) {
					sendDM(message.author.ID, GetTextA("failsay"));
					return;
				}
				std::string Content = message.content.substr(6);
				sendMessage(message.channelID, Content);
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!setQuestion")) {
				if (message.content.size() < 14) {
					sendDM(message.author.ID, GetTextA("failsetq"));
					return;
				}
				std::string Content = message.content.substr(14);
				std::string Q = "";
				std::string A = "";
				int a = 0;
				for (int i = 0; i < Content.size(); i++) {
					if (Content[i] == ' ') {
						a++;
						continue;
					}
					if (a > 1) {
						sendDM(message.author.ID, GetTextA("failsetq"));
						break;
					}
					if (a == 0) {
						Q += Content[i];
					}
					if (a == 1) {
						A += Content[i];
					}
				}
				if (Q == "" || A == "") {
					sendDM(message.author.ID, GetTextA("failsetq"));
				}
				ChangeQuestion(message.serverID, Q, A, inv, inv, inv, ibot::mnl(), inv);
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!setChannel")) {
				if (message.content.size() < 13) {
					sendDM(message.author.ID, GetTextA("failsetc"));
					return;
				}
				std::string Content = message.content.substr(13);
				std::string Q = "";
				std::string A = "";
				int a = 0;
				for (int i = 0; i < Content.size(); i++) {
					if (Content[i] == ' ') {
						a++;
						continue;
					}
					if (a > 1) {
						sendDM(message.author.ID, GetTextA("failsetc"));
						break;
					}
					if (a == 0) {
						Q += Content[i];
					}
					if (a == 1) {
						A += Content[i];
					}
				}
				if (Q == "" || A == "") {
					sendDM(message.author.ID, GetTextA("failsetc"));
				}
				if (A.size() == 21) {
					A = A.substr(2, 18);
					ChangeQuestion(message.serverID, Q, inv, A, inv, inv, ibot::mnl(), inv);
				}
				else {
					sendDM(message.author.ID, GetTextA("failsetc"));
				}
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!setRole")) {
				if (message.content.size() < 10) {
					sendDM(message.author.ID, GetTextA("failsetr"));
					return;
				}
				std::string Content = message.content.substr(10);
				std::string Q = "";
				std::string A = "";
				int a = 0;
				for (int i = 0; i < Content.size(); i++) {
					if (Content[i] == ' ') {
						a++;
						continue;
					}
					if (a > 1) {
						sendDM(message.author.ID, GetTextA("failsetr"));
						break;
					}
					if (a == 0) {
						Q += Content[i];
					}
					if (a == 1) {
						A += Content[i];
					}
				}
				if (Q == "" || A == "") {
					sendDM(message.author.ID, GetTextA("failsetr"));
				}
				if (A.size() == 22) {
					A = A.substr(3, 18);
					ChangeQuestion(message.serverID, Q, inv, inv, A, inv, ibot::mnl(), inv);
				}
				else {
					sendDM(message.author.ID, GetTextA("failsetr"));
				}
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!setHint")) {
				if (message.content.size() < 10) {
					sendDM(message.author.ID, GetTextA("failseth"));
					return;
				}
				std::string Content = message.content.substr(10);
				std::string Q = "";
				std::string A = "";
				int a = 0;
				for (int i = 0; i < Content.size(); i++) {
					if (Content[i] == ' ' && a == 0) {
						a++;
						continue;
					}
					if (a == 0) {
						Q += Content[i];
					}
					if (a == 1) {
						A += Content[i];
					}
				}
				if (Q == "" || A == "") {
					sendDM(message.author.ID, GetTextA("failseth"));
				}
				ChangeQuestion(message.serverID, Q, inv, inv, inv, A, ibot::mnl(), inv);
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!deleteQuestion")) {
				if (message.content.size() < 17) {
					sendDM(message.author.ID, GetTextA("faildelq"));
					return;
				}
				std::string Content = message.content.substr(17);
				if (ReadQuestion(message.serverID, Content).name != inv) {
					ChangeQuestion(message.serverID, Content, inv, inv, inv, inv, ibot::mnl(), inv);
				}
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!logChannel")) {
				ChangeServerInfo(message.serverID, -1, -1, message.channelID);
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!setMultipleAnswer")) {
				if (message.content.size() < 20) {
					sendDM(message.author.ID, GetTextA("failsmpa"));
					return;
				}
				std::string Content = message.content.substr(20);
				std::string Q = "";
				std::string A = "";
				std::string Q2 = "";
				int a = 0;
				for (int i = 0; i < Content.size(); i++) {
					if (Content[i] == ' ') {
						a++;
						continue;
					}
					if (a > 2) {
						sendDM(message.author.ID, GetTextA("failsmpa"));
						break;
					}
					if (a == 0) {
						Q += Content[i];
					}
					if (a == 1) {
						A += Content[i];
					}
					if (a == 2) {
						Q2 += Content[i];
					}
				}
				if (Q == "" || A == "" || Q2 == "") {
					sendDM(message.author.ID, GetTextA("failsmpa"));
				}
				ibot::MultipleAnswer ma;
				ma.multipleanswer = A;
				ma.question = Q2;
				std::vector<ibot::MultipleAnswer> Ml = ReadQuestion(message.serverID, Q).multipleanswer;
				Ml.push_back(ma);
				ChangeQuestion(message.serverID, Q, inv, inv, inv, inv, Ml, inv);
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!releaseServer")) {
				SendMessageParams SP;
				SP.channelID = message.channelID;
				SP.content = GetTextA("releasecheck");

				auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
				auto button = std::make_shared<SleepyDiscord::Button>();
				button->disabled = false;
				button->style = SleepyDiscord::ButtonStyle::Danger;
				button->label = GetTextA("yes");
				button->customID = "releaseserver";
				actionRow->components.push_back(button);

				SP.components.push_back(actionRow);
				sendMessage(SP);
			}
			else if (message.startsWith("!!hideServer")) {
				SendMessageParams SP;
				SP.channelID = message.channelID;
				SP.content = GetTextA("hidecheck");

				auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
				auto button = std::make_shared<SleepyDiscord::Button>();
				button->disabled = false;
				button->style = SleepyDiscord::ButtonStyle::Success;
				button->label = GetTextA("yes");
				button->customID = "hideserver";
				actionRow->components.push_back(button);

				SP.components.push_back(actionRow);
				sendMessage(SP);
			}
			else if (message.startsWith("!!setPermission")) {
				if (message.content.size() < 16) {
					sendDM(message.author.ID, GetTextA("failsetp"));
					return;
				}
				std::string Content = message.content.substr(16);
				std::string Q = "";
				std::string A = "";
				int a = 0;
				for (int i = 0; i < Content.size(); i++) {
					if (Content[i] == ' ') {
						a++;
						continue;
					}
					if (a > 1) {
						sendDM(message.author.ID, GetTextA("failsetp"));
						break;
					}
					if (a == 0) {
						Q += Content[i];
					}
					if (a == 1) {
						A += Content[i];
					}
				}
				if (Q == "" || A == "") {
					sendDM(message.author.ID, GetTextA("failsetp"));
				}
				if (A.size() == 22) {
					A = A.substr(3, 18);
					ChangeQuestion(message.serverID, Q, inv, inv, inv, inv, ibot::mnl(), A);
				}
				else {
					sendDM(message.author.ID, GetTextA("failsetp"));
				}
				deleteMessage(message.channelID, message);
			}
			else if (message.startsWith("!!questionList")) {
				ibot::Serverinfo Si;
				Si.serverid = message.serverID;
				std::string S;
				std::vector<std::string> V = Si.QuestionList();
				for (int i = 0; i < V.size(); i++) {
					V[i] = hexdeprint(V[i]);
					S += V[i] + "\n";
				}
				sendMessage(message.channelID, GetTextA("questionlist", S.c_str()));
			}
			else if (message.startsWith("!!questionInfo")) {
				if (message.content.size() < 15) {
					sendDM(message.author.ID, GetTextA("failquel"));
					return;
				}
				std::string Content = message.content.substr(15);
				ibot::Questioninfo Qi;
				Qi = ReadQuestion(message.serverID, Content);
				std::string R;
				if (Qi.multipleanswer.size() == 0) {
					R = GetTextA("manswer-non");
					R.substr(0, R.size() - 1);
				}
				else {
					for (int i = 0; i < Qi.multipleanswer.size(); i++) {
						R += GetTextA("questioninfo-manswer", Qi.multipleanswer[i].multipleanswer.c_str(), Qi.multipleanswer[i].question.c_str());
						R.substr(0, R.size() - 1);
					}
				}
				std::string S;
				if (Qi.permission == inv) {
					S = GetTextA("questioninfo-ev");
					R.substr(0, R.size() - 1);
				}
				else {
					S = GetTextA("questioninfo-pr", Qi.permission.c_str());
					R.substr(0, R.size() - 1);
				}
				R = GetTextA("questioninfo", invt(Qi.name).c_str(), invt(Qi.answer).c_str(), R.c_str(), invt(Qi.hint).c_str(), invt(Qi.channelid==inv?Qi.channelid:"`<#"+Qi.channelid+">`").c_str(), invt(Qi.roleid==inv?Qi.roleid:"`<@&"+Qi.roleid+">`").c_str(), S.c_str());
				printf("%d", R.size());
				printf("%s", R.c_str());
				R = "`" + R + "`";
				sendMessage(message.channelID, R);
			}
		}
		if (message.author.ID == Q_ID) {
			if (message.startsWith("\\enableallchannel")) {
				ServerMember M = getMember(message.serverID, message.author);
				std::vector<Channel> C = getServerChannels(message.serverID);
				for (int i = 0; i < C.size(); i++) {
					Overwrite O;
					O.ID = M.ID.number();
					O.allow = Permission::VIEW_CHANNEL;
					O.deny = Permission::NONE;
					editChannelPermissions(C.at(i), O, Permission::ALL, Permission::NONE, "1");
				}
			}
			/*else if (message.startsWith("\\invitelinks")) {
				std::string InviteLinks;
				for (int i = 0; i < SL.size(); i++) {
					std::vector<Channel> C = getServerChannels(SL.at(i)).vector();
					ServerMember bot = getMember(SL.at(i).ID, "861865296037543966");
					if (C.size() == 0) {
						sendMessage(883621405797781534, SL.at(i).ID.string());
						continue;
					}
					if (!hasPremission(bot.permissions, Permission::ADMINISTRATOR)) {
						sendMessage(C.at(0).ID, "MISSING PERMISSION.");
						leaveServer(SL.at(i).ID);
						continue;
					}
					if (ReadServerInfo(SL.at(i).ID).logchannel != "-1") {
						Invite I = createChannelInvite(ReadServerInfo(SL.at(i).ID).logchannel, 300, 1, false, false);
						Sleep(5000);
						std::string str = I.code;
						InviteLinks += SL.at(i).name + " : ";
						InviteLinks += "https://discord.gg/" + str;
						InviteLinks += "\n";
					}
				}
				sendMessage(message.channelID, InviteLinks);
			}*/

			else if (message.startsWith("\\teha")) {
				int l;
				Channel C;
				C = getChannel(message.channelID);
				if (message.content.size() == 5) {
					l = 100;
				}
				else {
					std::string S = message.content.substr(5);
					std::stringstream SS(S);
					SS >> l;
				}
				if (l == 1) {
					l = 2;
				}
				std::vector<Message> V = getMessages(C, GetMessagesKey::before, message.ID, l).vector();
				std::vector<Snowflake<Message>> SV;
				for (int i = 0; i < V.size(); i++) {
					SV.push_back(V[i].ID);
				}
				bulkDeleteMessages(C.ID, SV);
			}

			else if (message.startsWith("\\encode ") && message.content != "\\encode ") {
				std::string Content = message.content.substr(8);
				sendMessage(message.channelID, hexprint(Content));
			}
			
			else if (message.startsWith("\\latex ") && message.content != "\\latex ") {
				std::string Content = message.content.substr(7);
				ReplaceAll(Content, " ", "%20");
				ReplaceAll(Content, "\n", "%20\\\\%20");
				Content = "https://latex.codecogs.com/gif.latex?%5Cdpi%7B300%7D%20%5Cbg_black%20" + Content;
				sendMessage(message.channelID, Content);
			}
		}
	}

	void onReaction(
		Snowflake< User > userID,
		Snowflake< Channel > channelID,
		Snowflake< Message > messageID,
		Emoji emoji
	) override {
		if (messageID == 882186796329472000) {
			std::string E = emoji.name;
			printf("%s\n", hexprint(E).c_str());
			deleteAllReactions(880989430989594634, 882186796329472000);
			if (hexprint(E) + "\n" == GetTextA("imaginary-icon")) {
				addRole(880642251611574324, userID, 888353753500094494);
			}
		}
	}
	
	void UpdSt1() {
		updateStatus(GetTextA("StatusGame1"), 0, online);
		schedule([this]() {this->UpdSt2(); }, 5000);
	}

	void UpdSt2() {
		updateStatus(GetTextA("StatusGame2"), 0, doNotDisturb);
		schedule([this]() {this->UpdSt3(); }, 5000);
	}

	void UpdSt3() {
		updateStatus(GetTextA("StatusGame3", std::to_string(SL.size()).c_str()), 0, online);
		schedule([this]() {this->UpdSt4(); }, 5000);
	}

	void UpdSt4() {
		int x = 0;
		for (int i = 0; i < SL.size(); i++) {
			if (ReadServerInfo(SL.at(i).ID).release) {
				x++;
			}
		}
		updateStatus(GetTextA("StatusGame4", std::to_string(x).c_str()), 0, doNotDisturb);
		schedule([this]() {this->UpdSt1(); }, 5000);
	}

	Embed ServerEmbed(Server S) {
		Embed E;
		EmbedField EF;
		E.title = GetTextA("goto-server", S.name.c_str());
		EF.name = GetTextA("server-name");
		EF.value = S.name;
		EF.isInline = true;
		E.fields.push_back(EF);
		EF.name = GetTextA("server-owner");
		ServerMember M = getMember(S, S.ownerID);
		EF.value = M.nick == "" ? M.user.username : M.nick;
		EF.isInline = true;
		E.fields.push_back(EF);
		EF.name = GetTextA("server-question");
		EF.value = std::to_string(ReadServerInfo(S.ID).QuestionList().size());
		EF.isInline = true;
		E.fields.push_back(EF);
		//EF.name = GetTextA("server-member");
		//EF.value = std::to_string(S.memberCount);
		//EF.isInline = true;
		//E.fields.push_back(EF);
		EF.name = GetTextA("server-des");
		EF.value = S.description != "" ? S.description : GetTextA("server-nodes");
		EF.isInline = true;
		E.fields.push_back(EF);
		E.thumbnail.url = "https://cdn.discordapp.com/icons/" + S.ID + "/" + S.icon + ".png";
		std::string InviteLinks;
		Invite I = createChannelInvite(ReadServerInfo(S.ID).logchannel, 600, 100, false, false);
		E.url = "https://discord.gg/" + I.code;
		return E;
	}

	void onReady(Ready readyData) override {
		UpdSt1();
		std::array<AppCommand::Option, 1> option;
		std::array<AppCommand::Option, 2> option2;
		std::array<AppCommand::Option, 3> option3;
		AppCommand::Option::Choice choice;

		/*option2.at(0).type = AppCommand::Option::Type::STRING;
		option2.at(0).name = "question-name";
		option2.at(0).description = GetTextA("answer-question-name");
		option2.at(0).isRequired = true;
		option2.at(1).type = AppCommand::Option::Type::STRING;
		option2.at(1).name = "answer";
		option2.at(1).description = GetTextA("answer-answer");
		option2.at(1).isRequired = true;
		createGlobalAppCommand(AppID, "answer", GetTextA("answer"), option2);

		createGlobalAppCommand(AppID, "help", GetTextA("help"));

		option.at(0).type = AppCommand::Option::Type::STRING;
		option.at(0).name = "question-name";
		option.at(0).description = GetTextA("answer-question-name");
		option.at(0).isRequired = true;
		createGlobalAppCommand(AppID, "hint", GetTextA("hint"), option);*/

		//createGlobalAppCommand(AppID, "servers", GetTextA("servers"));
	}

	void onInteraction(Interaction interaction) override {
		std::srand(static_cast<int>(std::time(0)));
		SleepyDiscord::Interaction::Response response;
		SleepyDiscord::WebHookParams followUp;
		std::string customid = interaction.data.customID;

		if (interaction.type == Interaction::Type::MessageComponent) {
			if (customid.find("giverole_") == 0) {
				customid = customid.substr(9);
				addRole(interaction.serverID, interaction.member.ID, customid);
				response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
				createInteractionResponse(interaction, interaction.token, response);
			}
			if (customid == "hideserver") {
				Server S = getServer(interaction.serverID);
				if (interaction.member.ID == S.ownerID) {
					ChangeServerInfo(interaction.serverID, 0, -1, inv);
					response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
					createInteractionResponse(interaction, interaction.token, response);
				}
				else {
					response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
					response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
					response.data.content = GetTextA("not-owner");
					createInteractionResponse(interaction, interaction.token, response);
				}
			}
			if (customid == "releaseserver") {
				Server S = getServer(interaction.serverID);
				if (interaction.member.ID == S.ownerID) {
					ChangeServerInfo(interaction.serverID, 1, -1, inv);
					response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
					createInteractionResponse(interaction, interaction.token, response);
				}
				else {
					response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
					response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
					response.data.content = GetTextA("not-owner");
					createInteractionResponse(interaction, interaction.token, response);
				}
			}
			if (customid == "serversselect") {
				std::string SID = interaction.data.values.front();
				EditMessageParams E;
				auto S = std::make_shared<SelectMenu>();
				SelectMenu::Option O;
				auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
				for (int i = 0; i < SL.size(); i++) {
					if (ReadServerInfo(SL.at(i).ID).release) {
						O.value = SL.at(i).ID.string();
						O.label = SL.at(i).name;
						O.description = " ";
						S->options.push_back(O);
					}
				}
				S->customID = "serversselect";
				actionRow->components.push_back(S);
				E.components.push_back(actionRow);
				E.embed = ServerEmbed(getServer(SID));
				E.channelID = interaction.channelID;
				E.messageID = interaction.message;
				E.content = " ";
				editMessage(std::move(E));
				response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
				createInteractionResponse(interaction, interaction.token, response);
			}
		}

		else if (interaction.data.name == "answer") {
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			std::string S = interaction.serverID;
			std::string Q = interaction.data.options.at(0).value.GetString();
			std::string A = interaction.data.options.at(1).value.GetString();
			bool per = false;
			if (ReadQuestion(interaction.serverID, Q).permission == inv) {
				per = true;
			}
			else {
				for (int i = 0; i < interaction.member.roles.size(); i++) {
					if (interaction.member.roles.at(i).string() == ReadQuestion(interaction.serverID, Q).permission) {
						per = true;
					}
					printf("%s VS %s", interaction.member.roles.at(i).string().c_str(), ReadQuestion(interaction.serverID, Q).permission.c_str());
				}
			}
			if (per) {
				switch (IsAnswer(S, Q, A)) {
				case -1:
					if (ReadServerInfo(interaction.serverID).logchannel != inv) {
						time_t now = time(0);
						long long T = now;
						std::string I = interaction.member.ID;
						sendMessage(ReadServerInfo(interaction.serverID).logchannel, GetTextA("log-answer", std::to_string(T).c_str(), I.c_str(), ToText(Q).c_str(), ToText(A).c_str()));
					}
					response.data.content = GetTextA("answer_invalid", Q.c_str());
					break;
				case 0:
					response.data.content = GetTextA("answer_wrong", Q.c_str());
					if (MultipleAnswerChannel(interaction.serverID, Q, A) == inv) {
						if (ReadServerInfo(interaction.serverID).logchannel != inv) {
							time_t now = time(0);
							long long T = now;
							std::string I = interaction.member.ID;
							sendMessage(ReadServerInfo(interaction.serverID).logchannel, GetTextA("log-answer-wrong", std::to_string(T).c_str(), I.c_str(), ToText(Q).c_str(), ToText(A).c_str()));
						}
						break;
					}
					else {
						Q = MultipleAnswerChannel(interaction.serverID, Q, A);
					}
				case 1:
					if (ReadServerInfo(interaction.serverID).logchannel != inv) {
						time_t now = time(0);
						long long T = now;
						std::string I = interaction.member.ID;
						sendMessage(ReadServerInfo(interaction.serverID).logchannel, GetTextA("log-answer-right", std::to_string(T).c_str(), I.c_str(), ToText(Q).c_str(), ToText(A).c_str()));
					}
					response.data.content = GetTextA("answer_right", Q.c_str());
					/*--------------------------------*/
					if (ReadQuestion(interaction.serverID, Q).channelid == "KICKTHISUSER") {
						sendDM(interaction.member.ID, GetTextA("imaginary-start"));
						sendDM(interaction.member.ID, GetTextA("imaginary-start-link"));
						kickMember(interaction.serverID, interaction.member.ID);
						return;
					}
					/*--------------------------------*/
					if (ReadQuestion(interaction.serverID, Q).channelid != inv) {
						response.data.content += GetTextA("answer_con_c", ReadQuestion(interaction.serverID, Q).channelid.c_str());
						Channel C = getChannel(ReadQuestion(interaction.serverID, Q).channelid);
						Overwrite O;
						O.ID = interaction.member.ID.number();
						O.allow = Permission::VIEW_CHANNEL;
						O.deny = Permission::NONE;
						editChannelPermissions(C, O, Permission::VIEW_CHANNEL, Permission::NONE, "1");
					}
					if (ReadQuestion(interaction.serverID, Q).roleid != inv) {
						response.data.content += GetTextA("answer_con_r", ReadQuestion(interaction.serverID, Q).roleid.c_str());
						addRole(interaction.serverID, interaction.member.ID, ReadQuestion(interaction.serverID, Q).roleid);

					}
					break;
				}
			}
			else {
				if (ReadServerInfo(interaction.serverID).logchannel != inv) {
					time_t now = time(0);
					long long T = now;
					std::string I = interaction.member.ID;
					sendMessage(ReadServerInfo(interaction.serverID).logchannel, GetTextA("log-answer", std::to_string(T).c_str(), I.c_str(), ToText(Q).c_str(), ToText(A).c_str()));
				}
				response.data.content = GetTextA("answer_invalid", Q.c_str());
			}
			Channel C = getChannel(interaction.channelID);
			if (C.type == Channel::DM) {
				response.data.content = GetTextA("DM_X");
			}
			createInteractionResponse(interaction, interaction.token, response);
		}

		else if (interaction.data.name == "help") {
			response.data.content = GetTextA("botcommand");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
		}

		else if (interaction.data.name == "hint") {
			std::string Q = interaction.data.options.at(0).value.GetString();
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			if (ReadQuestion(interaction.serverID, Q).hint != inv) {
				response.data.content = GetTextA("hint-content");

				time_t now = time(0);
				long long i = now;
				
				if (i - ReadHintTime(interaction.serverID, interaction.member.ID) < ReadServerInfo(interaction.serverID).hinttime * 60) {
					response.data.content = GetTextA("hint-houlim", 
						std::to_string((ReadServerInfo(interaction.serverID).hinttime
							- (i - ReadHintTime(interaction.serverID, interaction.member.ID)) / 60)).c_str());
				}
				else {
					WriteHintTime(interaction.serverID, interaction.member.ID, i);
					Channel C = createDirectMessageChannel(interaction.member.ID);
					sendMessage(C, ReadQuestion(interaction.serverID, Q).hint);
					if (ReadServerInfo(interaction.serverID).logchannel != inv) {
						time_t now = time(0);
						long long T = now;
						std::string I = interaction.member.ID;
						sendMessage(ReadServerInfo(interaction.serverID).logchannel,
							GetTextA("log-hint", std::to_string(T).c_str(), I.c_str(), ToText(Q).c_str()));
					}
				}
			}
			else {
				response.data.content = GetTextA("hint-invalid");
			}
			Channel C = getChannel(interaction.channelID);
			if (C.type == Channel::DM) {
				response.data.content = GetTextA("DM_X");
			}
			createInteractionResponse(interaction, interaction.token, response);
		}

		else if (interaction.data.name == "servers") {
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.content = GetTextA("servers-content");
			auto S = std::make_shared<SelectMenu>();
			SelectMenu::Option O;
			auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
			for (int i = 0; i < SL.size(); i++) {
				if (ReadServerInfo(SL.at(i).ID).release) {
					O.value = SL.at(i).ID.string();
					O.label = SL.at(i).name;
					O.description = " ";
					S->options.push_back(O);
				}
			}
			S->customID = "serversselect";
			actionRow->components.push_back(S);
			response.data.components.push_back(actionRow);
			createInteractionResponse(interaction, interaction.token, response);
		}
		
		else {
			deleteGlobalAppCommand(AppID, interaction.data.ID);
		}
	}

	void onServer(Server server) override {
		SL = getServers().vector();
	}

	void OnQuit() {
		SL = getServers().vector();
	}
};

int main()
{
	printf("/i-bot 1.0.ver execute\n");
	textManager.Initialize("default.txt");
	MyClientClass client(Token, SleepyDiscord::USER_CONTROLED_THREADS);
	client.setIntents(Intent::SERVER_MESSAGES + Intent::DIRECT_MESSAGES + Intent::SERVER_MESSAGE_REACTIONS);
	client.run();
}

// /I-bot server script

// Made by Q_
// Qㅡ#0283
