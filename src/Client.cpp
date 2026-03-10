#define NOMINMAX
#include <iostream>
#include <string>
#include <limits>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

std::string sendToServer(const std::string& message) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return "ERROR: Could not connect to server.";
    }

    send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
    
    char buffer[4096] = {0}; // Increased buffer to catch large statistic reports
    recv(clientSocket, buffer, 4096, 0);
    
    closesocket(clientSocket);
    WSACleanup();
    
    return std::string(buffer);
}

int getValidIntInput() {
    int input;
    while (!(std::cin >> input)) {
        std::cin.clear(); 
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
        std::cout << "Invalid input. Please enter a valid number: ";
    }
    return input;
}

// --- RESTORED ADMIN MENU ---
void adminMenu() {
    int choice;
    while (true) {
        std::cout << "\n--- Remote Admin Panel ---\n";
        std::cout << "1. Add Candidate\n";
        std::cout << "2. Remove Candidate\n";
        std::cout << "3. View Election Statistics\n";
        std::cout << "4. View Graphical Results\n";
        std::cout << "5. DANGER: Reset Entire Election\n";
        std::cout << "6. Return to Main Menu\n";
        std::cout << "Enter choice: ";
        
        choice = getValidIntInput();

        if (choice == 1) {
            std::string name;
            std::cout << "Enter candidate name: ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, name);
            std::cout << "Server: " << sendToServer("ADMIN|ADD|" + name) << "\n";
        } else if (choice == 2) {
            std::string name;
            std::cout << "Enter candidate name: ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, name);
            std::cout << "Server: " << sendToServer("ADMIN|REMOVE|" + name) << "\n";
        } else if (choice == 3) {
            std::cout << sendToServer("ADMIN|STATS") << "\n";
        } else if (choice == 4) {
            std::cout << sendToServer("ADMIN|GRAPHICS") << "\n";
        } else if (choice == 5) {
            std::cout << "Are you absolutely sure? This will delete all current votes! (y/n): ";
            char confirm;
            std::cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                std::cout << "Server: " << sendToServer("ADMIN|RESET") << "\n";
            }
        } else if (choice == 6) {
            break;
        } else {
            std::cout << "Invalid choice.\n";
        }
    }
}
// ---------------------------

int main() {
    int choice;
    while (true) {
        std::cout << "\n--- Voter Terminal (Network Client) ---\n";
        std::cout << "1. Register\n";
        std::cout << "2. Login and Vote\n";
        std::cout << "3. Admin Panel\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter choice: ";
        choice = getValidIntInput();

        if (choice == 1) {
            std::string pass;
            std::cout << "Create a Password: ";
            std::cin >> pass;
            std::string response = sendToServer("REGISTER|" + pass);
            std::cout << "Server Response: " << response << "\n";
        } 
        else if (choice == 2) {
            std::string id, pass, candidate;
            std::cout << "Enter Voter ID: ";
            std::cin >> id;
            std::cout << "Enter Password: ";
            std::cin >> pass;
            std::cout << "Enter Candidate Exact Name: ";
            std::cin.ignore();
            std::getline(std::cin, candidate);

            std::string payload = "VOTE|" + id + "|" + pass + "|" + candidate;
            std::string response = sendToServer(payload);
            std::cout << "Server Response: " << response << "\n";
        } 
        else if (choice == 3) {
            adminMenu();
        }
        else if (choice == 4) {
            std::cout << "Closing connection...\n";
            break;
        }
    }
    return 0;
}