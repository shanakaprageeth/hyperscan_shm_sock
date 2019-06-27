
#include "RuleManager.hpp"

AppRule::AppRule(unsigned app, string inRule){        
	if (inRule.empty() || inRule[0] == '#') {
		cerr << "ERROR: rule is a comment " << endl;
		return;
	}
	size_t colonIdx = inRule.find_first_of(':');
	if (colonIdx == string::npos) {
		cerr << "ERROR: Could not parse rule " << endl;
		return;
	}
	// we should have an unsigned int as an ID, before the colon
	Id = std::stoi(inRule.substr(0, colonIdx).c_str());
	// rest of the expression is the PCRE
	const string expr(inRule.substr(colonIdx + 1));
	size_t flagsStart = expr.find_last_of('/');
	if (flagsStart == string::npos) {
		cerr << "ERROR: no trailing '/' char" << endl;
		return;
	}
	application = app;
	rule = inRule;
	string pcre(expr.substr(1, flagsStart - 1));
	pattern = pcre;
	string flagsStr(expr.substr(flagsStart + 1, expr.size() - flagsStart));
	flags = parseFlags(flagsStr);
	valid = true;
}
AppRule::~AppRule(){        
}

unsigned AppRule::getApplication(){
	return application;
} 
string AppRule::getRule(){
	return rule;
} 
string AppRule::getPattern(){
	return pattern;
} 
unsigned AppRule::getId(){
	return Id;
}
unsigned AppRule::getFlags(){
	return flags;
}
int AppRule::isValid(){
	return valid;
}
unsigned AppRule::parseFlags(const string &flagsStr) {
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
		default:
			cerr << "Unsupported flag \'" << c << "\'" << endl;
			exit(-1);
		}
	}
	return flags;
}

UniqRule::UniqRule(string inpatternAndFlags, string inPattern, unsigned inFlags,unsigned inUId, long appRelationID){
	patternAndFlags = inpatternAndFlags;
	pattern = inPattern;
	flags = inFlags;
	UId = inUId;
	appRelationIDs.push_back(appRelationID);
}
UniqRule::~UniqRule(){        
}  
int UniqRule::addAppRelationIDToURule(long appRelationID){
	appRelationIDs.push_back(appRelationID);
	return APP_MAN_PASS;
}   
int UniqRule::removeAppRelationIDToURule(long appRelationID){
	auto it = std::find(appRelationIDs.begin(), appRelationIDs.end(), appRelationID);
	if (it == appRelationIDs.end())
	{
	  return APP_MAN_FAIL;
	} 
	else
	{          
	  appRelationIDs.erase(it);	  
	  return APP_MAN_PASS;
	}  
} 
string UniqRule::getPatternAndFlags(){
	return patternAndFlags;
} 
string UniqRule::getPattern(){
	return pattern;
} 
unsigned UniqRule::getUId(){
	return UId;
}
unsigned UniqRule::getFlags(){
	return flags;
}
vector<long> UniqRule::getAppRelationIDs(){
	return appRelationIDs;
}



