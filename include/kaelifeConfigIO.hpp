/**
 * @file kaelifeConfigIO.hpp
 * 
 * @brief JSON Config parser and MasterConfig struct in ConfigHandler class
 * 
 */

#include <iostream>

/**
 * @brief JSON Config parser and MasterConfig struct in ConfigHandler class
 * 
 * TODO: MasterConfig class would store variables frequently modified and required by various classes that it is easier to pass a lot of variables between classes
 * 
 * The idea
 * @code
 * class InputHandler {
 * private:
 * struct PrivateInputConfig {
 * 	using KeyFunction = std::function<void(CAData&)>;
 * //...
 * };
 * PrivateInputConfig privCfg;
 * 
 * public:
 * struct PublicInputConfig {
 * 	std::atomic<bool> QUIT_FLAG = false;
 * //...
 * };
 * PublicInputConfig publCfg;
 * 
 * Then copy the config in constructor
 * 
 * //constructor
 * InputHandler(MasterConfig masterCfg){
 * 	privCfg = masterCfg.privCfg;
 * 	publCfg = masterCfg.publCfg;
 * 	//...
 * }
 * 
 * 
 * 
 * 
 * 
 * class Configuration {
 * public:
 *     static Configuration& getInstance() {
 *         static Configuration instance;
 *         return instance;
 *     }
 * 
 *     int configValue1;
 *     float configValue2;
 *     // ... other configuration values
 * 
 * private:
 *     Configuration() {
 *         // Initialize configuration values
 *         configValue1 = 42;
 *         configValue2 = 3.14f;
 *     }
 * 
 *     // Avoid accidental copy and assignment
 *     Configuration(const Configuration&) = delete;
 *     Configuration& operator=(const Configuration&) = delete;
 * };
 * 
 * // Usage:
 * int value1 = Configuration::getInstance().configValue1;
 * float value2 = Configuration::getInstance().configValue2;
 * @endcode
*/
class MasterConfig{
	public:
	MasterConfig(const char* configFolder){
		printf(configFolder);
		printf("\n");
	};

};