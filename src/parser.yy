%skeleton "lalr1.cc"
%require "3.8"

%define api.namespace {hyh}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%define lr.type ielr

%locations

%code requires {
    #include <string>
    #include <vector>
    #include "ast.hpp"

    namespace hyh {
        class Driver;
    }
}

%parse-param { hyh::Driver& driver }
%lex-param   { hyh::Driver& driver }

%code {
    #include "driver.hpp"

    static hyh::Parser::symbol_type yylex(hyh::Driver& driver) {
        return driver.lex();
    }
}

%token EOF_TOKEN 0 "end of file"

%token HAYA "היה"
%token HAYTA "הייתה"
%token BEERETZ "בארץ"
%token REHOKA "רחוקה"
%token VESHMA "ושמה"
%token MAMLAKHA "ממלכה"
%token KETANA "קטנה"
%token VEKAKH "וכך"
%token TAM "תם"
%token SIPURA "סיפורה"
%token SHEL "של"
%token MAMLEKHET "ממלכת"

%token MIN "מן"
%token HAMAMLAKHA "הממלכה"
%token HAATIKA "העתיקה"
%token HIGIA "הגיע"
%token USHMO "ושמו"

%token AL "על"
%token EL "אל"
%token NICHTAV "נכתב"
%token NACHU "נחו"
%token KARA "קרא"
%token ET "את"
%token HAKATUV "הכתוב"
%token SHEAL "שעל"
%token NIFTACH "נפתח"
%token NOSAF "נוסף"
%token SHUV "שוב"
%token VESHUV "ושוב"
%token KOL "כל"
%token OD "עוד"
%token PACHOT "פחות"
%token MI_PREFIX "מ־"
%token KAKH "כך"
%token CHAZAR "חזר"
%token HADAVAR "הדבר"
%token KAASHER "כאשר"
%token AMRA "אמרה"
%token YADAA "ידעה"
%token KI "כי"
%token EMET "אמת"
%token BEFI "בפי"
%token NODA "נודע"
%token DAVAR "דבר"
%token BEKHOL "בכל"
%token HIBITA "הביטה"
%token VESHAKTA "ושתקה"
%token HISTIRA "הסתירה"
%token MITACHAT "מתחת"
%token SHAALA "שאלה"

%token DOT "."
%token COMMA ","
%token COLON ":"

%token <std::string> NAME
%token <std::string> ASCII_NAME
%token <std::string> STRING
%token <int> NUMBER

%type <std::string> country_intro kingdom_intro local_alias external_symbol definite_name in_container_name to_container_name
%type <hyh::Statement> statement speech count_change loop_statement condition_statement narrative_statement block_statement
%type <std::vector<hyh::Statement>> block_items
%type <hyh::CountCondition> count_condition

%%

program:
    opening top_level_items ending EOF_TOKEN
    ;

opening:
    HAYA HAYTA COMMA country_intro COMMA kingdom_intro DOT
    | HAYA NAME COMMA country_intro COMMA kingdom_intro DOT
    ;

country_intro:
    BEERETZ REHOKA REHOKA VESHMA NAME
    {
        driver.setCountry($5);
        $$ = $5;
    }
    | BEERETZ REHOKA VESHMA NAME
    {
        driver.setCountry($4);
        $$ = $4;
    }
    ;

kingdom_intro:
    MAMLAKHA KETANA VESHMA NAME
    {
        driver.setKingdom($4);
        $$ = $4;
    }
    | MAMLAKHA VESHMA NAME
    {
        driver.setKingdom($3);
        $$ = $3;
    }
    ;

top_level_items:
    /* empty */
    | top_level_items top_level_item
    ;

top_level_item:
    import_statement
    | declaration
    | statement
    {
        driver.addStatement(std::move($1));
    }
    ;

import_statement:
    MIN HAMAMLAKHA HAATIKA HIGIA external_symbol USHMO local_alias DOT
    {
        driver.addStdImport($7, $5);
    }
    | MIN HAMAMLAKHA HAATIKA HIGIA external_symbol VESHMA local_alias DOT
    {
        driver.addStdImport($7, $5);
    }
    ;

local_alias:
    NAME { $$ = $1; }
    ;

