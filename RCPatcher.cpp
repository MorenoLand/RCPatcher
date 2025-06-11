#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <csignal>
using namespace std;
void signalHandler(int signum) {
    cout << "\n[!] Interrupted by user. Exiting..." << endl;
    exit(0);
}
void banner() {
    cout << "\n###############################################################################\n";
    cout << "#    ######   ###### #######   #####  ####### ###### ##   ## ####### ######     #\n";
    cout << "#    ##   ##  ##     ##   ##  ##   ##    ##   ##     ##   ## ##      ##   ##    #\n";
    cout << "#    ######   ##     #######  #######    ##   ##     ####### #####   ######     #\n";
    cout << "#    ##   ##  ##     ##       ##   ##    ##   ##     ##   ## ##      ##   ##    #\n";
    cout << "#    ##   ##  ###### ##       ##   ##    ##   ###### ##   ## ####### ##   ##    #\n";
    cout << "#################################################################################\n\n";
}
bool findSignature(const vector<char>& data, const string& sig) {
    for (size_t i = 0; i <= data.size() - sig.length(); i++) {
        if (memcmp(&data[i], sig.c_str(), sig.length()) == 0) return true;
    }
    return false;
}
void patchString(vector<char>& data, size_t offset, const string& newStr, size_t maxLen) {
    if (offset + maxLen > data.size()) return;
    memset(&data[offset], 0, maxLen);
    memcpy(&data[offset], newStr.c_str(), min(newStr.length(), maxLen));
}
int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    banner();
    string filename;
    if (argc == 2) filename = argv[1];
    else if (argc == 1) {
        cout << "[?] Enter target executable path: ";
        getline(cin, filename);
        if (filename.empty()) { cout << "[!] No file specified. Exiting..." << endl; return 0; }
    }else {
        cout << "[-] Usage: " << argv[0] << " <target.exe>" << endl;
        return 1;
    }
    cout << "[*] Loading target executable: " << filename << endl;
    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "[-] ERROR: Cannot open " << filename << "\n[*] Press any key to exit...";
        cin.ignore(); cin.get(); return 1;
    }
    vector<char> data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    cout << "[*] File loaded (" << data.size() << " bytes)\n[*] Verifying target signature..." << endl;
    if (!findSignature(data, "GSERV025") && !findSignature(data, "RemoteControl")) {
        cout << "[-] SIGNATURE MISMATCH! Wrong executable.\n[*] Press any key to exit...";
        cin.ignore(); cin.get(); return 1;
    }
    cout << "[+] Target verified! Proceeding with patch..." << endl;
    string newServer, newPort;
    cout << "\n[?] Enter new server: "; cin >> newServer;
    cout << "[?] Enter new port: "; cin >> newPort;

    cout << "\n[*] Patching offsets..." << endl;
    const size_t serverOffset = 0xFB5D1, portOffset = 0xFB5EC;
    if (serverOffset < data.size()) {
        patchString(data, serverOffset, newServer, 25);
        cout << "[+] Server patched at 0x" << hex << uppercase << serverOffset << endl;
    }
    if (portOffset < data.size()) {
        patchString(data, portOffset, newPort, 8);
        cout << "[+] Port patched at 0x" << hex << uppercase << portOffset << endl;
    }
    cout << "\n[*] Writing patched executable..." << endl;
    size_t lastSlash = filename.find_last_of("/\\");
    string outputName = (lastSlash != string::npos) ? filename.substr(0, lastSlash + 1) + "patched_" + filename.substr(lastSlash + 1) : "patched_" + filename;
    ofstream outFile(outputName, ios::binary);
    if (!outFile) {
        cout << "[-] ERROR: Cannot write output file\n[*] Press any key to exit...";
        cin.ignore(); cin.get(); return 1;
    }
    outFile.write(data.data(), data.size());
    outFile.close();
    cout << "[+] SUCCESS! Patched file: " << outputName << endl;
    cout << "[+] Target: " << newServer << ":" << newPort << endl;
    cout << "\n[*] Press any key to exit...";
    cin.ignore(); cin.get();
    return 0;
}