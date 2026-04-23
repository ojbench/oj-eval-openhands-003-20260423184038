#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemStatus {
    bool solved = false;
    int wrongAttempts = 0;
    int solveTime = 0;
    int frozenSubmissions = 0;
    bool frozen = false;
    vector<Submission> frozenSubs;
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int solvedCount = 0;
    int penaltyTime = 0;
    int ranking = 0;
    vector<int> solveTimes;
    
    void updateStats(const vector<string>& problemList) {
        solvedCount = 0;
        penaltyTime = 0;
        solveTimes.clear();
        
        for (const auto& prob : problemList) {
            auto it = problems.find(prob);
            if (it != problems.end() && it->second.solved && !it->second.frozen) {
                solvedCount++;
                int penalty = 20 * it->second.wrongAttempts + it->second.solveTime;
                penaltyTime += penalty;
                solveTimes.push_back(it->second.solveTime);
            }
        }
        sort(solveTimes.rbegin(), solveTimes.rend());
    }
    
    bool hasFrozenProblems() const {
        for (const auto& p : problems) {
            if (p.second.frozen) return true;
        }
        return false;
    }
    
    string getSmallestFrozenProblem(const vector<string>& problemList) const {
        for (const auto& prob : problemList) {
            auto it = problems.find(prob);
            if (it != problems.end() && it->second.frozen) {
                return prob;
            }
        }
        return "";
    }
};

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> teamOrder;
    bool started = false;
    bool frozen = false;
    int durationTime = 0;
    int problemCount = 0;
    vector<string> problemList;
    vector<string> rankedTeams;
    bool scoreBoardFlushed = false;
    
    bool compareTeams(const Team& a, const Team& b) {
        if (a.solvedCount != b.solvedCount) return a.solvedCount > b.solvedCount;
        if (a.penaltyTime != b.penaltyTime) return a.penaltyTime < b.penaltyTime;
        
        size_t minSize = min(a.solveTimes.size(), b.solveTimes.size());
        for (size_t i = 0; i < minSize; i++) {
            if (a.solveTimes[i] != b.solveTimes[i]) {
                return a.solveTimes[i] < b.solveTimes[i];
            }
        }
        
        return a.name < b.name;
    }
    
    void updateRankings() {
        for (auto& t : teams) {
            t.second.updateStats(problemList);
        }
        
        rankedTeams.clear();
        for (const auto& t : teams) {
            rankedTeams.push_back(t.first);
        }
        
        sort(rankedTeams.begin(), rankedTeams.end(), [this](const string& a, const string& b) {
            return compareTeams(teams[a], teams[b]);
        });
        
        for (size_t i = 0; i < rankedTeams.size(); i++) {
            teams[rankedTeams[i]].ranking = i + 1;
        }
    }
    
    void printScoreboard() {
        for (const auto& teamName : rankedTeams) {
            const Team& t = teams[teamName];
            cout << t.name << " " << t.ranking << " " << t.solvedCount << " " << t.penaltyTime;
            
            for (const auto& prob : problemList) {
                cout << " ";
                auto it = t.problems.find(prob);
                if (it == t.problems.end() || (!it->second.solved && it->second.wrongAttempts == 0 && !it->second.frozen)) {
                    cout << ".";
                } else if (it->second.frozen) {
                    if (it->second.wrongAttempts > 0) {
                        cout << "-" << it->second.wrongAttempts << "/" << it->second.frozenSubmissions;
                    } else {
                        cout << "0/" << it->second.frozenSubmissions;
                    }
                } else if (it->second.solved) {
                    if (it->second.wrongAttempts > 0) {
                        cout << "+" << it->second.wrongAttempts;
                    } else {
                        cout << "+";
                    }
                } else {
                    cout << "-" << it->second.wrongAttempts;
                }
            }
            cout << "\n";
        }
    }
    
