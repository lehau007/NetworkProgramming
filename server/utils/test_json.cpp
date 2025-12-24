#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std;
// Alias for convenience
using json = nlohmann::json;

int main() {
    // 1. Your raw JSON string
    std::string input = R"({
        "name": "Hau",
        "age": 22,
        "scores": [10, 20, 30],
        "active": true,
        "extra": null
    })";

    try {
        // 2. Parse the string into a JSON object
        json j = json::parse(input);

        // 3. Access simple values
        // You can use .get<type>() or implicit casting
        std::string name = j["name"];
        int age = j["age"];
        bool isActive = j["active"];

        std::cout << "Name: " << name << "\n";
        std::cout << "Age: " << age << "\n";
        std::cout << "Is Active: " << (isActive ? "Yes" : "No") << "\n";

        // 4. Process the Array ("scores")
        std::cout << "Scores: ";
        // You can iterate directly like a standard vector
        for (int score : j["scores"]) {
            std::cout << score << " ";
        }
        std::cout << "\n";

        // 5. Handle Null values safely
        if (j["extra"].is_null()) {
            std::cout << "Extra info is NULL (as expected)\n";
        } else {
            std::cout << "Extra info: " << j["extra"] << "\n";
        }

        // 6. Example: Modifying the data
        j["age"] = 23; // Update value
        j["new_field"] = "Created in C++"; // Add new field

        // Dump back to string (4 spaces indentation)
        // std::cout << "\nModified JSON:\n" << j.dump(4) << std::endl;

    } catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return 1;
    } catch (json::type_error& e) {
        std::cerr << "JSON Type Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}