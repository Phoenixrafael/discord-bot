#include "QasinoHeader.h"
TextManager textManager(10);
QasinoBot* client;
using namespace SleepyDiscord;
using namespace qasino;

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

std::string replaceAll(std::string& str, std::string from, std::string to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
	return str;
}

std::string toText(std::string inp) {
	replaceAll(inp, std::string("`"), std::string("'"));
	inp = "`" + inp + "`";
	return inp;
}

bool SoloGame::OnStart(Interaction interaction) {
	_channel = client->getChannel(interaction.channelID);
	_player = client->readQamblerInfo(interaction.member.ID);
	_playing = true;
	_player.SetInt(qasino::SYS_IS_PLAYING, 1);
	_gameID = interaction.ID;
	_player.info[qasino::SYS_GAMEID] = _gameID;
	_betting = interaction.data.options.at(1).value.GetInt();
	_displayname = client->GetTextL(("solo-" + _name).c_str());

	if (_betting < _leastbet) {
		SleepyDiscord::Interaction::Response response;
		response.data.content = GetTextA("solo-game-betting-error");
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
		client->createInteractionResponse(interaction, interaction.token, response);
		Clear();
		return false;
	}
	if (_player.GetInt(qasino::SYS_GAME_CHIP) < _betting) {
		SleepyDiscord::Interaction::Response response;
		response.data.content = GetTextA("solo-game-chip-error");
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
		client->createInteractionResponse(interaction, interaction.token, response);
		Clear();
		return false;
	}

	SleepyDiscord::Interaction::Response response;
	response.data.content = GetTextA("solo-game-start", _displayname.c_str(), std::to_string(_betting).c_str());
	response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
	client->createInteractionResponse(interaction, interaction.token, response);
	client->writeQamblerInfo(_player);

	return true;
}

bool SoloGame::EndGame(int result, int money) {
	SendMessageParams Sp;
	Sp.content = GetTextA("solo-game-result", _displayname.c_str());
	Sp.channelID = _channel;
	Embed E;
	E.title = client->GetTextL("solo-game-result-title", _displayname.c_str());
	EmbedField BetMoney;
	EmbedField GetMoney;
	EmbedField Total;
	BetMoney.name = client->GetTextL("solo-game-result-bet");
	GetMoney.name = client->GetTextL("solo-game-result-get");
	Total.name = client->GetTextL("solo-game-result-tot");
	BetMoney.value = std::to_string(_betting);
	if (result == true) {
		E.description = client->GetTextL("solo-game-result-win", _player.nick.c_str());
		GetMoney.value = std::to_string(money);
		Total.value = std::to_string(money - _betting);
		money -= _betting;
		_player.ChangeInt(qasino::SYS_GAME_CHIP, money);
		client->writeQamblerInfo(_player);
	}
	else if (result == false) {
		_player.ChangeInt(qasino::SYS_GAME_CHIP, -1 * _betting);
		client->writeQamblerInfo(_player);
		E.description = client->GetTextL("solo-game-result-lose", _player.nick.c_str());
		GetMoney.value = std::to_string(0);
		Total.value = std::to_string(0 - _betting);
	}
	else {
		E.description = client->GetTextL("solo-game-result-tie");
		GetMoney.value = std::to_string(_betting);
		Total.value = std::to_string(0);
	}
	BetMoney.isInline = true;
	GetMoney.isInline = true;
	Total.isInline = true;
	E.fields.push_back(BetMoney);
	E.fields.push_back(GetMoney);
	E.fields.push_back(Total);
	Sp.embed = E;
	client->sendMessage(Sp);
	Clear();
	return true;
}

bool SoloGame::Process(Interaction interaction, bool start = false) {
	return true;
}

bool SoloGame::Clear() {
	_playing = false;
	_player.SetInt(qasino::SYS_IS_PLAYING, 0);
	_player.info[qasino::SYS_GAMEID] = "-1";
	client->writeQamblerInfo(_player);
	_gameID = -1;
	return false;
}

bool DiceBet::Clear() {
	_bet = -1;
	_count = 0;
	SoloGame::Clear();
	return true;
}

