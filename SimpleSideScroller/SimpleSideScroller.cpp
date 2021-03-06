// SimpleSideScroller.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.

#include "obstacle.hpp"

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <thread>
#include <string>
#include <cstdlib>
#include <fstream>

#include <iostream>

class background
{
private:
	sf::Image image;
	sf::Texture texture;
	sf::Sprite sprite;
public:
	background()
		: image()
		, texture()
		, sprite()
	{}
	sf::Image& get_image() { return image; }
	sf::Texture& get_texture() { return texture; }
	sf::Sprite& get_sprite() { return sprite; }
};

class background_tile
{
private:
	sf::Image image;
	sf::Texture texture;
	sf::Sprite sprite;
	int	speed;
public:
	background_tile()
		: image()
		, texture()
		, sprite()
		, speed()
	{}
	sf::Image& get_image() { return image; }
	sf::Texture& get_texture() { return texture; }
	sf::Sprite& get_sprite() { return sprite; }
	int get_speed() { return speed; }
	void set_speed(int new_speed) { speed = new_speed; }
};

void
loop(sf::RenderWindow& window);

void
events(sf::RenderWindow& window, bool& playing, bool& lost_game, float& player_velocity, bool& jump_allowed);

void
update_velocity(float const& gravity, float& player_velocity);

void
create_obstacles(sf::RenderWindow& window, std::vector<std::shared_ptr<obstacle>>& obstacles, unsigned int count, sf::Time& obs_cooldown, sf::Time& min_obs_cooldown, float& ground_height);

void
update_obstacles(sf::RenderWindow& window, std::vector<std::shared_ptr<obstacle>>& obstacles, sf::Clock& obstacle_clock, sf::Time& obs_cooldown, sf::Time& min_obs_cooldown, float& ground_height, unsigned int& score_int, float& delta_time);

void
move_player(sf::RenderWindow& window, std::shared_ptr<sf::RectangleShape>& player, float& player_velocity, float& delta_time, float const& gravity, float& ground_height, bool& jump_allowed);

void
colission_checking(sf::RenderWindow& window, std::vector<std::shared_ptr<obstacle>>& obstacles, std::shared_ptr<sf::RectangleShape>& player, float& player_velocity, bool& lost_game, bool& playing, unsigned int& score_int, float& ground_height, bool& jump_allowed);

void
write_highscore(unsigned int new_highscore);

unsigned int
read_highscore();

int
main() // no parameters needed
{
	// create window
	unsigned int width = 960; // for creating the window with n width
	unsigned int height = 540; // for creating the window with n height
	sf::RenderWindow window(sf::VideoMode(width, height, 32), "3S - Small Side Scroller"); // creating the window with video mode (width, height, color spectrum) and title
	// set cursor not visible
	window.setMouseCursorVisible(false); // for not getting distracted while playing
	// set frame limitations
	window.setFramerateLimit(60); // for better frame rate control
	window.setVerticalSyncEnabled(true); // for better frame rate control

	// isOpen loop
	loop(window); // main game loop in here --> to seperate window creation from game loop

    return 0;
}