RuleManager::RuleManager(){}
RuleManager::~RuleManager(){}
int RuleManager::insertAppRule(AppRule appRule, unsigned *mainID){
	if(!( appRule.isValid() )){
		cerr << "ERROR: rule is not valid" << endl;
		return APP_MAN_FAIL;
	} 
	unsigned app = appRule.getApplication();
	unsigned appRuleId = appRule.getId();
	string appPattern = appRule.getPattern();
	unsigned appRuleFlags = appRule.getFlags();
	long appRelationID = calculateMainId( app,  appRuleId);
	string patternAndFlags = appPattern  + to_string( appRuleFlags );
	if (uniqRules.size() == 0) {
		UniqRule URule = UniqRule(patternAndFlags, appPattern, appRuleFlags, 0x0, appRelationID);
		uniqRules.push_back(URule);
		UIds.push_back(0x0);
		isChanged = true;
		newRulesChanges = newRulesChanges + 1;
		*mainID = 0x0;          
		return APP_MAN_PASS;
	}        
	else {            
		for (int itemIdx = 0; itemIdx < uniqRules.size(); itemIdx++){  
			if ( uniqRules[itemIdx].getPatternAndFlags() == patternAndFlags ){                    
				vector<long> tempRelationIDs =  getRelatedAppsMainIds( UIds[itemIdx] );
				if( find(tempRelationIDs.begin(), tempRelationIDs.end(), appRelationID) != tempRelationIDs.end() ) {
					*mainID = UIds[itemIdx];
					cerr << "ERROR: app rule already in the rule manager" <<endl;
				} 
				else {
					uniqRules[itemIdx].addAppRelationIDToURule(appRelationID);
					*mainID = UIds[itemIdx];
				}         
				return APP_MAN_PASS;
			}

		}
		
		unsigned lastUId = uniqRules[uniqRules.size() - 1].getUId();
		UniqRule URule = UniqRule(patternAndFlags, appPattern, appRuleFlags, lastUId+0x1, appRelationID);  
		uniqRules.push_back(URule);
		UIds.push_back(lastUId+0x1);
		isChanged = true;
		newRulesChanges = newRulesChanges + 1;
		*mainID = lastUId+0x1;   
					
		return APP_MAN_PASS;
	}         
	
}

int RuleManager::removeAppRule( AppRule appRule, unsigned *mainID ){ 
	unsigned app = appRule.getApplication();
	unsigned appRuleId = appRule.getId();
	string appPattern = appRule.getPattern();
	unsigned appRuleFlags = appRule.getFlags();
	long appRelationID = calculateMainId( app,  appRuleId);
	string patternAndFlags = appPattern  + to_string( (int)appRuleFlags );      
	for (int itemIdx = 0; itemIdx < uniqRules.size(); itemIdx++){
		if ( uniqRules[itemIdx].getPatternAndFlags() == patternAndFlags){
			*mainID = UIds[itemIdx];
			uniqRules[itemIdx].removeAppRelationIDToURule(appRelationID);
			if (uniqRules[itemIdx].getAppRelationIDs().size() == 0){
				uniqRules.erase( uniqRules.begin() + itemIdx );
				UIds.erase(UIds.begin() + itemIdx);
				isChanged = true;
				newRulesChanges = newRulesChanges + 1;
			}
			return APP_MAN_PASS;
		}
	}         
	return APP_MAN_FAIL;
}
long RuleManager::calculateMainId(unsigned app, unsigned Id){
	int shiftApp = 32;
	unsigned Lmask = (unsigned)(pow(2, shiftApp) - 1);
	long tempLong = 0;
	tempLong = app;
	tempLong = (tempLong << shiftApp);
	tempLong |=  ( Id & Lmask );
	return tempLong ;
}
int RuleManager::calculateAppIds(long appRelationID, int *app, int *ruleId){	
	int shiftApp = 32;
	unsigned Lmask = (unsigned)(pow(2, shiftApp) - 1);
	*ruleId = appRelationID & Lmask;
	*app = appRelationID >> shiftApp;
	return APP_MAN_PASS;
}
vector<long> RuleManager::getRelatedAppsMainIds(unsigned UId){
	unsigned itemIdx = UId;	
	if(itemIdx < UIds.size()){
		if (UIds[itemIdx] == UId){
			return uniqRules[itemIdx].getAppRelationIDs();
		}
		else if (UIds[itemIdx] > UId){
			for (;itemIdx > 0 ; itemIdx--){
				if (UIds[itemIdx] == UId){
					return uniqRules[itemIdx].getAppRelationIDs();
				}
			}
		}
		else{
			for (;itemIdx > uniqRules.size(); itemIdx--){
				if (UIds[itemIdx] == UId){
					return uniqRules[itemIdx].getAppRelationIDs();
				}
			}
		}
	}
	
	//cerr << "ERROR: UId "<<+UId <<" not in the Rule Manager" << endl;
	vector<long> emptyUIdVec;
	return emptyUIdVec;//{};//vector<long> ();//emptyUIdVec;
}

vector<UniqRule> RuleManager::getUniqRules(){
	return uniqRules;
}
int RuleManager::getIsRuleChanged(){
	return isChanged;
}

int RuleManager::getNoofRuleChanges(){
	return newRulesChanges;
}