bool DiceBet::Process(Interaction interaction, bool start = false) {
	if (start) {
		SendMessageParams sP;
		sP.content = GetTextA("dice-bet-start");
		sP.channelID = interaction.channelID;
		auto selectmenus = std::make_shared<SelectMenu>();
		SelectMenu::Option selectOptions;
		selectOptions.value = "101010.5";
		selectOptions.label = client->GetTextL("dice-bet-odd");
		selectOptions.description = client->GetTextL("dice-bet-odd-des");
		selectmenus->options.push_back(selectOptions);
		selectOptions.value = "010101.5";
		selectOptions.label = client->GetTextL("dice-bet-even");
		selectOptions.description = client->GetTextL("dice-bet-even-des");
		selectmenus->options.push_back(selectOptions);
		selectOptions.value = "011010.5";
		selectOptions.label = client->GetTextL("dice-bet-prime");
		selectOptions.description = client->GetTextL("dice-bet-prime-des");
		selectmenus->options.push_back(selectOptions);
		selectOptions.value = "100001.7";
		selectOptions.label = client->GetTextL("dice-bet-oneorsix");
		selectOptions.description = client->GetTextL("dice-bet-oneorsix-des");
		selectmenus->options.push_back(selectOptions);
		selectOptions.value = "011110.2";
		selectOptions.label = client->GetTextL("dice-bet-notonenorsix");
		selectOptions.description = client->GetTextL("dice-bet-notonenorsix-des");
		selectmenus->options.push_back(selectOptions);
		selectOptions.value = "111110.1";
		selectOptions.label = client->GetTextL("dice-bet-imperfect");
		selectOptions.description = client->GetTextL("dice-bet-imperfect-des");
		selectmenus->options.push_back(selectOptions);
		selectmenus->customID = "solo-" + GetID() + "-start";

		auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
		actionRow->components.push_back(selectmenus);
		sP.components.push_back(actionRow);
		Message message = client->sendMessage(sP);
		SetMessage(message.ID);
		_dicem = client->sendMessage(_channel, "_ _");
	}
	else {
		std::vector<std::string> SplitID = client->split(interaction.data.customID, '-');
		if (SplitID[2] == "start") {
			_bet = interaction.data.values.front();
			_table = GetBetting() * 7 / 10;
			SplitID[2] = "go";
		}
		if (SplitID[2] == "go") {
			_count++;
			std::string tn = GetTextA(("dice-bet-" + _bet).c_str());
			int result = client->RollDice(interaction.channelID, false, 0.5, GetTextA("dice-bet-dicelabel", std::to_string(_count).c_str(), tn.c_str()), false, _dicem.ID);
			if (_bet[result - 1] == '1') {
				_table = _table + (_bet[7] - '1' + 1) * _table / 10 * _count + ((((_bet[7] - '1' + 1) * _table * 2 - 1) / 20 * _count == 0) ? 1 : 0);
				SleepyDiscord::Interaction::Response response;
				response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
				client->createInteractionResponse(interaction, interaction.token, response);

				EditMessageParams Ep;
				Ep.messageID = GameMessage();
				Ep.channelID = interaction.channelID;

				auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
				auto button1 = std::make_shared<SleepyDiscord::Button>();
				button1->style = SleepyDiscord::ButtonStyle::Success;
				button1->label = GetTextA("dice-bet-go");
				button1->customID = "solo-" + GetID() + "-go";
				button1->disabled = false;
				actionRow->components.push_back(button1);
				auto button2 = std::make_shared<SleepyDiscord::Button>();
				button2->style = SleepyDiscord::ButtonStyle::Danger;
				button2->label = GetTextA("dice-bet-stop");
				button2->customID = "solo-" + GetID() + "-stop";
				button2->disabled = false;
				actionRow->components.push_back(button2);

				Ep.components.push_back(actionRow);
				Ep.content = "_ _";
				Ep.embed = Success(result);
				client->editMessage(Ep);
			}
			else {
				SleepyDiscord::Interaction::Response response;
				response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
				client->createInteractionResponse(interaction, interaction.token, response);
				EditMessageParams Ep;
				Ep.embed = Fail(result);
				Ep.channelID = _channel;
				Ep.messageID = GameMessage();
				auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
				auto button1 = std::make_shared<SleepyDiscord::Button>();
				button1->style = SleepyDiscord::ButtonStyle::Danger;
				button1->label = GetTextA("dice-bet-gameover");
				button1->customID = "gameover";
				button1->disabled = true;
				actionRow->components.push_back(button1);
				Ep.components.push_back(actionRow);
				Ep.content = "_ _";
				client->editMessage(Ep);
				EndGame(false, _table);
			}
		}
		else {
			SleepyDiscord::Interaction::Response response;
			response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
			client->createInteractionResponse(interaction, interaction.token, response);
			EditMessageParams Ep;
			Ep.embed = Stop();
			Ep.channelID = _channel;
			Ep.messageID = GameMessage();
			auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
			auto button1 = std::make_shared<SleepyDiscord::Button>();
			button1->style = SleepyDiscord::ButtonStyle::Danger;
			button1->label = GetTextA("dice-bet-gameover");
			button1->customID = "gameover";
			button1->disabled = true;
			actionRow->components.push_back(button1);
			Ep.components.push_back(actionRow);
			Ep.content = "_ _";
			client->editMessage(Ep);
			EndGame(true, _table);
		}
	}
	return true;
}

Embed DiceBet::Success(int result) {
	Embed E;
	E.color = 8453970;
	E.title = client->GetTextL("dice-bet-success-title");
	E.description = client->GetTextL("dice-bet-success-description");
	EmbedField EF;
	EF.name = client->GetTextL("dice-bet-success-ifgo");
	EF.value = std::to_string(_table + (_bet[7] - '1' + 1) * _table / 10 * (_count + 1) + ((((_bet[7] - '1' + 1) * _table * 2 - 1) / 20 * _count == 0) ? 1 : 0));
	E.fields.push_back(EF);
	EF.name = client->GetTextL("dice-bet-success-ifstop");
	EF.value = std::to_string(_table);
	E.fields.push_back(EF);
	return E;
}

Embed DiceBet::Fail(int result) {
	Embed E;
	E.color = 12189724;
	E.title = client->GetTextL("dice-bet-fail-title");
	E.description = client->GetTextL("dice-bet-fail-description");
	return E;
}

Embed DiceBet::Stop() {
	Embed E;
	E.color = 16756224;
	E.title = client->GetTextL("dice-bet-stop-title");
	E.description = client->GetTextL("dice-bet-stop-description");
	return E;
}

bool BlackJack::Clear() {
	_stand = 0;
	_score = 0;
	_dealer = 0;
	_doubledown = 0;
	_deck.clear();
	SoloGame::Clear();
	return true;
}

