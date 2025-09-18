#include <iostream>
#include <curl/curl.h>
#include <json/json.h>
#include <string>
#include <ctime>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>


// Global variables to store API keys
std::string alpha_vantage_api_key;
std::string janitorai_api_key;
std::string openai_api_key;

std::map<std::string, std::string> users;

// Function to handle the response from the server
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to read API keys from a file
void load_api_keys_and_logins() {
    std::ifstream file("credentials.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                if (key == "alpha_vantage_key") alpha_vantage_api_key = value;
                else if (key == "janitorai_key") janitorai_api_key = value;
                else if (key == "openai_key") openai_api_key = value;
            }
        }
        file.close();
    } else {
        std::cerr << "Unable to open credentials file!" << std::endl;
    }
}

// Function to call Alpha Vantage API for RSI
void get_rsi(const std::string &symbol, std::string &rsi) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string readBuffer;
        std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=IBM&outputsize=full&apikey=demo" + symbol + "&interval=daily&time_period=14&series_type=close&apikey=" + alpha_vantage_api_key;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        else {
            // Parse JSON response to extract RSI value
            Json::CharReaderBuilder readerBuilder;
            Json::Value jsonData;
            std::istringstream s(readBuffer);
            std::string errs;
            if (Json::parseFromStream(readerBuilder, s, &jsonData, &errs)) {
                const Json::Value &rsiData = jsonData["Technical Analysis: RSI"];
                if (!rsiData.empty()) {
                    rsi = rsiData[0]["RSI"].asString();
                }
            }
        }

        curl_easy_cleanup(curl);
    }
}

// Function to call JanitorAI API
void call_janitor_ai(const std::string &query) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string readBuffer;
        std::string url = "https://api.janitorai.com/characters/b5e5a72d-b355-416d-b294-34d7b7d0f105_character-alice/query";
        std::string data = "query=" + query;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

        // Set headers, including the API key for authentication
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + janitorai_api_key).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set callback function to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        else
            std::cout << "alice: " << readBuffer << std::endl;

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

// Function to call OpenAI API
void call_openai_api(const std::string &query) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        std::string readBuffer;
        std::string url = "https://api.openai.com/v1/engines/davinci-codex/completions";
        std::string data = "{\"prompt\": \"" + query + "\", \"max_tokens\": 100}";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

        // Set headers, including the API key for authentication
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + openai_api_key).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set callback function to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        else
            std::cout << "ChatGPT: " << readBuffer << std::endl;

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

// Function to perform automatic trading based on RSI
void auto_trade(const std::string &account) {
    std::ifstream file("investments.txt");
    std::string line;
    while (std::getline(file, line)) {
        std::string symbol = line;
        std::string rsi;
        get_rsi(symbol, rsi);
        if (!rsi.empty()) {
            float rsi_value = std::stof(rsi);
            if (rsi_value < 30) {
                // Buy logic here
                std::cout << "Buying " << symbol << " for account " << account << std::endl;
                // Implement buy order here
            } else if (rsi_value > 70) {
                // Sell logic here
                std::cout << "Selling " << symbol << " for account " << account << std::endl;
                // Implement sell order here
            }
        }
    }
}

// Function to automatically sell at the end of the day
void auto_sell_end_of_day(const std::string &account) {
    std::ifstream file("investments.txt");
    std::string line;
    while (std::getline(file, line)) {
        std::string symbol = line;
        std::cout << "Selling " << symbol << " for account " << account << " at the end of the day" << std::endl;
        // Implement end-of-day sell order here
    }
}

// Function to test investments and check RSI values
void test_investments() {
    std::ifstream file("investments.txt");
    std::string line;
    while (std::getline(file, line)) {
        std::string symbol = line;
        std::string rsi;
        get_rsi(symbol, rsi);
        if (!rsi.empty()) {
            std::cout << "Symbol: " << symbol << " RSI: " << rsi << std::endl;
        }
    }
}

// Function to check if today is a holiday
void check_holiday() {
    std::map<std::string, std::string> holidays = {
        {"12-25", "Christmas"},
        {"01-01", "New Year's Day"},
        // Add more holidays here
    };

    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string date = std::to_string(1 + ltm->tm_mon) + "-" + std::to_string(ltm->tm_mday);

    if (holidays.find(date) != holidays.end()) {
        std::string holiday = holidays[date];
        std::cout << "Today is " + holiday + ". Have a great day!" << std::endl;
    }
}

// Function to check if today is the user's birthday
void check_birthday(const std::string &user_birthday) {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string today = std::to_string(1 + ltm->tm_mon) + "-" + std::to_string(ltm->tm_mday);

    if (today == user_birthday) {
        std::cout << "Happy Birthday!" << std::endl;
    }
}