void
loop(sf::RenderWindow& window)
{
	// TODO: rework into classes
	// random seed
	srand(time(NULL)); // for generating different sized obstacles

	// set various boolean values
	bool playing = false; // to determine if the user is still playing
	bool lost_game = false; // to determine if the user has lost the game
	bool jumping = false; // to determine if the player is jumping
	bool jump_allowed = false; // to determine if the player is allowed to jump

	// set constant values
	float const gravity = 50.0; // force of gravity pulling on the jumping character/player

	// declare various values
	float player_velocity = 0.0; // for player movement --> jumping
	float ground_height = 50; // height of the drawn ground at the windows bottom

	// declare clocks
	sf::Clock main_clock; // for delta_time --> cpu/gpu indepentent movement
	sf::Clock jump_clock; // for jumping every n seconds
	sf::Clock obstacle_clock; // for creating obstacles every n seconds

	// declare time values
	float delta_time = 0.0;
	sf::Time jump_cooldown = sf::seconds(0.5); // for jumping every n seconds
	sf::Time obs_cooldown = sf::seconds(1.4); // for creating obstacles every n seconds
	sf::Time min_obs_cooldown = sf::seconds(0.6); // for creating obstacles at minimum every n seconds

	// create ground
	auto ground = std::make_shared<sf::RectangleShape>(); // as pointer for later --> maybe put it in a drawing vector later
	ground->setPosition(sf::Vector2f(0, window.getSize().y - ground_height)); // from the windows bottom ground_height high
	ground->setFillColor(sf::Color::White); // so the user is able to identify it faster
	ground->setOutlineThickness(1); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	ground->setOutlineColor(sf::Color::Black); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	ground->setSize(sf::Vector2f(window.getSize().x, ground_height)); // as wide as the window and ground_height high

	// load background texture
	auto back_ground = std::make_shared<background>();
	back_ground->get_image().loadFromFile(".\\Resources\\Images\\Background\\city.png");
	back_ground->get_texture().loadFromImage(back_ground->get_image());
	back_ground->get_texture().setSmooth(true);
	back_ground->get_sprite().setPosition(sf::Vector2f(0, ground_height - 15)); // as big as the window
	back_ground->get_sprite().setTexture(back_ground->get_texture());

	// continuious background
	std::vector<std::shared_ptr<background_tile>> background;
	int maximum_tiles = 10;
	int last_tiles_x = 0;
	for (unsigned int i = 0; i < maximum_tiles; i++)
	{
		auto tile = std::make_shared<background_tile>();
		tile->get_image().loadFromFile(".\\Resources\\Images\\Background\\house_" + std::to_string(rand() % 3 + 1) + ".png");
		tile->get_texture().loadFromImage(tile->get_image());
		tile->get_texture().setSmooth(true);
		tile->get_sprite().setTexture(tile->get_texture());
		tile->get_sprite().setPosition(sf::Vector2f(rand()% window.getSize().x + tile->get_sprite().getGlobalBounds().width, 35));
		tile->set_speed(rand()% 150 + 25);
		background.push_back(std::move(tile));
	}

	// create player
	auto player = std::make_shared<sf::RectangleShape>(); // as pointer for later --> maybe put it in a drawing vector later
	player->setSize(sf::Vector2f(20.0, 20.0));
	player->setPosition(sf::Vector2f(35.0, window.getSize().y - player->getOutlineThickness() - player->getGlobalBounds().height)); // set it on top of the ground
	player->setFillColor(sf::Color::Red); // for identifying it easily
	player->setOutlineThickness(1); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	player->setOutlineColor(sf::Color::Black); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)

	// create font
	sf::Font font; // for loading and setting the text fonts
	font.loadFromFile(".\\Resources\\Fonts\\Exo-Medium.otf"); // just a sample free to use font

	// create menu text
	sf::Text accept_start; // "start menu" text that encourages the user to press space to begin
	accept_start.setPosition(sf::Vector2f(10.0, 10.0)); // just at the top left
	accept_start.setString("Start by pressing space!"); // text that encourages the user to press space to begin
	accept_start.setOutlineThickness(1); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	accept_start.setOutlineColor(sf::Color::Black); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	accept_start.setCharacterSize(50); // make it big so it can be read/seen easier by the user
	accept_start.setFillColor(sf::Color::White); // make it white (with black outline) so it can be read/seen easier by the user
	accept_start.setFont(font); // setting the font is required for actually seeing it..

	// create looser text
	sf::Text lost; // "looser" text that informs the user that he has lost
	lost.setPosition(sf::Vector2f(10.0, 10.0)); // just at the top left
	lost.setOutlineThickness(1); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	lost.setOutlineColor(sf::Color::Black); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	lost.setCharacterSize(50); // make it big so it can be read/seen easier by the user
	lost.setFillColor(sf::Color::White); // make it white (with black outline) so it can be read/seen easier by the user
	lost.setFont(font); // setting the font is required for actually seeing it..

	// create score text
	unsigned int score_int = 0; //  the current score (obstacles passed)
	sf::Text score; // "score" text that shows the current score (obstacles passed) to the user
	score.setPosition(sf::Vector2f(10.0, 10.0)); // just at the top left
	score.setString(std::to_string(score_int)); // text that shows the current score (obstacles passed) to the user
	score.setOutlineThickness(1); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	score.setOutlineColor(sf::Color::Black); // everything can be read/seen easier with a black outline (black text excluded for it doesn't matter)
	score.setCharacterSize(50); // make it big so it can be read/seen easier by the user
	score.setFillColor(sf::Color::White); // make it white (with black outline) so it can be read/seen easier by the user
	score.setFont(font); // setting the font is required for actually seeing it..

	// create obstacle vector
	std::vector<std::shared_ptr<obstacle>> obstacles; // so the game is able to create multiple obstacles

	// restart timers before game loop starts to prevent false measurements
	obstacle_clock.restart(); // to prevent false measurements regarding obstacle creation
	jump_clock.restart(); // to prevent false measurements regarding the player jumping
	main_clock.restart(); // to prevent false measurements regarding the overall movement speeds

	unsigned int highscore = read_highscore();
	unsigned int tmp_score = 0; // for displaying score in lost screen

	// game loop
	while (window.isOpen()) // basic sfml game loop checking
	{
		// event checking
		events(window, playing, lost_game, player_velocity, jump_allowed); // check events outsourced because it is an ugly mess..

		if (playing) // game is running
		{
			// update obstacles
			update_obstacles(window, obstacles, obstacle_clock, obs_cooldown, min_obs_cooldown, ground_height, score_int, delta_time);
			// move player
			move_player(window, player, player_velocity, delta_time, gravity, ground_height, jump_allowed);
			// update score -> has to be before colission checking for the score gets reset in there
			score.setString("Score: " + std::to_string(score_int) + "\nHighscore: " + std::to_string(highscore));
			// colission checking
			colission_checking(window, obstacles, player, player_velocity, lost_game, playing, score_int, ground_height, jump_allowed);
			// move background tiles
			for (unsigned int i = 0; i < background.size(); i++)
			{
				background.at(i)->get_sprite().move((background.at(i)->get_speed() * (-1)) * delta_time, 0);
				// reset background tiles
				if (background.at(i)->get_sprite().getPosition().x + background.at(i)->get_sprite().getGlobalBounds().width <= 0)
				{
					background.at(i)->get_sprite().setPosition(window.getSize().x, 35);
				}
			}
			tmp_score = 0;
		}

		if (lost_game) // if lost --> erase all obstacles
		{
			obstacles.erase(obstacles.begin(), obstacles.end()); // all obstacles need to be deleted when the game is lost
			back_ground->get_sprite().setPosition(0, 0); // reset background to starting position when the game is lost
		}

		// update delta_time
		sf::Time tmp = main_clock.restart(); // for correct movement speeds
		delta_time = tmp.asSeconds(); // for correct movement speeds

		// draw the window and its' content
		window.setActive(true); // for opengl --> just do it like this to prevent opengl errors
		window.clear(sf::Color::Black); // so there are no white, but black, flashes --> more pleasant if(!) it happens --> it should not..
		if (!playing) // check if game is running or not --> program running cannot be checked for it is irrelevent here
		{
			// game is not running
			if (!lost_game) // check if game is lost or not
			{
				// game is not lost
				window.draw(accept_start); // show start message to encourage user to play (again)
			}
			else
			{
				// game is lost
				if (score_int != 0)
				{
					tmp_score = score_int;
				}

				if (score_int == 0) // to get a more personalized end message
				{
					// score was = 0
					highscore = read_highscore();
					lost.setString("You lost!\n\nScore: " + std::to_string(tmp_score) + "\nHighscore: " + std::to_string(highscore) + "\nRestart by pressing space"); // score.getString() only works because it isn't set to "" after colission checking
				}
				else
				{
					// score was > 0
					if (score_int > highscore)
					{
						write_highscore(score_int);
					}
					highscore = read_highscore();
					lost.setString("Good game!\n\nScore: " + std::to_string(tmp_score) + "\nHighscore: " + std::to_string(highscore) + "\nRestart by pressing space"); // score.getString() only works because it isn't set to "" after colission checking
				}
				window.draw(lost); // draw lost message and maybe encourage user to play again
				score_int = 0; // reset score
			}
		}
		else
		{
			// game is running
			window.draw(back_ground->get_sprite()); // dereferencing because ground is a pointer
			for (unsigned int i = 0; i < background.size(); i++)
			{
				window.draw(background.at(i)->get_sprite());
			}

			window.draw(*ground); // dereferencing because ground is a pointer

			for (unsigned int i = 0; i < obstacles.size(); i++) // all particles have to be drawn
			{
				window.draw(obstacles.at(i)->get_rect()); // draw the obstaces' sf::RectangleShape
			}

			window.draw(*player); // dereferencing because player is a pointer

			window.draw(score); // draw the users' current score
		}
		window.display(); // display everything that just got drawn
		window.setActive(false); // for opengl --> just do it like this to prevent opengl errors
	}
}