int BlackJack::CardToScore(card C) {
	C.number++;
	if (C.number > 10) {
		C.number = 10;
	}
	if (C.number == 1) {
		if (C.mark == 1) {
			return 11;
		}
	}
	return C.number;
}

bool BlackJack::Process(Interaction interaction, bool start = false) {
	if (start) {
		_stand = 0;
		_score = 0;
		_dealer = 0;
		_doubledown = 0;
		_deck = qasino::ClassicDeck(true, false);
		_playerPacket.push_back(qasino::cardPick(_deck, qasino::ClassicDeck(true, false)));
		_playerPacket.push_back(qasino::cardPick(_deck, qasino::ClassicDeck(true, false)));
		_dealerPacket.push_back(qasino::cardPick(_deck, qasino::ClassicDeck(true, false)));
		if (_dealerPacket.back().number == 0) {
			if (_dealer > 10) {
				_dealerPacket.back().mark == 2;
			}
			else {
				_dealerPacket.back().mark == 1;
			}
		}
		Message message = client->sendMessage(_channel, "_ _");
		SetMessage(message.ID);
	}
	else {
		std::vector<std::string> SplitID = client->split(interaction.data.customID, '-');
		if (SplitID[2] == "hit") {
			SleepyDiscord::Interaction::Response response;
			response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
			client->createInteractionResponse(interaction, interaction.token, response);
			_playerPacket.push_back(qasino::cardPick(_deck, qasino::ClassicDeck(true, false)));
		}
		else if (SplitID[2] == "stand") {
			SleepyDiscord::Interaction::Response response;
			response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
			client->createInteractionResponse(interaction, interaction.token, response);
			_stand = true;
		}
		else if (SplitID[2] == "doubledown") {
			if (_player.GetInt(qasino::SYS_GAME_CHIP) >= _betting) {
				_betting *= 2;
				SleepyDiscord::Interaction::Response response;
				response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
				client->createInteractionResponse(interaction, interaction.token, response);
				_doubledown = true;
			}
			else {
				SleepyDiscord::Interaction::Response response;
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				response.data.content = GetTextA("blackjack1p-doubledown-fail");
				client->createInteractionResponse(interaction, interaction.token, response);
				return true;
			}
		}
		else if (SplitID[2] == "next") {
			SleepyDiscord::Interaction::Response response;
			response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
			client->createInteractionResponse(interaction, interaction.token, response);
			if (_doubledown) {
				_playerPacket.push_back(qasino::cardPick(_deck, qasino::ClassicDeck(true, false)));
				_doubledown = false;
				_stand = true;
			}
			else {
				_dealerPacket.push_back(qasino::cardPick(_deck, qasino::ClassicDeck(true, false)));
				if (_dealerPacket.back().number == 0) {
					if (_dealer > 10) {
						_dealerPacket.back().mark == 2;
					}
					else {
						_dealerPacket.back().mark == 1;
					}
				}
			}
		}
		else if (SplitID[2] == "aceno") {
			SleepyDiscord::Interaction::Response response;
			response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
			client->createInteractionResponse(interaction, interaction.token, response);
			_playerPacket.back().mark = 2;
		}
		else if (SplitID[2] == "aceyes") {
			SleepyDiscord::Interaction::Response response;
			response.type = SleepyDiscord::Interaction::Response::Type::DeferredUpdateMessage;
			client->createInteractionResponse(interaction, interaction.token, response);
			_playerPacket.back().mark = 1;
		}
	}
	std::string player;
	int& playerscore = _score;
	playerscore = 0;
	std::string error;
	for (int i = 0; i < _playerPacket.size(); i++) {
		error = _playerPacket[i].emoji();
		replaceAll(error, "<:card_black_1:917005102563332096>", "<:bjcard_black_11:932820184316649483>");
		replaceAll(error, "<:card_red_1:917005102051651604>", "<:bjcard_red_11:932820184434081833>");
		player += _playerPacket[i].mark == 1 ? error : _playerPacket[i].emoji();
		playerscore += CardToScore(_playerPacket[i]);
	}
	std::string dealer;
	int& dealerscore = _dealer;
	dealerscore = 0;
	for (int i = 0; i < _dealerPacket.size(); i++) {
		error = _dealerPacket[i].emoji();
		replaceAll(error, "<:card_black_1:917005102563332096>", "<:bjcard_black_11:932820184316649483>");
		replaceAll(error, "<:card_red_1:917005102051651604>", "<:bjcard_red_11:932820184434081833>");
		dealer += _dealerPacket[i].mark==1?error: _dealerPacket[i].emoji();
		dealerscore += CardToScore(_dealerPacket[i]);
	}
	if (_dealerPacket.size() == 1) {
		dealerscore = CardToScore(_dealerPacket[0]);
		dealer += qasino::cardBack();
	}
	EditMessageParams Ep;
	Ep.content = GetTextA("blackjack1p-table", player.c_str(),
		dealer.c_str(),
		std::to_string(playerscore).c_str(),
		std::to_string(dealerscore).c_str());
	Ep.channelID = _channel;
	Ep.messageID = GameMessage();

	if (_playerPacket.back().number == 0 && _playerPacket.back().mark == 0) {
		Ep.content += GetTextA("blackjack1p-ace");
		auto button1 = std::make_shared<SleepyDiscord::Button>();
		auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
		button1->style = SleepyDiscord::ButtonStyle::Success;
		button1->label = GetTextA("blackjack-aceyes");
		button1->customID = "solo-" + GetID() + "-aceyes";
		button1->disabled = false;
		actionRow->components.push_back(button1);

		auto button2 = std::make_shared<SleepyDiscord::Button>();
		button2->style = SleepyDiscord::ButtonStyle::Danger;
		button2->label = GetTextA("blackjack-aceno");
		button2->customID = "solo-" + GetID() + "-aceno";
		button1->disabled = false;
		actionRow->components.push_back(button2);
		Ep.components.clear();
		Ep.components.push_back(actionRow);
		client->editMessage(Ep);
		return true;
	}

	auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
	auto button1 = std::make_shared<SleepyDiscord::Button>();
	button1->style = SleepyDiscord::ButtonStyle::Primary;
	button1->label = GetTextA("blackjack-hit");
	button1->customID = "solo-" + GetID() + "-hit";
	button1->disabled = _doubledown || _stand;
	actionRow->components.push_back(button1);

	auto button2 = std::make_shared<SleepyDiscord::Button>();
	button2->style = SleepyDiscord::ButtonStyle::Secondary;
	button2->label = GetTextA("blackjack-stand");
	button2->customID = "solo-" + GetID() + "-stand";
	button2->disabled = _doubledown || _stand;
	actionRow->components.push_back(button2);

	auto button3 = std::make_shared<SleepyDiscord::Button>();
	button3->style = SleepyDiscord::ButtonStyle::Danger;
	button3->label = GetTextA("blackjack-doubledown");
	button3->customID = "solo-" + GetID() + "-doubledown";
	button3->disabled = _doubledown || _stand;
	actionRow->components.push_back(button3);

	Ep.components.push_back(actionRow);

	if (playerscore > 21) {
		Ep.content = GetTextA("blackjack1p-table-bust", player.c_str(),
			dealer.c_str(),
			std::to_string(playerscore).c_str(),
			std::to_string(dealerscore).c_str());
		auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
		auto button1 = std::make_shared<SleepyDiscord::Button>();
		button1->style = SleepyDiscord::ButtonStyle::Danger;
		button1->label = GetTextA("dice-bet-gameover");
		button1->customID = "gameover";
		button1->disabled = true;
		actionRow->components.push_back(button1);
		Ep.components.clear();
		Ep.components.push_back(actionRow);
		client->editMessage(Ep);
		EndGame(false, 0);
		return 0;
	}else if(playerscore == 21 && _playerPacket.size() == 2){
		Ep.content = GetTextA("blackjack1p-table-bj", player.c_str(),
			dealer.c_str(),
			std::to_string(dealerscore).c_str());
		auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
		auto button1 = std::make_shared<SleepyDiscord::Button>();
		button1->style = SleepyDiscord::ButtonStyle::Danger;
		button1->label = GetTextA("dice-bet-gameover");
		button1->customID = "gameover";
		button1->disabled = true;
		actionRow->components.push_back(button1);
		Ep.components.clear();
		Ep.components.push_back(actionRow);
		client->editMessage(Ep);
		EndGame(true, _betting * 2);
	}
	if (_doubledown || _stand) {
		Ep.components.clear();

		auto button4 = std::make_shared<SleepyDiscord::Button>();
		button4->style = SleepyDiscord::ButtonStyle::Success;
		button4->label = GetTextA("blackjack-next");
		button4->customID = "solo-" + GetID() + "-next";
		button4->disabled = false;
		actionRow->components.push_back(button4);
		Ep.components.push_back(actionRow);
		if (_stand) {
			Ep.content = GetTextA("blackjack1p-table-stand", player.c_str(),
				dealer.c_str(),
				std::to_string(playerscore).c_str(),
				std::to_string(dealerscore).c_str());
		}
		if (_doubledown) {
			Ep.content = GetTextA("blackjack1p-table-doubledown", player.c_str(),
				dealer.c_str(),
				std::to_string(playerscore).c_str(),
				std::to_string(dealerscore).c_str());
		}
	}
	if (_stand && (dealerscore > 21 || (dealerscore >= 17 && playerscore > dealerscore))) {
		Ep.content = GetTextA("blackjack1p-table-playerwin", player.c_str(),
			dealer.c_str(),
			std::to_string(playerscore).c_str(),
			std::to_string(dealerscore).c_str());
		auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
		auto button1 = std::make_shared<SleepyDiscord::Button>();
		button1->style = SleepyDiscord::ButtonStyle::Danger;
		button1->label = GetTextA("dice-bet-gameover");
		button1->customID = "gameover";
		button1->disabled = true;
		actionRow->components.push_back(button1);
		Ep.components.clear();
		Ep.components.push_back(actionRow);
		client->editMessage(Ep);
		EndGame(true, _betting*2);
		return 0;
	}
	if (_stand && (playerscore <= dealerscore && dealerscore >= 17)) {
		Ep.content = GetTextA("blackjack1p-table-dealerwin", player.c_str(),
			dealer.c_str(),
			std::to_string(playerscore).c_str(),
			std::to_string(dealerscore).c_str());
		auto actionRow = std::make_shared<SleepyDiscord::ActionRow>();
		auto button1 = std::make_shared<SleepyDiscord::Button>();
		button1->style = SleepyDiscord::ButtonStyle::Danger;
		button1->label = GetTextA("dice-bet-gameover");
		button1->customID = "gameover";
		button1->disabled = true;
		actionRow->components.push_back(button1);
		Ep.components.clear();
		Ep.components.push_back(actionRow);
		client->editMessage(Ep);
		EndGame(false, 0);
		return 0;
	}
	client->editMessage(Ep);

	return true;
}

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