// Function to respond to the user
void respond_to_user(const std::string &user_name) {
    if (user_name.empty()) {
        std::cout << "Hello, Operator. my name is Alice how may I assist you today?" << std::endl;
    } else {
        std::cout << "Hello, " + user_name + ". my name is Alice how may I assist you today?" << std::endl;
    }
}

// Function to check if the user is in memory
bool is_user_in_memory(const std::string &user_name) {
    std::map<std::string, std::string> users = {
        {"Alice", "06-07"},
        {"Black", "01-29"}
    };
    return users.find(user_name) != users.end();
}

// Function to get the user's birthday from memory
std::string get_user_birthday(const std::string &user_name) {
    std::map<std::string, std::string> users = {
        {"Alice", "06-07"},
        {"Black", "01-29"}
    };
    return users[user_name];
}

// Function to save users to a file
void save_users_to_file() {
    std::ofstream file("users.txt");
    for (const auto &user : users) {
        file << user.first << " " << user.second << std::endl;
    }
    file.close();
}

// Function to load users from a file
void load_users_from_file() {
    std::ifstream file("users.txt");
    std::string name, birthday;
    while (file >> name >> birthday) {
        users[name] = birthday;
    }
    file.close();
}

// Function to add a new user
void add_user(const std::string &name, const std::string &birthday) {
    users[name] = birthday;
    save_users_to_file();
    std::cout << "User " + name + " added with birthday " + birthday << std::endl;
}

// Function to display the help menu
void display_help() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help                - Display this help message" << std::endl;
    std::cout << "  exit, quit, end or bye - Exit the program" << std::endl;
    std::cout << "  date                - Display the current date" << std::endl;
    std::cout << "  check holiday       - Check if today is a holiday" << std::endl;
    std::cout << "  check birthday      - Check if today is the user's birthday" << std::endl;
    std::cout << "  alice <query>       - Call the JanitorAI character 'Alice' with a query" << std::endl;
    std::cout << "  ChatGPT <query>     - Simulate a response from ChatGPT with a query" << std::endl;
    std::cout << "  add user <name> <birthday> - Add a new user with the specified name and birthday" << std::endl;
    std::cout << "  invest <account>    - Invest using the specified account" << std::endl;
    std::cout << "  test                - Test current investments and RSI values" << std::endl;
}

// Main function tying everything together
int main() {
    load_api_keys_and_logins();
    load_users_from_file();

    std::string user_name;
    std::string user_birthday;

    // Request user input for their name
    std::cout << "Please enter your name: ";
    std::getline(std::cin, user_name);

    // Check if the user is in memory
    if (is_user_in_memory(user_name)) {
        user_birthday = get_user_birthday(user_name);
    } else {
        // Request user input for their birthday if not in memory
        std::cout << "Please enter your birthday (MM-DD): ";
        std::getline(std::cin, user_birthday);
        add_user(user_name, user_birthday);
    }

    // Check for holidays and birthdays
    check_holiday();
    check_birthday(user_birthday);

    // Personalized greeting
    respond_to_user(user_name);

    // Text interface loop
    while (true) {
        std::string input;
        std::cout << "You: ";
        std::getline(std::cin, input);

        if (input == "exit" || input == "quit" || input == "end" || input == "bye") {
            std::cout << "Goodbye!" << std::endl;
            break;
        } else if (input == "date") {
            system("date");
        } else if (input == "check holiday") {
            check_holiday();
        } else if (input == "check birthday") {
            check_birthday(user_birthday);
        } else if (input.substr(0, 5) == "alice") {
            call_janitor_ai(input.substr(6));
        } else if (input.substr(0, 7) == "ChatGPT") {
            call_openai_api(input.substr(8));
        } else if (input.substr(0, 8) == "add user") {
            std::string new_user_name = input.substr(9, input.find(' ', 9) - 9);
            std::string new_user_birthday = input.substr(input.find(' ', 9) + 1);
            add_user(new_user_name, new_user_birthday);
        } else if (input.substr(0, 7) == "invest") {
            std::string account = input.substr(8);
            auto_trade(account);
            auto_sell_end_of_day(account);
        } else if (input == "test") {
            test_investments();
        } else if (input == "help") {
            display_help();
        } else if (input == "what are you") {
            std::cout << "I am Project Alice. I am an AI assistant that helps in data analytics and strategic planning. My name Alice stands for Advanced, Learning, Interactive, Combat, Engram." << std::endl;
        } else {
            std::cout << "Unknown command. Try 'help' for a list of commands." << std::endl;
        }
    }

    return 0;
}