void
write_highscore(unsigned int new_highscore)
{
	std::ofstream score;
	score.open("highscore.txt", std::ios::out);
	score << std::to_string(new_highscore);
	score.close();
}

unsigned int
read_highscore()
{
	unsigned int highscore = 0;

	std::string line;
	std::ifstream score("highscore.txt");
	if (score.is_open())
	{
		getline(score, line); // get only first line
		try
		{
			highscore = std::stoi(line);
		}
		catch (...)
		{
			std::cout << "Highscore could not be read!" << std::endl;
			highscore = 0;
		}
		score.close();
	}

	return highscore;
}

void
colission_checking(sf::RenderWindow& window, std::vector<std::shared_ptr<obstacle>>& obstacles, std::shared_ptr<sf::RectangleShape>& player, float& player_velocity, bool& lost_game, bool& playing, unsigned int& score_int, float& ground_height, bool& jump_allowed)
{
	for (unsigned int i = 0; i < obstacles.size(); i++) // all obstacles need to be checked against player
	{
		if (obstacles.at(i)->get_rect().getGlobalBounds().intersects(player->getGlobalBounds())) // check if bounds match up
		{
			if (player->getPosition().y - player->getSize().y < obstacles.at(i)->get_rect().getPosition().y) // y of player - y of player size = on top or left
			{
				// top or left --> top == no colission --> left == colission
				if (player->getPosition().y < obstacles.at(i)->get_rect().getPosition().y)
				{
					// on top of obstacle --> no colission
					player->setPosition(35.0, obstacles.at(i)->get_rect().getPosition().y - player->getSize().y - 2); // set player' on top of obstacle --> + 2 for outline thickness
					jump_allowed = true;
					if (!obstacles.at(i)->get_touched())
					{
						score_int++; // increase the users' score if the obstacle was jumped on
					}
					obstacles.at(i)->set_touched();
					player_velocity = 0.0;
				}
				else
				{
					// on left, right or down side of obstacle --> colission
					lost_game = true; // user lost the game if they do
					playing = false; // user isn't playing anymore if the game is lost
					player_velocity = 0.0; // reset velocity
					player->setPosition(35.0, window.getSize().y - 2 - player->getSize().y - ground_height); // reset players' position
					return;
				}
			}
			else
			{ // not on top or left obstacle --> colission
				lost_game = true; // user lost the game if they do
				playing = false; // user isn't playing anymore if the game is lost
				player_velocity = 0.0; // reset velocity
				player->setPosition(35.0, window.getSize().y - 2 - player->getSize().y - ground_height); // reset players' position
				return;
			} // right side of obstacle is not possible and should never occur
		}
	}
}