int RuleManager::setIsRuleChanged(int state){
	isChanged = state;
	return APP_MAN_PASS;
}

int RuleManager::compileDatabase( hs_database_t *hs_db,
					int db_type    //HS_MODE_BLOCK or HS_MODE_STREAM
					){
   newRulesChanges = 0;
	vector<UniqRule> tempUniqRules = this->getUniqRules();
	vector<string> patterns;
	vector<unsigned> flags;
	vector<unsigned> ids;
	for (int index = 0 ; index <  tempUniqRules.size(); index++){
		patterns.push_back(tempUniqRules[index].getPattern());
		flags.push_back(tempUniqRules[index].getFlags());
		ids.push_back(tempUniqRules[index].getUId());
		//printf("rule id %d \n",tempUniqRules[index].getUId());
	}
	// Turn our vector of strings into a vector of char*'s to pass in to
	// hs_compile_multi. (This is just using the vector of strings as dynamic
	// storage.)
	vector<const char*> cstrPatterns;
	for (const auto &pattern : patterns) {
		cstrPatterns.push_back(pattern.c_str());
	}    
	hs_database_t *tempDB;
	tempDB = static_cast<hs_database_t*>(buildDatabase(cstrPatterns, flags, ids, db_type));
	char *buffer; 
	size_t serialized_length;
	hs_serialize_database(tempDB,&buffer,&serialized_length);
	hs_deserialize_database_at(buffer, serialized_length,
									  hs_db);
	this->setIsRuleChanged(false);
	return 1;
}

int RuleManager::updateDatabase( hs_database_t *hs_db,
					int db_type    //HS_MODE_BLOCK or HS_MODE_STREAM
					){
   
	if( this->getIsRuleChanged()  ){
		newRulesChanges = 0;
		vector<UniqRule> tempUniqRules = this->getUniqRules();
		vector<string> patterns;
		vector<unsigned> flags;
		vector<unsigned> ids;
		for (int index = 0 ; index <  tempUniqRules.size(); index++){
			patterns.push_back(tempUniqRules[index].getPattern());
			flags.push_back(tempUniqRules[index].getFlags());
			ids.push_back(tempUniqRules[index].getUId());
		}
		// Turn our vector of strings into a vector of char*'s to pass in to
		// hs_compile_multi. (This is just using the vector of strings as dynamic
		// storage.)
		vector<const char*> cstrPatterns;
		for (const auto &pattern : patterns) {
			cstrPatterns.push_back(pattern.c_str());
		}    

		hs_database_t *tempDB;
		tempDB = static_cast<hs_database_t*>(buildDatabase(cstrPatterns, flags, ids, db_type));
		char *buffer; 
		size_t serialized_length;
		hs_serialize_database(tempDB,&buffer,&serialized_length);
		hs_deserialize_database_at(buffer, serialized_length,
										  hs_db);

		this->setIsRuleChanged(false);
		
		return 1;
	}
	else{
		return 0;
	}
}

hs_database_t *RuleManager::buildDatabase(const vector<const char *> &expressions,
                                    const vector<unsigned> flags,
                                    const vector<unsigned> ids,
                                    unsigned int mode) {
    hs_database_t *db;
    hs_compile_error_t *compileErr;
    hs_error_t err;


    err = hs_compile_multi(expressions.data(), flags.data(), ids.data(),
                           expressions.size(), mode, nullptr, &db, &compileErr);

    if (err != HS_SUCCESS) {
        if (compileErr->expression < 0) {
            // The error does not refer to a particular expression.
            cerr << "ERROR: " << compileErr->message << endl;
        } else {
            cerr << "ERROR: Pattern '" << expressions[compileErr->expression]
                 << "' failed compilation with error: " << compileErr->message
                 << endl;
        }
        // As the compileErr pointer points to dynamically allocated memory, if
        // we get an error, we must be sure to release it. This is not
        // necessary when no error is detected.
        hs_free_compile_error(compileErr);
        exit(-1);
    }

    cout << "Hyperscan " << (mode == HS_MODE_STREAM ? "streaming" : "block")
         << " mode database compiled "<< endl;

    return db;
}