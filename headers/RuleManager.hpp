#ifndef RULEMANAGER_H
#define RULEMANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <cstring>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>       
#include <iostream>     
#include <sstream>  
#include <algorithm>

#include </usr/local/include/hs/hs.h> 


using namespace std;


#define APP_MAN_FAIL 1
#define APP_MAN_PASS 0

/* application Rule Create Rule Object with App and Rule information */
class AppRule {
private:
    unsigned application;        // application id
    string rule;            // rule 
    string pattern;         // pattern in the rule 
    unsigned Id;            // application rule id 
    unsigned flags;         // flags in the rule
    int valid = false;     // validness of the rule
public:
    AppRule(unsigned app, string inRule);
    ~AppRule();
    unsigned getApplication();
    string getRule();
    string getPattern();
    unsigned getId();
    unsigned getFlags();
    int isValid();
    unsigned parseFlags(const string &flagsStr) ;
};

/* application Rule Create Rule Object with App and Rule information */
class UniqRule {
private:
    string patternAndFlags;
    string pattern;         // pattern in the rule 
    unsigned UId;            // application rule id 
    unsigned flags;         // flags in the rule
    vector<long> appRelationIDs;
public:
    UniqRule(string inpatternAndFlags, string inPattern, unsigned inFlags,unsigned inUId, long appRelationID);
    ~UniqRule();
    int addAppRelationIDToURule(long appRelationID);
    int removeAppRelationIDToURule(long appRelationID);
    string getPatternAndFlags();
    string getPattern();
    unsigned getUId();
    unsigned getFlags();
    vector<long> getAppRelationIDs();
};

/* create Application Rule Management Engine */
class RuleManager {
private:
    vector<UniqRule> uniqRules;  
    vector<unsigned> UIds;  
    int isChanged = false;  
    int newRulesChanges = 0;
    hs_database_t *buildDatabase(const vector<const char *> &expressions,
                                    const vector<unsigned> flags,
                                    const vector<unsigned> ids,
                                    unsigned int mode);
public:
    RuleManager();
    ~RuleManager();
    int insertAppRule(AppRule appRule, unsigned *mainID);
    int removeAppRule( AppRule appRule, unsigned *mainID );
    long calculateMainId(unsigned app, unsigned Id);
    int calculateAppIds(long appRelationID, int *app, int *ruleId);
    vector<long> getRelatedAppsMainIds(unsigned UId);
    vector<UniqRule> getUniqRules();
    int getIsRuleChanged();
    int getNoofRuleChanges();
    int setIsRuleChanged(int state);
    int updateDatabase( hs_database_t *hs_db,int db_type    //HS_MODE_BLOCK or HS_MODE_STREAM
        );
    int compileDatabase( hs_database_t *hs_db,int db_type    //HS_MODE_BLOCK or HS_MODE_STREAM
        );
};

#endif