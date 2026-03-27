#include <iostream>
#include <string>

using namespace std;

// Mocking FastBot2 structure to see output
class Keyboard {
    string _kb = "";
public:
    void addButton(string text) {
        _kb += "{\"text\":\"" + text + "\"}";
    }
    void addButtonRaw(string raw) {
        _kb += raw;
    }
    void newRow() {
        _kb += "],[";
    }
    string toJson() {
        return "[[" + _kb + "]]";
    }
};

int main() {
    Keyboard settingsMenu;

    string waBtn = "{\"text\":\"App\",\"web_app\":{\"url\":\"http://test\"}}";
    settingsMenu.addButtonRaw(waBtn);

    string items[] = {"A", "B", "C", "D", "E", "F", "G"};
    int i = 0;
    for (const auto& item : items) {
        if (i % 2 == 0) settingsMenu.newRow();
        settingsMenu.addButton(item);
        i++;
    }

    cout << settingsMenu.toJson() << endl;
    return 0;
}