public:
    void addTeam(const string& teamName) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.find(teamName) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            Team t;
            t.name = teamName;
            teams[teamName] = t;
            teamOrder.push_back(teamName);
            cout << "[Info]Add successfully.\n";
        }
    }
    
    void startCompetition(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            started = true;
            durationTime = duration;
            problemCount = problems;
            problemList.clear();
            for (int i = 0; i < problems; i++) {
                problemList.push_back(string(1, 'A' + i));
            }
            cout << "[Info]Competition starts.\n";
        }
    }
    
    void submit(const string& problem, const string& teamName, const string& status, int time) {
        Submission sub = {problem, status, time};
        teams[teamName].submissions.push_back(sub);
        
        ProblemStatus& ps = teams[teamName].problems[problem];
        
        if (frozen && !ps.solved) {
            ps.frozenSubmissions++;
            ps.frozen = true;
            ps.frozenSubs.push_back(sub);
        } else if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solveTime = time;
            } else {
                ps.wrongAttempts++;
            }
        }
    }
    
    void flush() {
        updateRankings();
        scoreBoardFlushed = true;
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            frozen = true;
            cout << "[Info]Freeze scoreboard.\n";
        }
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        updateRankings();
        printScoreboard();
        
        while (true) {
            bool found = false;
            for (int i = rankedTeams.size() - 1; i >= 0; i--) {
                Team& team = teams[rankedTeams[i]];
                if (team.hasFrozenProblems()) {
                    string prob = team.getSmallestFrozenProblem(problemList);
                    if (!prob.empty()) {
                        int oldRank = team.ranking;
                        string teamName = team.name;
                        
                        ProblemStatus& ps = team.problems[prob];
                        ps.frozen = false;
                        
                        // Replay frozen submissions
                        for (const auto& sub : ps.frozenSubs) {
                            if (!ps.solved) {
                                if (sub.status == "Accepted") {
                                    ps.solved = true;
                                    ps.solveTime = sub.time;
                                } else {
                                    ps.wrongAttempts++;
                                }
                            }
                        }
                        ps.frozenSubs.clear();
                        ps.frozenSubmissions = 0;
                        
                        // Save the old ranking list before update
                        vector<string> oldRankedTeams = rankedTeams;
                        
                        updateRankings();
                        
                        int newRank = teams[teamName].ranking;
                        if (newRank < oldRank) {
                            // Find the team that was at the NEW position in the OLD ranking
                            string replacedTeam = oldRankedTeams[newRank - 1];
                            cout << teamName << " " << replacedTeam << " " 
                                 << teams[teamName].solvedCount << " " << teams[teamName].penaltyTime << "\n";
                        }
                        
                        found = true;
                        break;
                    }
                }
            }
            if (!found) break;
        }
        
        printScoreboard();
        frozen = false;
    }
    
    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query ranking.\n";
            if (frozen) {
                cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            }
            cout << teamName << " NOW AT RANKING " << teams[teamName].ranking << "\n";
        }
    }
    
    void querySubmission(const string& teamName, const string& problem, const string& status) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query submission.\n";
            
            const vector<Submission>& subs = teams[teamName].submissions;
            Submission* found = nullptr;
            
            for (int i = subs.size() - 1; i >= 0; i--) {
                bool matchProblem = (problem == "ALL" || subs[i].problem == problem);
                bool matchStatus = (status == "ALL" || subs[i].status == status);
                if (matchProblem && matchStatus) {
                    found = const_cast<Submission*>(&subs[i]);
                    break;
                }
            }
            
            if (found) {
                cout << teamName << " " << found->problem << " " << found->status << " " << found->time << "\n";
            } else {
                cout << "Cannot find any submission.\n";
            }
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string teamName;
            iss >> teamName;
            system.addTeam(teamName);
        } else if (cmd == "START") {
            string duration, problem;
            int dur, prob;
            iss >> duration >> dur >> problem >> prob;
            system.startCompetition(dur, prob);
        } else if (cmd == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            system.submit(problem, teamName, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            system.queryRanking(teamName);
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, part1, part2;
            iss >> teamName >> where;
            getline(iss, part1);
            
            size_t probPos = part1.find("PROBLEM=");
            size_t statusPos = part1.find("STATUS=");
            
            string problem, status;
            if (probPos != string::npos && statusPos != string::npos) {
                size_t probStart = probPos + 8;
                size_t probEnd = part1.find(" AND", probStart);
                problem = part1.substr(probStart, probEnd - probStart);
                
                size_t statusStart = statusPos + 7;
                status = part1.substr(statusStart);
            }
            
            system.querySubmission(teamName, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}
