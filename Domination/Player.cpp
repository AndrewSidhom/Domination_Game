#include "Player.h"

#include <iostream>
#include <ctime>
using namespace std;

int* Player::currentGenId = new int(1);

//constructor, destructor
Player::Player() : id(new int(*currentGenId)), name(new string("Player " + to_string(*id))), ownedCountries(new map<int, Country*>), numOfOwnedCountriesPerContinent(new map<int, int>), hand(NULL), dice(new Dice()) {	genNextID(); }
Player::Player(string name, Deck *deck, Map *mapPtr) : id(new int(*currentGenId)), name(new string(name)), ownedCountries(new map<int, Country*>), numOfOwnedCountriesPerContinent(new map<int,int>), mapPtr(mapPtr), hand(new Hand(deck, ownedCountries)), dice(new Dice()){ genNextID(); }
Player::Player(const Player &p) : id(p.id), name(p.name), ownedCountries(p.ownedCountries), mapPtr(p.mapPtr), hand(p.hand), dice(p.dice), numOfOwnedCountriesPerContinent(p.numOfOwnedCountriesPerContinent) {}
Player::~Player() { delete id, name, ownedCountries, numOfOwnedCountriesPerContinent, hand, dice, mapPtr; }

/*	 Used in the startup phase of the game. Stores all owned countries and places 1 army on each.
	 And sets how many owned countries are in each continent for reinforcing armies.
*/
void Player::setOwnedCountries(list<Country*> countriesList) 
{
	for (Country* country : countriesList) {
		country->player = this;
		country->armies = 0;
		(*ownedCountries)[country->id] = country;
		(*numOfOwnedCountriesPerContinent)[country->continentId] += 1;
	}
}

//used during attack(). Adds this country to the ones owned by the player, places on it this many armies.
void Player::claimCountry(Country* country, int armies)
{
	country->player = this;
	country->armies = armies;
	(*ownedCountries)[country->id] = country;
	(*numOfOwnedCountriesPerContinent)[country->continentId] += 1;
}

//used during attack(). Returns a pointer to the lost country so that another player can add it to their collection. Returns nullptr if the country with this id is not owned.
Country* Player::loseCountry(int id)
{
	if (ownedCountries->count(id) == 0)
		return nullptr;
	else {
		Country* country = (*ownedCountries)[id];
		ownedCountries->erase(id);
		country->player = nullptr;
		country->armies = -1;
		(*numOfOwnedCountriesPerContinent)[country->continentId] -= 1;

		return country;
	}
}

// Returns true if Player decides to attack, false otherwise
bool Player::decideToAttack()
{
	for (auto ownedCountry = ownedCountries->begin(); ownedCountry != ownedCountries->end(); ownedCountry++) {
		// continue attacking until there is no more countries that can be attacked
		// a country can be attacked if 1. it's a neighbour of an owned country
		// 2. it's not owned by the player
		// 3. the owned country has at least two armies
		list<int> neighbors = ownedCountry->second->neighbors;

		for (auto const& neighbor : neighbors) {
			Country* countryNeighbor = mapPtr->getCountryById(neighbor);

			if (countryNeighbor->player->getId() != *id && ownedCountry->second->armies >= 2) {
				return true;
			}
		}
	}

	return false;
}

// Returns a pointer to the Country which will be attacking
Country * Player::selectAttackingCountry()
{
	// the attacking country must 1. have a neighbour that is not owned by that player
	// 2. have at least two armies
	Country* attackingCountry;

	for (auto ownedCountry = ownedCountries->begin(); ownedCountry != ownedCountries->end(); ownedCountry++) {
		list<int> neighbors = ownedCountry->second->neighbors;

		for (auto const& neighbor : neighbors) {
			Country* countryNeighbor = mapPtr->getCountryById(neighbor);

			if (countryNeighbor->player->getId() != *id && ownedCountry->second->armies >= 2) {
				attackingCountry = ownedCountry->second;
				return attackingCountry;
			}
		}
	}

	throw exception("No countries owned by this Player can attack another country.");
}