std::string GetToken(const char* key, ...) {
	std::string ret = GetTextA(key);
	replaceAll(ret, "\n", "");
	replaceAll(ret, "\r", "");
	return hexdeprint(ret);
}

std::string QasinoBot::GetTextL(const char* key, ...) {
	std::string ret = GetTextA(key);
	replaceAll(ret, "\n", "");
	replaceAll(ret, "\r", "");
	return ret;
}
std::string QasinoBot::GetTextR(const char* key, ...) {
	std::string ret = GetTextA(key);
	replaceAll(ret, "\r", "");
	return ret;
}

void QasinoBot::Clear() {
	_sl = getServers().vector();
	_uptimet = time(0);
	_uptime = _uptimet;

	QasinoServer = client->getServer(QasinoServerID);
}

SleepyDiscord::ObjectResponse<SleepyDiscord::Message> QasinoBot::sendPrintf(SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID, const char* format, ...)
{
	char szBuf[1024] = { 0, };

	va_list lpStart;
	va_start(lpStart, format);
	vsprintf(szBuf, format, lpStart);
	va_end(lpStart);

	return sendMessage(channelID, szBuf);
}

int QasinoBot::sendDM(std::string UserID, std::string Content) {
	Channel C = createDirectMessageChannel(UserID);
	sendMessage(C, Content);
	return 0;
}