external_symbol:
    ASCII_NAME { $$ = $1; }
    ;

declaration:
    boolean_declaration
    | text_declaration
    | count_declaration
    ;

boolean_declaration:
    definite_name HAYA NAME DOT
    {
        driver.addBoolDecl($1, $3);
    }
    | definite_name HAYTA NAME DOT
    {
        driver.addBoolDecl($1, $3);
    }
    ;

text_declaration:
    AL definite_name NICHTAV STRING DOT
    {
        driver.addTextDecl($2, $4);
    }
    ;

count_declaration:
    in_container_name NACHU NUMBER NAME DOT
    {
        driver.addCountDecl($1, $3, $4);
    }
    ;

statement:
    speech
    | count_change
    | loop_statement
    | condition_statement
    | narrative_statement
    ;

speech:
    definite_name KARA STRING DOT
    {
        $$ = driver.makePrintString($1, $3);
    }
    | definite_name KARA ET HAKATUV SHEAL definite_name DOT
    {
        $$ = driver.makePrintWrittenText($1, $6);
    }
    ;

count_change:
    NOSAF to_container_name NAME NUMBER DOT
    {
        $$ = driver.makeCountChange($2, $3, $4);
    }
    ;

loop_statement:
    SHUV VESHUV COMMA KOL OD count_condition COLON block_items KAKH CHAZAR HADAVAR DOT
    {
        $$ = driver.makeLoop(std::move($6), std::move($8));
    }
    ;

condition_statement:
    KAASHER count_condition COLON block_items TAM HADAVAR DOT
    {
        $$ = driver.makeCondition(std::move($2), std::move($4));
    }
    | KAASHER AMRA definite_name STRING COLON block_items TAM HADAVAR DOT
    {
        $$ = driver.makeUtteranceCondition(std::move($6));
    }
    ;

block_items:
    /* empty */
    {
        $$ = {};
    }
    | block_items block_statement
    {
        $$ = std::move($1);
        $$.push_back(std::move($2));
    }
    ;

block_statement:
    speech
    | count_change
    | narrative_statement
    ;

count_condition:
    in_container_name NACHU PACHOT MI_PREFIX NUMBER NAME
    {
        $$ = driver.makeCountCondition($1, $6, $5);
    }
    ;

narrative_statement:
    in_container_name NAME NAME NAME NAME COMMA NAME NAME NAME DOT
    {
        $$ = driver.makeNarrative();
    }
    | in_container_name NAME NUMBER NAME NUMBER NAME in_container_name DOT
    {
        $$ = driver.makeNarrative();
    }
    | definite_name NIFTACH DOT
    {
        $$ = driver.makeNarrative();
    }
    | EL definite_name NAME definite_name definite_name MIN definite_name DOT
    {
        $$ = driver.makeNarrative();
    }
    | definite_name HIBITA in_container_name VESHAKTA DOT
    {
        $$ = driver.makeNarrative();
    }
    | definite_name HISTIRA NAME NUMBER MITACHAT to_container_name DOT
    {
        $$ = driver.makeNarrative();
    }
    | in_container_name definite_name SHAALA definite_name ET definite_name NAME NAME NAME NAME NAME DOT
    {
        $$ = driver.makeNarrative();
    }
    | definite_name YADAA KI EMET BEFI definite_name DOT
    {
        $$ = driver.makeNarrative();
    }
    | VEKAKH NODA DAVAR definite_name BEKHOL MAMLEKHET NAME DOT
    {
        $$ = driver.makeNarrative();
    }
    ;

definite_name:
    NAME
    {
        $$ = driver.stripOptionalDefinite($1);
    }
    ;

in_container_name:
    NAME
    {
        $$ = driver.stripRequiredPrefix($1, "ב", "in-container name");
    }
    ;

to_container_name:
    NAME
    {
        $$ = driver.stripRequiredPrefix($1, "ל", "to-container name");
    }
    ;

ending:
    VEKAKH TAM SIPURA SHEL MAMLEKHET NAME DOT
    {
        driver.finish($6);
    }
    ;

%%

void hyh::Parser::error(const location_type& location, const std::string& message) {
    driver.reportError(location, message);
}
