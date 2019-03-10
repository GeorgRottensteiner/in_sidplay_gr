
    // Don't include stuff here, include in Scanner.h


    #define TOKEN_UNKNOWN        -1 // BLAAAAA!!!!!!!!
    #define TOKEN_EOQ             0 // End of Query
    #define TOKEN_IDENTIFIER      1 // Column name or data to match against
    #define TOKEN_EQUAL           2 // =, ==, IS
    #define TOKEN_NOTEQUAL        3 // !=, =!, <>, ><
    #define TOKEN_BELOW           4 // <
    #define TOKEN_ABOVE           5 // >
    #define TOKEN_BOREQUAL        6 // <=, =<
    #define TOKEN_AOREQUAL        7 // >=, =>
    #define TOKEN_NOT             8 // !, NOT
    #define TOKEN_AND             9 // &, &&, AND
    #define TOKEN_OR             10 // |, ||, OR
    #define TOKEN_PAROPEN        11 // (
    #define TOKEN_PARCLOSE       12 // )
    #define TOKEN_CONTAINS       13 // HAS
    #define TOKEN_BEGINS         14	// string starts with...
    #define TOKEN_ENDS           15	// string ends with...
    #define TOKEN_LIKE           16 // string is nearly (excluding "the " and whitespace etc)
    #define TOKEN_ISEMPTY        17	// field does not exists
    #define TOKEN_SQBRACKETOPEN  18 // [
    #define TOKEN_SQBRACKETCLOSE 19 // ]
    #define TOKEN_COMMA          20 // ,
    #define TOKEN_NOTCONTAINS    21    // NOTHAS
    #define TOKEN_ISNOTEMPTY     22	// field does not exists
    #define TOKEN_BEGINSLIKE     23 // string is nearly starts with (excluding "the " and whitespace etc)

    BOOL Query_Parse(const wchar_t *query);
    static int Query_LookupToken(wchar_t *token);
    void Query_CleanUp(void);
    void Query_SyntaxError(int c);
public:
		static int Query_GetNextToken(const wchar_t *p, int *size, wchar_t **token, int tokentable=0);
		static const wchar_t *Query_EatSpace(const wchar_t *p);
		static wchar_t *Query_ProbeSpace(wchar_t *p);
    static const wchar_t *Query_ProbeNonAlphaNum(const wchar_t *p);
    static wchar_t *Query_ProbeAlphaNum(wchar_t *p);
    static int Query_isControlChar(wchar_t p);
    // you must free the mem!
    static wchar_t *Query_Unescape(wchar_t *p);

BOOL Query(const char *query);
BOOL Query(const wchar_t *query);
const wchar_t *GetLastQuery();


public://fucko: protected
    //String token;
    LinkedList pstack;
    wchar_t *token;

wchar_t *last_query;
int last_query_failed;
