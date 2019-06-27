
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <list>
#include <iostream>  
#include <chrono>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include "headers/door_docker_api.h"
#include "headers/door_docker_api.hpp"

using namespace std;
using namespace std::chrono;

hs_database_t *databaseHS_array[HS_DATABASE_NO];
hs_shared_db_control_t *hs_db_controller;
bridge_door_control_t *bridge_controller;
char *test_signals;
// Simple timing class
class Clock {
public:    
    void start() {
        time_start = high_resolution_clock::now();
    }

    void stop() {
        time_end = high_resolution_clock::now();
    }

    double seconds() const {
        duration<double> time_span = duration_cast<duration<double>>(time_end - time_start);
        return time_span.count();
    }

    double secondsPassed() const {
        duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - time_start);
        return time_span.count();
    }
private:
    //std::chrono::high_resolution_clock::time_point<std::chrono::high_resolution_clock> time_start, time_end;
    high_resolution_clock::time_point time_start;
    high_resolution_clock::time_point time_end;
};
// helper function - see end of file
static void parseFile(const char *filename, vector<string> &patterns,
                      vector<unsigned> &flags, vector<unsigned> &ids, vector<string> &rulestrings);

int main(int argc, char *argv[]) {

    RuleManager rule_manager;

  int max_rules = 1500;
    string temp = "y";
    Clock clock;
    clock.start();
    double compile_start_time;
    double compile_end_time;
    double change_signal_time;
    double change_complete_time;
    double compile_time = 0;
    double change_time = 0;
    size_t db_size = 0;

    // door control shared memories
    const char *rule_file_name = argv[1];
    const char *test_signal_file_name = "/home/shanaka/repos/test_signals"; 
    const char *bridge_door_control_file_name = "/home/shanaka/repos/bridge_door_control";
    const char *hd_control_file_name = "/home/shanaka/repos/hs_db_control";
    const char *hd_db_file_name = "/home/shanaka/repos/hs_db_handle";
    const char *hd_db_sb_file_name = "/home/shanaka/repos/hs_db_handle_standby";
    
    int file_size;  
    int options = PROT_READ | PROT_WRITE;
    int mode = MAP_SHARED;
    
    test_signals = (char *)set_up_mmap(test_signal_file_name, 100, options, mode);
    file_size = sizeof(bridge_door_control_t);
    bridge_controller = (bridge_door_control_t *)set_up_mmap(bridge_door_control_file_name, file_size, options, mode);
    file_size = sizeof(hs_shared_db_control_t);
    hs_db_controller = (hs_shared_db_control_t *)(set_up_mmap(hd_control_file_name, file_size, options, mode));
    file_size = 1024*1024*1024;
    databaseHS_array[0] = (hs_database_t*)(set_up_mmap(hd_db_file_name, file_size, options, mode));  
    databaseHS_array[1] = (hs_database_t*)(set_up_mmap(hd_db_sb_file_name, file_size, options, mode));
    
    int initial_database_compile = 1;   
    hs_db_controller->change_db = 0;
    hs_db_controller->init_database = 0;

    // Hyper Scan compilation
        vector<string> rulesString;
        vector<string> patterns;
        vector<unsigned> flags;
        vector<unsigned> ids;

        // do the actual file reading and string handling
        parseFile(rule_file_name, patterns, flags, ids, rulesString);
        /*
        rulesString.push_back("500:/HTTP/");
        rulesString.push_back("501:/GET/");
        rulesString.push_back("502:/POST/");
        rulesString.push_back("503:/Host:/");
        rulesString.push_back("504:/<title/");
        rulesString.push_back("505:/Content-Length:/");
        rulesString.push_back("506:/Content-Type:/");
        rulesString.push_back("507:/Accept-Encoding:/");
        rulesString.push_back("508:/Content-Encoding:/");
        rulesString.push_back("509:/Status:/");
        */
        
        vector<AppRule> app_1_Rules;
        vector<AppRule> app_3_Rules;
        for (int i =0; i<max_rules ; i++){
            app_1_Rules.push_back(AppRule( 0x1, rulesString[i]));
        }
        for (int i =max_rules; i<max_rules*2 ; i++){
            app_3_Rules.push_back(AppRule( 0x1, rulesString[i]));
        }
         
         /*   
        app_1_Rules.push_back(AppRule( 0x1, rulesString[0]));
        app_1_Rules.push_back(AppRule( 0x1, rulesString[1]));

        vector<AppRule> app_2_Rules;
        app_2_Rules.push_back(AppRule( 0x2, rulesString[0]));
        app_2_Rules.push_back(AppRule( 0x2, rulesString[1]));

        vector<AppRule> app_3_Rules;
        app_3_Rules.push_back(AppRule( 0x3, rulesString[2]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[3]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[4]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[5]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[6]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[7]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[8]));
        app_3_Rules.push_back(AppRule( 0x3, rulesString[9]));
    */


        
        unsigned tempMainID;
        for (int index = 0; index < app_1_Rules.size(); index++){
            rule_manager.insertAppRule(app_1_Rules[index], &tempMainID);
            //cout <<" tempMainID "<< tempMainID << endl;l
        }
        /*
        for (int index = 0; index < app_2_Rules.size(); index++){
            rule_manager.insertAppRule(app_2_Rules[index], &tempMainID);
            //cout <<" tempMainID "<< tempMainID << endl;
        }
        */
        cout << "database compile started" <<endl;
    rule_manager.compileDatabase(  databaseHS_array[0],
                            HS_MODE_STREAM    //HS_MODE_BLOCK or HS_MODE_STREAM
                                );
    rule_manager.compileDatabase(  databaseHS_array[1],
                            HS_MODE_STREAM    //HS_MODE_BLOCK or HS_MODE_STREAM
                                );
    cout << "database compile complete" <<endl;
    int loop_count = 0;
    test_signals[0] = 0x0;
    while(1){
        if( test_signals[0] == 0x1  )
            loop_count++;
        if(loop_count == 100 && test_signals[0] == 0x1){
            for (int index = 0; index < app_3_Rules.size(); index++){
            rule_manager.insertAppRule(app_3_Rules[index], &tempMainID);
            //cout <<" tempMainID "<< tempMainID << endl;
        }
        }
        // database compile 
        
        int standby_db = (hs_db_controller->newest_database == 1) ? 0 : 1;
        if (hs_db_controller->running[standby_db] == 0 && rule_manager.getNoofRuleChanges() > 5){
            printf("db change started\n");
            compile_start_time = clock.secondsPassed();
            if(initial_database_compile){
                rule_manager.compileDatabase(  databaseHS_array[standby_db],
                            HS_MODE_STREAM    //HS_MODE_BLOCK or HS_MODE_STREAM
                                );
                initial_database_compile = 0;
            }
            else{
                 rule_manager.updateDatabase(  databaseHS_array[standby_db],
                            HS_MODE_STREAM    //HS_MODE_BLOCK or HS_MODE_STREAM
                                );
            }
            compile_end_time = clock.secondsPassed(); 
            hs_database_size(databaseHS_array[standby_db], &db_size);
            if(hs_db_controller->change_db == 0){
                hs_db_controller->init_database = standby_db;
                hs_db_controller->change_db = 1;
                change_signal_time = clock.secondsPassed();
                // wait till scratch allocation
                while(hs_db_controller->change_db == 1){
                    //printf(".");
                }
                change_complete_time = clock.secondsPassed();
                printf("\ndb changed complete\n");
            }
            
        }        
        if(loop_count > 0 && test_signals[0] == 0x0 )
            break;
        clock.stop();
    } 
    compile_time = (double)(1000000*(compile_end_time  - compile_start_time)); 
    change_time = (double)(1000000*(change_complete_time  - change_signal_time)); 

    printf("rules, %d \n compile_time, %f us \nsignal_time, %f us \ndb_size, %d bytes\n",max_rules,compile_time, change_time, db_size);
    return 0;
}