// Returns a pointer to the Country which will be attacked
Country * Player::selectDefendingCountry(Country * attackingCountry)
{
	// The attack target must 1. be a neighbour of the attacking country
	// 2. must not be owned by this Player
	Country* attackTarget;

	list<int> neighbors = attackingCountry->neighbors;
	for (auto const& neighbor : neighbors) {
		Country* countryNeighbor = mapPtr->getCountryById(neighbor);

		if (countryNeighbor->player->getId() != *id) {
			attackTarget = countryNeighbor;
			return attackTarget;
		}
	}

	throw exception("No countries can be an attack target for the provided attacking country.");
}

// Returns an integer - the number of dice to be rolled by the attacker
int Player::selectAttackDice(Country * attackingCountry)
{
	// The attack is allowed min: 1 die, max: (number of armies in the attacking Country - 1) dice
	// Will select the max number of dice to roll
	int max = attackingCountry->armies - 1;

	return max;
}

// Returns an integer - the number of dice to be rolled by the defender
int Player::selectDefenseDice(Country * defendingCountry)
{
	// The defense is allowed min: 1 die, max: 2 dice if the number of armies in defendingCountry >= 2
	// Will select the min number of dice to roll

	return 1;
}

// Returns the number of armies the attacker will move in newly conquered country
int Player::selectArmiesToMoveAfterAttackSuccess(Country * attackingCountry, Country * defendingCountry, int diceRolled)
{
	// The Player is allowed to move min: (number of dice rolled) armies, max: (number of armies in attacking Country - 1) armies
	// Will select the max number
	int max = attackingCountry->armies - 1;

	return max;
}

// TODO: to be removed?
//rolls this number of dice, returns dice results
vector<int> Player::rollDice(int howMany)
{
	//needs the Dice class to provide a roll() function that takes the number of dice as parameter. 
	return vector<int>();
}

/*	Get reinforcement armies and let player distribute given armies.
*/
void Player::reinforce() {

	cout << "**Reinforcement Phase**\n";
	int armies = 0, r = 0;
	armies += getCountryReinforcement();
	armies += getContinentReinforcement();

	while (hand->getHandCount() >= 3) {
		r += hand->exchange();
		if (r == 0)	// note 0 means user cancelled exchange action
			break;
		else if (hand->getHandCount() >= 3)
			cout << "\nYou still have " << hand->getHandCount() << " cards left.\n";
		armies += r;
		cout << "\nArmies from exchanging: " << r << endl;	// TODO remove after testing phase 2
		r = 0;
	}
	cout << "\nTotal reinforcement armies: " << armies << endl;	// TODO remove after testing phase 2

	if (!disableArmyDistribution) // TODO remove after testing phase 2
		distributeArmies(armies, false);
}

