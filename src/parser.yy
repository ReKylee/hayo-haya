%skeleton "lalr1.cc"
%require "3.8"

%define api.namespace {hyh}
%define api.parser.class {GrammarParser}
%define api.value.type variant
%define api.token.constructor
%define parse.error verbose

%parse-param { hyh::Lexer& lexer }
%parse-param { hyh::Driver& driver }
%lex-param { hyh::Lexer& lexer }
%lex-param { hyh::Driver& driver }

%code requires {
	#include <string>
	#include <vector>
	#include "raw/raw_syntax.hpp"

	namespace hyh {
		class Lexer;
		class Driver;
	}
}

%code {
	#include "driver.hpp"
	#include "lexer.hpp"

	static hyh::GrammarParser::symbol_type yylex(hyh::Lexer& lexer, hyh::Driver&) {
		return lexer.next();
	}
}

%token END 0 "end of file"
%token DOT "."
%token QUESTION "?"
%token COLON ":"
%token COMMA ","
%token NEWLINE "newline"
%token <hyh::Lexeme> HEBREW_WORD "Hebrew word"
%token <hyh::Lexeme> ASCII_WORD "ASCII word"
%token <hyh::Lexeme> NUMBER "number"
%token <hyh::Lexeme> STRING "string"

%type <std::vector<hyh::Lexeme>> pieces
%type <hyh::Lexeme> piece

%%

program:
	  leading_newlines items maybe_final_item
	;

leading_newlines:
	  %empty
	| leading_newlines NEWLINE
	;

items:
	  %empty
	| items item
	;

item:
	  pieces DOT trailing_newlines
	{
		driver.addRawLine(std::move($1), hyh::RawLineTerminator::Dot);
	}
	| pieces COLON trailing_newlines
	{
		driver.addRawLine(std::move($1), hyh::RawLineTerminator::Colon);
	}
	| pieces QUESTION trailing_newlines
	{
		driver.addRawLine(std::move($1), hyh::RawLineTerminator::Question);
	}
	| pieces NEWLINE trailing_newlines
	{
		driver.addRawLine(std::move($1), hyh::RawLineTerminator::Newline);
	}
	;

maybe_final_item:
	  %empty
	| pieces
	{
		driver.addRawLine(std::move($1), hyh::RawLineTerminator::Newline);
	}
	;

trailing_newlines:
	  %empty
	| trailing_newlines NEWLINE
	;

pieces:
	  piece
	{
		$$ = std::vector<hyh::Lexeme>{std::move($1)};
	}
	| pieces piece
	{
		$1.push_back(std::move($2));
		$$ = std::move($1);
	}
	| pieces COMMA piece
	{
		hyh::Lexeme comma;
		comma.kind = hyh::Lexeme::Kind::Comma;
		comma.text = ",";
		$1.push_back(std::move(comma));
		$1.push_back(std::move($3));
		$$ = std::move($1);
	}
	;

piece:
	  HEBREW_WORD
	{
		$$ = std::move($1);
	}
	| ASCII_WORD
	{
		$$ = std::move($1);
	}
	| NUMBER
	{
		$$ = std::move($1);
	}
	| STRING
	{
		$$ = std::move($1);
	}
	;

%%

void hyh::GrammarParser::error(const std::string& message) {
	driver.addSyntaxError(message);
}
