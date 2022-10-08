#include "yaml.h"
#include <iostream>
#include <fstream>





int main()
{

    YAML::Node config = YAML::LoadFile("config.yaml");

    if (config["first name"]) {
        std::cout << "First name: " << config["first name"].as<std::string>() << "\n";
    }

    if (config["last name"]) {
        std::cout << "Last name: " << config["last name"].as<std::string>() << "\n";
    }

    if (config["color"]) {
        std::cout << "Coat color: " << config["color"].as<std::string>() << "\n";
    }

//    if (config["lastLogin"]) {
//        std::cout << "Last logged in: " << config["lastLogin"].as<DateTime>() << "\n";
//    }

//    const std::string username = config["username"].as<std::string>();
//    const std::string password = config["password"].as<std::string>();
    //login(username, password);
    config["color"] = "green";

    YAML::Node newconf;

    YAML::Node submap;
    submap["First name"] = "Klaus";
    submap["Last name"] = "Mueller";
    submap["color"] = "violet";

    newconf["Person"].push_back(submap);

    std::ofstream fout("config_new.yaml");
    fout << newconf;
    fout << "\n";

}