//the player carries out a number of attacks
void Player::attack()
{
	// TODO: 3. driver!!
	if (decideToAttack()) {
		bool atLeastOneCountryConquered = false;

		while (decideToAttack()) {
			cout << endl << "Player " << *name << " chose to attack!" << endl;

			// Select attacking and defending Countries
			Country* attackingCountry = selectAttackingCountry();
			Country* defendingCountry = selectDefendingCountry(attackingCountry);

			cout << endl << "Player " << *name << " will use Country " << attackingCountry->name << " to attack Player ";
			cout << *(defendingCountry->player->name) << "'s Country " << defendingCountry->name << endl;

			// Select number of dice to roll
			int numAttackDice = selectAttackDice(attackingCountry);
			int numDefenseDice = selectDefenseDice(defendingCountry);

			cout << endl << "Player " << *name << " will roll " << numAttackDice << " dice" << endl;
			cout << "Player " << *(defendingCountry->player->name) << " will roll " << numDefenseDice << " dice" << endl;

			// Roll dice
			list<int> attackRolls = dice->roll(numAttackDice);
			list<int> defenseRolls = dice->roll(numDefenseDice);

			cout << endl << "Player " << *name << " rolled:";
			for (auto const roll : attackRolls) {
				cout << " " << roll << " ";
			}

			cout << endl << "Player " << *(defendingCountry->player->name) << " rolled:";
			for (auto const roll : defenseRolls) {
				cout << " " << roll << " ";
			}
			cout << endl;

			// Calculate army loss for attacking and defending Countries
			int attackerLoss = 0;
			int defenderLoss = 0;
			auto itAttackRolls = attackRolls.begin();
			auto itDefenseRolls = defenseRolls.begin();
			while (itAttackRolls != attackRolls.end() && itDefenseRolls != defenseRolls.end()) {
				if (*itAttackRolls > *itDefenseRolls) {
					defenderLoss++;
				}
				else {
					attackerLoss++;
				}

				itAttackRolls++;
				itDefenseRolls++;
			}

			cout << endl << "Player " << *name << " loses " << attackerLoss << " armies in Country " << attackingCountry->name << endl;
			cout << "Player " << *(defendingCountry->player->name) << " loses " << defenderLoss << " armies in Country "; 
			cout << defendingCountry->name << endl;

			// Remove army loss from armies in attacking Country
			addOrRemoveArmies(attackingCountry->id, -1 * attackerLoss);
			cout << "There are now " << attackingCountry->armies << " armies in Country " << attackingCountry->name << endl;

			// If all armies in defending Country are defeated, attacking Player conquers the defending Country
			if (defendingCountry->armies - defenderLoss <= 0) {
				cout << endl << "Player " << *name << " defeated all of Player " << *(defendingCountry->player->name); 
				cout << "'s armies in Country " << defendingCountry->name << "!" << endl;
				cout << "Country " << defendingCountry->name << " now belongs to Player " << *name << endl;

				// Calculate how many armies to move in newly conquered Country
				int numArmiesToMove = selectArmiesToMoveAfterAttackSuccess(attackingCountry, defendingCountry, numAttackDice);

				cout << endl << "Player " << *name << " will move " << numArmiesToMove << " armies in Country "; 
				cout << defendingCountry->name << endl;

				// Change the defending Country's ownership and move armies in it
				defendingCountry->player->loseCountry(defendingCountry->id);
				claimCountry(defendingCountry, numArmiesToMove);
				addOrRemoveArmies(attackingCountry->id, -1 * numArmiesToMove);

				cout << "There are now " << attackingCountry->armies << " armies in Country " << attackingCountry->name << endl;
				cout << endl;

				atLeastOneCountryConquered = true;
			}
			else {
				// Remove army loss from defending Country's armies
				defendingCountry->player->addOrRemoveArmies(defendingCountry->id, -1 * defenderLoss);
				cout << "There are now " << defendingCountry->armies << " armies in Country " << defendingCountry->name << endl;
				cout << endl;
			}
		}

		cout << endl << "Player " << *name << " chose to stop attacking" << endl << endl;

		if (atLeastOneCountryConquered) {
			//TODO in A3: check if Player defeated other Player. Transfer cards and impose trading if necessary.
		}
	}
	else {
		cout << endl << "Player " << *name << " chose not to attack." << endl << endl;
	}
}

//move a number of armies from an owned country to another owned neighboring country
void Player::fortify()
{
	cout << "**Fortification Phase**\n";
	srand((int)time(0));
	cout << "Will search for a valid source country among this player's owned countries" << endl;
	Country* source;
	Country* destination;
	map<int, Country*>::iterator it = ownedCountries->begin();
	bool validSource = false;
	do {
		if (it == ownedCountries->end()) {
			cout << "No valid source country was found among this player's owned countries. Exiting fortify()" << endl;
			return;
		}
		source = it->second;
		destination = getFortifyDestination(source);
		if (source->armies <= 1) {
			cout << "Choice of source country with id " << source->id << " failed because it has less than 2 armies on it." << endl;
			it++;
		}
		else if (destination == nullptr) {
			cout << "Choice of source country with id " << source->id << " failed because it has no neighbors owned by the player." << endl;
			it++;
		}
		else
			validSource = true;
	} while (!validSource);

	int srcArmies = source->armies;
	cout << "A valid source country was found with enough armies and an owned neighbor." << endl;
	cout << "Source ID: " << source->id << "  //  Armies: " << srcArmies << endl;
	cout << "Destination ID: " << destination->id << endl;
	cout << "A number of armies to move will be chosen at random in the range from 1 to " << (srcArmies - 1) << endl;
	int armiesToMove = (rand() % (srcArmies -1)) + 1;
	cout << armiesToMove << " armies will be moved." << endl;

	try {
		addOrRemoveArmies(source->id, -armiesToMove);
		addOrRemoveArmies(destination->id, armiesToMove);
		cout << "Fortification has successfully completed." << endl;
	}
	catch (invalid_argument e) {
		cout << "Exception thrown when trying to add/remove armies:" << endl;
		cout << e.what() << endl;
	}
}