void
move_player(sf::RenderWindow& window, std::shared_ptr<sf::RectangleShape>& player, float& player_velocity, float& delta_time, float const& gravity, float& ground_height, bool& jump_allowed)
{
	if ((player->getPosition().y <= window.getSize().y - 2 - ground_height - player->getSize().y) && !(player->getPosition().y + player_velocity * delta_time > window.getSize().y - 2 - ground_height - player->getSize().y)) // only set position if the player is not "under" the ground or not going to be with the next step --> - 2 = outlineThickness for player and ground --> - 20 is the player size
	{
		update_velocity(gravity, player_velocity); // update players velocity
		player->move(0, player_velocity * delta_time); // move player in the window
	}
	else
	{
		player_velocity = 0.0; // reset players' velocity
		player->setPosition(35.0, window.getSize().y - 2 - ground_height - player->getSize().y); // reset players' position
		jump_allowed = true;
	}
}

void
update_obstacles(sf::RenderWindow& window, std::vector<std::shared_ptr<obstacle>>& obstacles, sf::Clock& obstacle_clock, sf::Time& obs_cooldown, sf::Time& min_obs_cooldown, float& ground_height, unsigned int& score_int, float& delta_time)
{
	// create new particles after elapsed obstacle_cooldown > obs_cooldown
	sf::Time elapsed = obstacle_clock.getElapsedTime(); // get elapsed time since last obstacle was created
	if (elapsed.asSeconds() > obs_cooldown.asSeconds()) // only create an obstacle after the cooldown has passed
	{
		create_obstacles(window, obstacles, 1, obs_cooldown, min_obs_cooldown, ground_height); // create a new obstacle
		obstacle_clock.restart(); // restart the clock so the cooldown starts anew
	}

	// update all obstacles
	for (unsigned int i = 0; i < obstacles.size(); i++) // has to update all obstacles
	{
		if (obstacles.at(i)->is_alive()) // only update if it's alive
		{
			if (obstacles.at(i)->get_rect().getPosition().x < 0 - obstacles.at(i)->get_rect().getSize().x) // only set obstacle dead if the obstacle passed outside the window
			{
				obstacles.at(i)->set_dead(); // set obstacles' dead/alive flag --> erase them at next iteration
			}
			else
			{
				obstacles.at(i)->get_rect().move(-350 * delta_time, 0); // only move obstacle if it's alive --> every obstacle has to move
			}
		}
		else
		{
			obstacles.erase(obstacles.begin() + i); // delete obstacle if it's dead
		}
	}
}

