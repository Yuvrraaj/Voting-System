#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "sqlite3.h"

class VotingSystem {
private:
    sqlite3* db;
    
    // --- CHANGED THIS LINE ---
    mutable std::recursive_mutex db_mutex; 
    // -------------------------

    void initializeDatabase();
    void executeSQL(const std::string& sql) const;

public:
    VotingSystem();
    ~VotingSystem();

    void logEvent(const std::string& event) const;

    int registerVoter(const std::string& password);
    bool authenticateVoter(int voterId, const std::string& password);
    bool castVote(int voterId, const std::string& candidateName);
    bool isVotingOpen() const;

    void addCandidate(const std::string& name);
    void removeCandidate(const std::string& name);
    void displayCandidates() const;
    void showElectionStatistics() const;
    void generateGraphicalVisualization() const;
    void resetElection();
};