/*	Get armies from total countries divided by 3, removing fractions.
	@return army reinforcement from owned country count
*/
int Player::getCountryReinforcement() {

	int reinforcements = (ownedCountries->size() / 3);
	if (reinforcements < 3)
		reinforcements = 3;	// 3 being minimum

	cout << "\nArmies from countries: " << reinforcements;	// TODO remove after testing driver
	return reinforcements;
}

/*	Get armies from owning any continent.
	@return army reinforcement from owned continent
*/
int Player::getContinentReinforcement() {
	
	int reinforcements = 0;
	list<Continent> allContinents = mapPtr->getContinents();

	for(Continent c : allContinents) {
		if(c.getSize() == (*numOfOwnedCountriesPerContinent)[c.getId()])
				{ reinforcements += c.getWorth(); }
	}
	cout << "\nArmies from continents: " << reinforcements;	// TODO remove after testing driver
	return reinforcements;
}

/*	Prompt user to choose which countries to distribute their reinforcement armies
	@param int: total armies from reinforcement to distribute
	@param bool: if called at startup or not. If so, player can place only 1 army
*/
void Player::distributeArmies(int totalArmies, bool startup) {

	int countryInput, armiesInput, i = 0;

	displayOwnedCountries();
	while(totalArmies > 0) 
	{
		cout << "---\nYou have " << totalArmies << " armies" << (i++? " left.\n" : ". Deploy your armies!\n"); // ++ to show different msg the second time
		try {
			// prompt choose country to add armies
			cout << "Country: ";
			countryInput = promptNumberInput();
			if(ownedCountries->count(countryInput) == 0)
				throw "\nYou do not own this country.";
			// prompt # of armies to place in that chosen country
			if (startup)
				armiesInput = 1;
			else {
				cout << "Armies: ";
				armiesInput = promptNumberInput();
				if (armiesInput > totalArmies || armiesInput < 0)
					throw ("\nYou must enter armies in the range given.");
			}
			
			addOrRemoveArmies(countryInput, armiesInput);
			totalArmies -= armiesInput;
			if (startup)
				break;
		}
		catch(const char* msg) {
			cout << msg << endl;
		}
	}
	displayOwnedCountries();
}

//returns the first neighbor of source which is found to be owned by the player, nullptr if there is no such neighbor
Country* Player::getFortifyDestination(Country* source) {
	for (int neighbor : source->neighbors) {
		if (ownedCountries->count(neighbor) == 1) //if the neighbor is owned by the player
			return ownedCountries->at(neighbor);  //return the first owned neighbor found
	}
	return nullptr;
}

/*	Armies can be a +ve or -ve integer, meaning add or remove this many armies.
	@exception throws if country is not owned or if the number of armies to remove >= current number of armies.
*/
void Player::addOrRemoveArmies(int countryId, int armies)
{
	if (ownedCountries->count(countryId) == 0)
		throw invalid_argument("Could not add/remove armies because the country with id " + to_string(countryId) + " is not owned by Player " + to_string(*id));
	else {
		int currentArmies = (*ownedCountries)[countryId]->armies;
		if (currentArmies + armies < 1)
			throw invalid_argument("Could not remove " + to_string(abs(armies)) + " armies from country with id " + to_string(countryId) + " because it currently has " + to_string(currentArmies) + " armies.");
		else
			(*ownedCountries)[countryId]->armies = currentArmies + armies;
	}
}

/*	Prompt user to input a NUMBER only.
	@return valid number input
*/
int Player::promptNumberInput() {

	int n;
	do {
		cin >> n;
		if (!cin.good()) {	/// !good() when input doesnt match declared type
			cout << "\nInvalid number input. Please try again.\n";
			cin.clear();		   /// clear error flag
			cin.ignore(100, '\n'); /// clear buffer
		}
		else break;
	} while (true);

	return n;
}

/*	Display owned countries' armies and continent id.
*/
void Player::displayOwnedCountries() {

	cout << "\nOwned Countries   Armies  Continent\n"
		<< "====================================";
	map<int, Country*>::iterator iter = ownedCountries->begin();
	for (; iter != ownedCountries->end(); ++iter)
	{
		cout << "\nCountry " << iter->first << " \t|| " << iter->second->armies << "\t|| " << iter->second->continentId;
	}
	cout << endl;
}