void
create_obstacles(sf::RenderWindow& window, std::vector<std::shared_ptr<obstacle>>& obstacles, unsigned int count, sf::Time& obs_cooldown, sf::Time& min_obs_cooldown, float& ground_height)
{
	for (unsigned int i = 0; i < count; i++) // create an obstacle count times
	{
		auto obs = std::make_shared<obstacle>(); // create a new obstacle
		obs->get_rect().setPosition(sf::Vector2f(window.getSize().x, window.getSize().y - ground_height - 2 /* outline thickness of ground and obstacle */ - obs->get_rect().getSize().y - rand() % 200 + 0)); // set the obstacles position relative to the window size and ground height
		obstacles.push_back(obs); // add the new obstacle 
	}
	sf::Time tmp_time = sf::seconds(0.1); // set a time to be able to decrease the cooldown
	if (obs_cooldown - tmp_time > min_obs_cooldown) // only decrease the cooldown if it's decreasable --> it has to stay greater than minimum cooldown
	{
		obs_cooldown -= tmp_time; // decrease cooldown for obstacle creation --> decrease the cooldown for faster obstacle creation
	}
}

void
update_velocity(float const& gravity, float& player_velocity)
{
	player_velocity += gravity; // update players' velocity by adding the games' gravity
}

void
events(sf::RenderWindow& window, bool& playing, bool& lost_game, float& player_velocity, bool& jump_allowed)
{
	sf::Event event; // create an SFML event
	while (window.pollEvent(event)) // only do something if an event is triggered
	{
		switch (event.type) // check which event type
		{
		default: // default case
			break; // break the switch
		case sf::Event::Closed: // if event is the window closed event
			window.close(); // close window when close event is activated
		case sf::Event::LostFocus: // if event is the focus is lost from the window
			playing = false;  // set playing to false --> so the main loop doesn't execute critical functions
			window.setTitle("3S - Small Side Scroller - PAUSED"); // set the title so the user can faster identify the games' status
			break;
		case sf::Event::GainedFocus: // if event is the focus is gained from the window
			playing = true;  // set playing to true --> so the main loop doesn't execute critical functions
			window.setTitle("3S - Small Side Scroller"); // set the title so the user can faster identify the games' status
			break;
		case sf::Event::Resized: // if event is the window is resized
			sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height); // create a SFML view and set it to the actual seen scene
			window.setView(sf::View(visibleArea)); // update the windows' view with the newly created one --> resizes every viewable object
			break;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) // only do something if space is pressed
	{
		if (playing) // check if playing or not
		{
			if (jump_allowed) // normal jump
			{
				jump_allowed = false;
				player_velocity = -950; // set players' velicity to jump velocity
			}

		}
		else
		{
			playing = true; // set playing to true --> so the main loop doesn't execute critical functions
			lost_game = false; // set lost_game to not lost
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) // only do something if escape is pressed
	{
		playing = false; // set playing to false --> so the main loop doesn't execute critical functions
	}
}