qasino::qambler QasinoBot::readQamblerInfo(std::string ID) {
	Json::Value Qj;
	std::ifstream readFile;
#ifdef _WIN32
	readFile.open("database\\qamblers\\" + ID + ".json");
#endif
#ifdef linux
	printf("linux is detected!\n");
	readFile.open("/home/phoenix/discord-bot/build/database/qamblers/" + ID + ".json");
#endif
	qasino::qambler Qb;
	Qb.ID = ID;

	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	JSONCPP_STRING errs;
	bool ok = parseFromStream(builder, readFile, &Qj, &errs);

	if (readFile.is_open() && ok) {
		for (int i = 0; i < 100; i++) {
			Qb.info[i] = Qj["info"][i].asString();
		}
		Qb.nick = hexdeprint(Qj["nick"].asString());
	}
	else {
		Qb.nick = "";
		for (int i = 0; i < 100; i++) {
			Qb.info[i] = "0";
		}
		Qb.SetInt(qasino::SYS_MONEY, 100000);
		writeQamblerInfo(Qb);
	}
	if (Qb.nick == "") {
		User U = getUser(ID);
		Qb.nick = U.username;
	}
	return Qb;
}

bool QasinoBot::writeQamblerInfo(qasino::qambler Qb) {
	printf("writeQamblerInfo [%s]", hexprint(Qb.ID).c_str());
	std::ofstream ofile;
#ifdef _WIN32
	ofile.open("database\\qamblers\\" + Qb.ID + ".json");
#endif
#ifdef linux
	ofile.open("/home/phoenix/discord-bot/build/database/qamblers/" + Qb.ID + ".json");
#endif
	Json::Value Qj;
	for (int i = 0; i < 100; i++) {
		Qj["info"][i] = Qb.info[i];
	}
	Qj["nick"] = hexprint(Qb.nick);
	Json::StreamWriterBuilder builder; 
	builder["commentStyle"] = "None"; 
	builder["indentation"] = " "; // Tab 
	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	writer->write(Qj, &ofile);
	writer->write(Qj, &std::cout);

	return true;
}

