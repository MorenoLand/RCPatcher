#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstring>
#include <csignal>
using namespace std;
void signalHandler(int signum) {
    cout << "\n[!] Interrupted by user. Exiting..." << endl;
    exit(0);
}
struct ServerInfo {
    string server;
    string port;
    size_t serverOffset;
    size_t portOffset;
    size_t serverMaxLen;
    size_t portMaxLen;
};
struct VersionInfo {
    const char* name;
    size_t serverOffset;
    size_t portOffset;
    size_t serverMaxLen;
    size_t portMaxLen;
};
ServerInfo findServerInfo(vector<char>& data) {
    ServerInfo info = { "", "", 0, 0, 0, 0 };
    const string marker = "Offline";
    const string portMarker1 = "14922";
    const string portMarker2 = "14900";
    for (size_t i = 0; i < data.size() - 100; i++) {
        if (memcmp(&data[i], marker.c_str(), marker.length()) == 0) {
            size_t serverStart = i + marker.length() + 1;
            if (serverStart >= data.size()) continue;
            size_t serverEnd = serverStart;
            while (serverEnd < data.size() && data[serverEnd] != 0 && serverEnd < serverStart + 50) serverEnd++;
            if (serverEnd > serverStart) {
                info.server = string(&data[serverStart], serverEnd - serverStart);
                info.serverOffset = serverStart;
                size_t searchStart = serverEnd;
                while (searchStart < data.size() - 10 && searchStart < serverEnd + 30) {
                    if ((memcmp(&data[searchStart], portMarker1.c_str(), 5) == 0) || (memcmp(&data[searchStart], portMarker2.c_str(), 5) == 0) || (data[searchStart] >= '1' && data[searchStart] <= '9' && data[searchStart + 1] >= '0' && data[searchStart + 1] <= '9')) {
                        info.portOffset = searchStart;
                        size_t portEnd = searchStart;
                        while (portEnd < data.size() && data[portEnd] >= '0' && data[portEnd] <= '9' && portEnd < searchStart + 8) portEnd++;
                        info.port = string(&data[searchStart], portEnd - searchStart);
                        info.serverMaxLen = searchStart - serverStart - 1;
                        info.portMaxLen = 8;
                        cout << "[*] Found server config at offset 0x" << hex << serverStart << dec << endl;
                        cout << "[*] Current: " << info.server << ":" << info.port << endl;
                        cout << "[*] Server field size: " << info.serverMaxLen << " bytes" << endl;
                        return info;
                    }
                    searchStart++;
                }
            }
        }
    }
    return info;
}
VersionInfo detectVersion(size_t fileSize) {
    if (fileSize >= 490000 && fileSize <= 510000) {
        return { "RC2 Windows", 0x72DC0, 0x72DD8, 27, 5 };
    } else if (fileSize >= 1180000 && fileSize <= 1190000) {
        return { "RC2 Linux", 0xE32DB, 0xE32FD, 27, 5 };
    } else if (fileSize >= 1800000) {
        return { "RC3", 0xFB5D1, 0xFB5EC, 27, 5 };
    } else {
        return { "Unknown", 0xE32DB, 0xE32FD, 27, 5 };
    }
}
void patchData(vector<char>& data, const ServerInfo& info, const string& newServer, const string& newPort) {
    if (newServer.length() > info.serverMaxLen) {
        cout << "[!] Warning: Server name too long (" << newServer.length() << " > " << info.serverMaxLen << "), will be truncated" << endl;
    }
    memset(&data[info.serverOffset], 0, info.serverMaxLen);
    memcpy(&data[info.serverOffset], newServer.c_str(), min(newServer.length(), info.serverMaxLen));
    memset(&data[info.portOffset], 0, info.portMaxLen);
    memcpy(&data[info.portOffset], newPort.c_str(), min(newPort.length(), info.portMaxLen));
}
int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    string filename = (argc == 2) ? argv[1] : "";
    if (filename.empty()) {
        cout << "[?] Enter target executable path: ";
        getline(cin, filename);
        if (filename.empty()) {
            cout << "\n[-] No filename provided!" << endl;
            return 1;
        }
    }
    cout << "[*] Loading: " << filename << endl;
    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "[-] Cannot open file!\n[*] Press any key to exit...";
        system("pause");
        return 1;
    }
    vector<char> data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    cout << "[*] Loaded " << data.size() << " bytes" << endl;
    cout << "[*] Scanning for server configuration..." << endl;
    ServerInfo info = findServerInfo(data);
    if (info.serverOffset == 0) {
        cout << "[-] Could not find server configuration!\n";
        cout << "[*] Detecting version for hardcoded offsets..." << endl;
        VersionInfo version = detectVersion(data.size());
        cout << "[*] Detected: " << version.name << endl;
        info.serverOffset = version.serverOffset;
        info.portOffset = version.portOffset;
        info.serverMaxLen = version.serverMaxLen;
        info.portMaxLen = version.portMaxLen;
    }
    string newServer, newPort;
    cout << "\n[?] Enter new server: "; cin >> newServer;
    cout << "[?] Enter new port: "; cin >> newPort;
    cout << "\n[*] Patching..." << endl;
    patchData(data, info, newServer, newPort);
    cout << "[+] Server patched at 0x" << hex << info.serverOffset << dec << endl;
    cout << "[+] Port patched at 0x" << hex << info.portOffset << dec << endl;
    size_t lastSlash = filename.find_last_of("/\\");
    string outputName = (lastSlash != string::npos) ? filename.substr(0, lastSlash + 1) + "patched_" + filename.substr(lastSlash + 1) : "patched_" + filename;
    ofstream outFile(outputName, ios::binary);
    if (!outFile) {
        cout << "[-] Cannot write output!\n[*] Press any key to exit...";
        cin.get(); return 1;
    }
    outFile.write(data.data(), data.size());
    outFile.close();
    cout << "[+] SUCCESS! Saved: " << outputName << endl;
    cout << "[+] New target: " << newServer << ":" << newPort << endl;
    cout << "\n[*] Press any key to exit...";
    cin.ignore(); cin.get();
    return 0;
}