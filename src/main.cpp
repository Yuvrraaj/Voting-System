#include "VotingSystem.h"
#include <iostream>
#include <limits>

// Helper function to safely read integers and prevent infinite loops
int getValidIntInput() {
    int input;
    while (!(std::cin >> input)) {
        std::cin.clear(); // Clear the error flag
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
        std::cout << "Invalid input. Please enter a valid number: ";
    }
    return input;
}

void adminMenu(VotingSystem& system) {
    int choice;
    while (true) {
        std::cout << "\n--- Secure Admin Panel ---\n";
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
            system.addCandidate(name);
        } else if (choice == 2) {
            std::string name;
            std::cout << "Enter candidate name: ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, name);
            system.removeCandidate(name);
        } else if (choice == 3) {
            system.showElectionStatistics();
        } else if (choice == 4) {
            system.generateGraphicalVisualization();
        } else if (choice == 5) {
            std::cout << "Are you absolutely sure? This will delete all current votes! (y/n): ";
            char confirm;
            std::cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                system.resetElection();
            } else {
                std::cout << "Election reset cancelled.\n";
            }
        } else if (choice == 6) {
            break;
        } else {
            std::cout << "Invalid choice.\n";
        }
    }
}

int main() {
    VotingSystem system;
    int choice;

    while (true) {
        std::cout << "\n--- Enterprise SQL Online Voting System ---\n";
        std::cout << "1. Register (Generates Voter ID)\n";
        std::cout << "2. Login and Vote\n";
        std::cout << "3. Admin Panel\n";
        std::cout << "4. Exit\n";
        std::cout << "Enter choice: ";
        
        choice = getValidIntInput();

        if (choice == 1) {
            std::string pass;
            std::cout << "Create a Password: ";
            std::cin >> pass;
            int newId = system.registerVoter(pass);
            std::cout << "\nSUCCESS! Your unique Voter ID is: " << newId << "\n";
            std::cout << "Please save this ID. You will need it to log in.\n";
        } 
        else if (choice == 2) {
            int id;
            std::string pass;
            std::cout << "Enter Voter ID: ";
            id = getValidIntInput();
            
            std::cout << "Enter Password: ";
            std::cin >> pass;

            if (system.authenticateVoter(id, pass)) {
                std::cout << "\nLogin successful.\n";
                system.displayCandidates();
                
                std::string voteChoice;
                std::cout << "Enter the exact name of the candidate: ";
                
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
                std::getline(std::cin, voteChoice);
                
                system.castVote(id, voteChoice);
            } else {
                std::cout << "Authentication failed.\n";
            }
        } 
        else if (choice == 3) {
            // In a production app, you would require a Master Admin Password here
            adminMenu(system);
        } 
        else if (choice == 4) {
            std::cout << "Safely disconnecting from database and exiting...\n";
            break;
        } else {
            std::cout << "Invalid choice. Please select 1-4.\n";
        }
    }
    return 0;
}