void QasinoBot::readStock(qasino::stock& S) {
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
			replaceAll(str, "\r", "");
			replaceAll(str, "\n", "");
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

void QasinoBot::writeStock(qasino::stock S) {
	std::ofstream ofile;
#ifdef _WIN32
	ofile.open("database\\stocks\\" + S.name + ".txt");
#endif
#ifdef linux
	printf("linux is detected!\n");
	ofile.open("/home/phoenix/discord-bot/build/database/stocks/" + S.name + ".txt");
#endif

	std::string out = "";
	for (int i = 0; i < 100; i++) {
		out += std::to_string(S.value[i]) + "\n";
	}

	ofile << out;
}

Embed QasinoBot::qamblerProfile(qasino::qambler Qb) {
	Embed E;
	EmbedField EF;
	E.title = GetTextA("profile-title", Qb.nick.c_str());
	EF.name = GetTextA("profile-nick");
	EF.value = Qb.nick;
	EF.isInline = true;
	E.fields.push_back(EF);
	EF.name = GetTextA("profile-money");
	EF.value = Qb.info[qasino::SYS_MONEY];
	EF.value += "Q$";
	EF.isInline = true;
	E.fields.push_back(EF);
	EF.name = GetTextA("profile-chip");
	EF.value = Qb.info[qasino::SYS_GAME_CHIP];
	EF.value += GetTextA("profile-chip-count");
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

std::vector<std::string> QasinoBot::split(std::string input, char delimiter) {
	std::vector<std::string> answer;
	std::stringstream ss(input);
	std::string temp;

	while (std::getline(ss, temp, delimiter)) {
		answer.push_back(temp);
	}

	return answer;
}

void QasinoBot::UpdStk() {
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

void QasinoBot::UpdNick(std::string ID, std::string nick, bool IsBeggar = false) {
	qasino::qambler Qb = client->readQamblerInfo(ID);
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

void QasinoBot::DiceEdit(std::string MessageID, std::string ChannelID, Embed E, int result, bool Delete) {
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
	Ep.embed.description = GetTextA("dice-end-dsc", Numbers[result - 1].c_str());
	Ep.embed.thumbnail.url = Images[result - 1];
	editMessage(Ep);
	if (Delete) {
		schedule([this, ChannelID, MessageID]() {this->deleteMessage(ChannelID, MessageID); }, 2000);
	}
}

int QasinoBot::RollDice(std::string ChannelID, 
	bool iseasteregg = false, 
	float time = 1, 
	std::string name = "~~~", 
	bool Delete = false,
	std::string editmessage = "~~~") {
	if (name == "~~~") {
		name = GetTextL("dice-title");
	}
	Embed E;
	EmbedField EF;
	E.title = name;
	E.description = GetTextL("dice-roll-dsc");
	E.thumbnail.url = "https://media.discordapp.net/attachments/843653570158657547/910283898699800616/diceroll.gif";
	SendMessageParams Sp;
	EditMessageParams Ep;
	Message Msg;
	if(editmessage == "~~~"){
		Sp.embed = E;
		Sp.content = "_ _";
		Sp.channelID = ChannelID;
		Msg = sendMessage(Sp);
	}
	else {
		Ep.embed = E;
		Ep.content = "_ _";
		Ep.channelID = ChannelID;
		Ep.messageID = editmessage;
		//editMessage(Ep);
		Msg = getMessage(ChannelID, editmessage);
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(1, 6);
	std::uniform_int_distribution<int> esdis(1, 12);
	std::uniform_int_distribution<int> es(1, 1);
	int result = dis(rd);
	if (iseasteregg && esdis(rd) == 1) {
		result = 6 + es(rd);
	}
	schedule([this, result, E, ChannelID, Delete, Msg]() {this->DiceEdit(Msg.ID.string(), ChannelID, E, result, Delete); }, 1000 * time);
	return result;
}

long long QasinoBot::CurrentTime() {
	time_t now = time(0);
	long long t = now;
	return t;
}

void QasinoBot::saveMessage(Message message) {
	std::ofstream ofile;
#ifdef _WIN32
	ofile.open("database\\log.txt", std::ios_base::app);
#endif
#ifdef linux
	ofile.open("/home/phoenix/discord-bot/build/database/log.txt", std::ios_base::app);
#endif

	ofile << message.author.username + ": " + message.content << endl;

	for (int i = 0; i < message.attachments.size(); i++) {
		ofile << " -> File : " + message.attachments.at(i).url << endl;
	}

}

void QasinoBot::UpdSt1() {
	updateStatus(GetTextA("StatusGame1", std::to_string(_sl.size()).c_str()), 0, online);
	schedule([this]() {this->UpdSt2(); }, 5000);
}
void QasinoBot::UpdSt2() {
	updateStatus(GetTextA("StatusGame2"), 0, doNotDisturb);
	schedule([this]() {this->UpdSt1(); }, 5000);
}

void QasinoBot::onReady(Ready readyData) {
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

	option2.at(0).type = AppCommand::Option::Type::STRING;
	option2.at(0).name = "game";
	option2.at(0).description = GetTextA("solo-game-game");
	option2.at(0).isRequired = true;

	choice.value = "dice-bet";
	choice.name = GetTextL("solo-dice-bet");
	option2.at(0).choices.push_back(std::move(choice));
	choice.value = "blackjack";
	choice.name = GetTextL("solo-blackjack");
	option2.at(0).choices.push_back(std::move(choice));

	/*---*/

	option2.at(1).type = AppCommand::Option::Type::INTEGER;
	option2.at(1).name = "betting";
	option2.at(1).description = GetTextA("solo-game-betting");
	option2.at(1).isRequired = true;

	//createGlobalAppCommand(QasinoAppID, "solo-game", GetTextA("solo-game"), option2);

	/*option2.at(0).type = AppCommand::Option::Type::USER;
	option2.at(0).name = "qambler";
	option2.at(0).description = GetTextA("send-qambler");
	option2.at(0).isRequired = true;
	option2.at(0).choices.clear();

	option2.at(1).type = AppCommand::Option::Type::INTEGER;
	option2.at(1).name == "money";
	option2.at(1).description = GetTextA("send-money");
	option2.at(1).isRequired = true;
	option2.at(1).choices.clear();

	createGlobalAppCommand(QasinoAppID, "send", GetTextA("send"), option2);*/

	/*0option2.at(0).type = AppCommand::Option::Type::STRING;
	option2.at(0).name = "action";
	option2.at(0).description = GetTextA("chip-action");
	option2.at(0).isRequired = true;

	option2.at(0).choices.clear();
	choice.value = "buy";
	choice.name = GetTextL("chip-buy");
	option2.at(0).choices.push_back(std::move(choice));
	choice.value = "sell";
	choice.name = GetTextL("chip-sell");
	option2.at(0).choices.push_back(std::move(choice));

	option2.at(1).type = AppCommand::Option::Type::INTEGER;
	option2.at(1).name = "count";
	option2.at(1).description = GetTextA("chip-count");
	option2.at(1).isRequired = true;

	createGlobalAppCommand(QasinoAppID, "chip", GetTextA("chip"), option2);


	createGlobalAppCommand(QasinoAppID, "uptime", GetTextA("uptime")); 

	option.at(0).type = AppCommand::Option::Type::INTEGER;
	option.at(0).name = "count";
	option.at(0).description = GetTextA("cardpick-count");
	option.at(0).isRequired = false;

	createGlobalAppCommand(QasinoAppID, "cardpick", GetTextA("cardpick"), option);*/


	stocks.push_back(qasino::CreateStock("Stock", 100, 20, 1, 3000));
	stocks.push_back(qasino::CreateStock("Inverse", 100, 20, 1, 3000));
	stocks.push_back(qasino::CreateStock("Triple", 900, 60, 0, 9000));
	stocks.push_back(qasino::CreateStock("Bitcoin", 3000, 100, 0, 50000));

	_globalDeck = qasino::ClassicDeck(true, true);

	UpdStk();
}

void QasinoBot::onMessage(SleepyDiscord::Message message) {
	saveMessage(message);
	std::vector<std::string> input;
	input = split(message.content, ' ');
	if (message.author.ID.string() == Q_ID || message.author.ID.string() == "273798478755528704") {
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
				qasino::qambler Qb = client->readQamblerInfo(nickID);
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
			if (input[0] == "||info") {
				std::string infoID = input[1].substr(3, input[1].size() - 4);
				int index;
				std::stringstream SS(input[2]);
				SS >> index;
				qambler Qb = readQamblerInfo(infoID);
				Qb.info[index] = input[3];
				writeQamblerInfo(Qb);
			}
		}
	}
}

void QasinoBot::onInteraction(Interaction interaction) {
	std::srand(static_cast<int>(std::time(0)));
	SleepyDiscord::Interaction::Response response;
	SleepyDiscord::WebHookParams followUp;
	std::string customid = interaction.data.customID;

	std::vector<std::string> SplitID = split(customid, '-');

	Channel GC = client->getChannel(interaction.channelID);
	if (GC.type == Channel::DM) {
		response.data.content = GetTextA("DM_X");
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
		createInteractionResponse(interaction, interaction.token, response);
		return;
	}
	qasino::qambler iQB = readQamblerInfo(interaction.member.ID);
	if (CurrentTime() - iQB.GetLLong(qasino::ECO_BEGGAR) <= 7200) {
		response.data.content = GetTextA("beggar-response", iQB.nick.substr(0, 6).c_str(), std::to_string(iQB.GetLLong(qasino::ECO_BEGGAR)).c_str(), iQB.nick.c_str());
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		createInteractionResponse(interaction, interaction.token, response);
		return;
	}
	else if (iQB.info[qasino::ECO_ISBEGGAR] == "true") {
		UpdNick(iQB.ID, iQB.nick.substr(7), true);
	}

	if (CurrentTime() - iQB.GetLLong(qasino::SYS_SPAMTIME) <= 3) {
		response.data.content = GetTextA("SPAM_X");
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
		createInteractionResponse(interaction, interaction.token, response);
		return;
	}

	iQB.SetLLong(qasino::SYS_SPAMTIME, CurrentTime());
	writeQamblerInfo(iQB);

	if (interaction.type == Interaction::Type::MessageComponent) {
		if (SplitID[0] == "solo") {
			SoloGame* soloGame;
			bool init = false;
			for (std::vector<SoloGame*>::iterator it = _sologames.begin(); it != _sologames.end(); it++) {
				soloGame = *it;
				if (soloGame->GetID() == SplitID[1]) {
					init = true;
					break;
				}
			}
			if (init) {
				if (soloGame->GetPlayer().ID == interaction.member.ID.string()) {
					soloGame->Process(std::move(interaction));
				}
			}
			else {
				deleteMessage(interaction.channelID, interaction.message);
				iQB.SetInt(qasino::SYS_IS_PLAYING, 0);
				writeQamblerInfo(iQB);
				response.data.content = GetTextA("wrong-interaction");
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
			}
		}
	}
	else {
		if (iQB.GetInt(qasino::SYS_IS_PLAYING)) {
			response.data.content = GetTextA("PL_X");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
			return;
		}
	}
	if (interaction.data.name == "help") {
		response.data.content = GetTextA("botcommand");
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
		createInteractionResponse(interaction, interaction.token, response);
	}
	else if (interaction.data.name == "uptime") {
		response.data.content = GetTextA("uptime-content", std::to_string(_uptime).c_str());
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
		createInteractionResponse(interaction, interaction.token, response);
	}
	else if (interaction.data.name == "profile") {
		std::string UID;
		if (interaction.data.options.size() == 0) {
			UID = interaction.member.ID;
		}
		else {
			UID = interaction.data.options.at(0).value.GetString();
		}
		qasino::qambler Qb = client->readQamblerInfo(UID);
		response.data.embeds.push_back(qamblerProfile(Qb));
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
		qasino::qambler Qb = client->readQamblerInfo(interaction.member.ID);
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

			Qb.ChangeInt(qasino::SYS_MONEY, count * stocks.at(stn).value[99] * 97 / 100);
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
				stocks.at(stn).print(count, Qb.GetInt(qasino::ECO_STOCK_MONEY + 2 * stn) / Qb.GetInt(qasino::ECO_STOCK + 2 * stn) * 100 / 97);
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
#ifdef _WIN32
			uploadFile(Sp, "database\\stocks\\" + stocks.at(stn).name + "Chart.md");
#endif
#ifdef linux
			uploadFile(Sp, "/home/phoenix/discord-bot/build/database/stocks/" + stocks.at(stn).name + "Chart.md");
#endif
		}
	}
	else if (interaction.data.name == "nick") {
		std::string nick = interaction.data.options.at(0).value.GetString();
		qasino::qambler Qb = client->readQamblerInfo(interaction.member.ID);
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
		qasino::qambler Qb = client->readQamblerInfo(interaction.member.ID);
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
				Qb.ChangeInt(qasino::SYS_MONEY, money);
				UpdNick(std::move(interaction.member.ID), GetTextL("beg-beggar") + " " + Qb.nick);
				writeQamblerInfo(Qb);
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
		qasino::qambler Qb = client->readQamblerInfo(interaction.member.ID);
		std::string title = GetTextA("dice-title-w-nick", Qb.nick.c_str());
		RollDice(interaction.channelID, false, 3, title);
	}
	else if (interaction.data.name == "solo-game") {
		std::string gametype = interaction.data.options.at(0).value.GetString();
		if (gametype == "dice-bet") {
			DiceBet* dicebet = new DiceBet(interaction.ID);
			_sologames.push_back(dicebet);
		}
		if (gametype == "blackjack") {
			BlackJack* blackjack = new BlackJack(interaction.ID);
			_sologames.push_back(blackjack);
		}
		SoloGame* soloGame;
		for (std::vector<SoloGame*>::iterator it = _sologames.begin(); it != _sologames.end(); it++) {
			soloGame = *it;
			if (soloGame->GetID() == interaction.ID.string()) {
				break;
			}
		}
		if (soloGame->OnStart(std::move(interaction))) {
			soloGame->Process(std::move(interaction), true);
		}
	}
	else if (interaction.data.name == "send") {
		qasino::qambler from = readQamblerInfo(interaction.member.ID);
		qasino::qambler to = readQamblerInfo(interaction.data.options.at(0).value.GetString());
		int money = interaction.data.options.at(1).value.GetInt();

		if (from.GetInt(qasino::SYS_MONEY) < money || money <= 0) {
			response.data.content = GetTextA("send-fail");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
			return;
		}

		response.data.content = GetTextA("send-success", from.ID.c_str(), to.ID.c_str(), std::to_string(money).c_str());
		response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
		createInteractionResponse(interaction, interaction.token, response);

		from.ChangeInt(qasino::SYS_MONEY, -1 * money);
		to.ChangeInt(qasino::SYS_MONEY, money);
		writeQamblerInfo(from);
		writeQamblerInfo(to);
	}
	else if (interaction.data.name == "chip") {
		std::string action = interaction.data.options.at(0).value.GetString();
		int count = interaction.data.options.at(1).value.GetInt();
		qasino::qambler Qb = readQamblerInfo(interaction.member.ID);

		if (action == "buy") {
			if (Qb.GetInt(qasino::SYS_MONEY) >= count * 1000) {
				Qb.ChangeInt(qasino::SYS_MONEY, -1 * count * 1000);
				Qb.ChangeInt(qasino::SYS_GAME_CHIP, count);
				writeQamblerInfo(Qb);
				response.data.content = GetTextA("chip-buy-msg", std::to_string(count).c_str());
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
				return;
			}
			response.data.content = GetTextA("chip-buy-fail");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
		}
		else {
			if (Qb.GetInt(qasino::SYS_GAME_CHIP) >= count) {
				Qb.ChangeInt(qasino::SYS_MONEY, count * 950);
				Qb.ChangeInt(qasino::SYS_GAME_CHIP, -1 * count);
				writeQamblerInfo(Qb);
				response.data.content = GetTextA("chip-sell-msg", std::to_string(count).c_str());
				response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
				response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
				createInteractionResponse(interaction, interaction.token, response);
				return;
			}
			response.data.content = GetTextA("chip-sell-fail");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			response.data.flags = InteractionAppCommandCallbackData::Flags::Ephemeral;
			createInteractionResponse(interaction, interaction.token, response);
		}
	}
	else if (interaction.data.name == "cardpick") {
		int count = interaction.data.options.at(0).value.GetInt();
		if (count < 1 || count > 20) {
			response.data.content = GetTextA("cardpick-invalid");
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			createInteractionResponse(interaction, interaction.token, response);
		}
		else {
			response.data.content = GetTextA("cardpick-process", std::to_string(count).c_str());
			std::string packetemoji;
			deck packet;
			for (int i = 0; i < count; i++) {
				packet.push_back(qasino::cardPick(_globalDeck, qasino::ClassicDeck(true, true)));
			}
			for (int i = 0; i < count; i++) {
				packetemoji += packet[i].emoji();
			}
			response.type = SleepyDiscord::Interaction::Response::Type::ChannelMessageWithSource;
			createInteractionResponse(interaction, interaction.token, response);
			sendMessage(interaction.channelID, packetemoji);
		}
	}
	else {

	}
}


int main()
{
	printf("QasinoBot execute\n");
	textManager.Initialize("qasino.txt");

	printf("%s", hexdeprint(GetToken("QasinoToken")).c_str());
	client = new QasinoBot(GetToken("QasinoToken"), SleepyDiscord::Mode::USER_CONTROLED_THREADS);
	client->Clear();
	client->setIntents(Intent::SERVER_MESSAGES, Intent::DIRECT_MESSAGES, Intent::SERVER_MESSAGE_REACTIONS);
	client->run();
}

// QasinoBot server script

// Made by Q_
// Q¤Ñ#0283