static unsigned parseFlags(const string &flagsStr) {
    unsigned flags = 0;
    for (const auto &c : flagsStr) {
        switch (c) {
        case 'i':
            flags |= HS_FLAG_CASELESS; break;
        case 'm':
            flags |= HS_FLAG_MULTILINE; break;
        case 's':
            flags |= HS_FLAG_DOTALL; break;
        case 'H':
            flags |= HS_FLAG_SINGLEMATCH; break;
        case 'V':
            flags |= HS_FLAG_ALLOWEMPTY; break;
        case '8':
            flags |= HS_FLAG_UTF8; break;
        case 'W':
            flags |= HS_FLAG_UCP; break;
        case '\r': // stray carriage-return
            break;
        default:
            cerr << "Unsupported flag \'" << c << "\'" << endl;
            exit(-1);
        }
    }
    return flags;
}

static void parseFile(const char *filename, vector<string> &patterns,
                      vector<unsigned> &flags, vector<unsigned> &ids, vector<string> &rulestrings) {
    ifstream inFile(filename);
    if (!inFile.good()) {
        cerr << "ERROR: Can't open pattern file \"" << filename << "\"" << endl;
        exit(-1);
    }

    for (unsigned i = 1; !inFile.eof(); ++i) {
        string line;
        getline(inFile, line);

        // if line is empty, or a comment, we can skip it
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // otherwise, it should be ID:PCRE, e.g.
        //  10001:/foobar/is

        size_t colonIdx = line.find_first_of(':');
        if (colonIdx == string::npos) {
            cerr << "ERROR: Could not parse line " << i << endl;
            exit(-1);
        }

        // we should have an unsigned int as an ID, before the colon
        unsigned id = std::stoi(line.substr(0, colonIdx).c_str());

        // rest of the expression is the PCRE
        const string expr(line.substr(colonIdx + 1));

        size_t flagsStart = expr.find_last_of('/');
        if (flagsStart == string::npos) {
            cerr << "ERROR: no trailing '/' char" << endl;
            exit(-1);
        }

        string pcre(expr.substr(1, flagsStart - 1));
        string flagsStr(expr.substr(flagsStart + 1, expr.size() - flagsStart));
        unsigned flag = parseFlags(flagsStr);
        rulestrings.push_back(line);
        patterns.push_back(pcre);
        flags.push_back(flag);
        ids.push_back(id);
    }
}
