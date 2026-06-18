#include "lexer.hpp"

#include <cstdlib>
#include <stdexcept>

namespace hyh {
namespace {

void advancePosition(const char* begin, const char* end, std::size_t& line, std::size_t& column) {
	for (const char* it = begin; it < end; ++it) {
		if (*it == '\n') {
			++line;
			column = 1;
		} else if (*it == '\r') {
			if (it + 1 < end && *(it + 1) == '\n') {
				continue;
			}
			++line;
			column = 1;
		} else {
			++column;
		}
	}
}

} // namespace

Lexer::Lexer(std::string_view input): input_(input) {
	cursor_ = input_.data();
	marker_ = cursor_;
	limit_ = input_.data() + input_.size();
}

bool Lexer::atEnd() const {
	return cursor_ >= limit_;
}

char Lexer::peek() const {
	return atEnd() ? '\0' : *cursor_;
}

char Lexer::advance() {
	const char c = peek();
	if (!atEnd()) {
		++cursor_;
		++offset_;
		++column_;
	}
	return c;
}

void Lexer::skipHorizontalWhitespace() {
	while (!atEnd()) {
		const char c = peek();
		if (c == ' ' || c == '\t' || c == '\f' || c == '\v') {
			advance();
		} else {
			break;
		}
	}
}

Lexeme Lexer::makeLexeme(Lexeme::Kind kind, std::size_t begin, std::size_t end, std::size_t line, std::size_t column) const {
	Lexeme lexeme;
	lexeme.kind = kind;
	lexeme.text = input_.substr(begin, end - begin);
	lexeme.line = line;
	lexeme.column = column;
	return lexeme;
}

Lexeme Lexer::makeNumber(std::size_t begin, std::size_t end, std::size_t line, std::size_t column) const {
	Lexeme lexeme = makeLexeme(Lexeme::Kind::Number, begin, end, line, column);
	lexeme.intValue = std::atoi(lexeme.text.c_str());
	return lexeme;
}

Lexeme Lexer::makeString(std::size_t begin, std::size_t end, std::size_t line, std::size_t column) const {
	Lexeme lexeme = makeLexeme(Lexeme::Kind::String, begin, end, line, column);
	if (lexeme.text.size() >= 2 && lexeme.text.front() == '"' && lexeme.text.back() == '"') {
		std::string unquoted;
		for (std::size_t i = 1; i + 1 < lexeme.text.size(); ++i) {
			if (lexeme.text[i] == '\\' && i + 1 < lexeme.text.size()) {
				++i;
			}
			unquoted.push_back(lexeme.text[i]);
		}
		lexeme.text = std::move(unquoted);
	}
	return lexeme;
}

GrammarParser::symbol_type Lexer::next() {
	skipHorizontalWhitespace();

	if (atEnd()) {
		return GrammarParser::make_END();
	}

	const char* tokenStart = cursor_;
	const std::size_t tokenLine = line_;
	const std::size_t tokenColumn = column_;
	const auto beginOffset = static_cast<std::size_t>(tokenStart - input_.data());

	/*!re2c
	re2c:define:YYCTYPE = "unsigned char";
	re2c:define:YYCURSOR = cursor_;
	re2c:define:YYMARKER = marker_;
	re2c:encoding:utf8 = 1;
	re2c:yyfill:enable = 0;

	newline = "\r\n" | "\n" | "\r";
	digit = [0-9];
	ascii = [A-Za-z_][A-Za-z0-9_\-]*;
	hebrew = [\u0590-\u05FF]+;
	str = "\"" ([^"\\\r\n\x00] | "\\" [^\r\n\x00])* "\"";

	newline {
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = static_cast<std::size_t>(cursor_ - input_.data());
		return GrammarParser::make_NEWLINE();
	}
	"." {
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = static_cast<std::size_t>(cursor_ - input_.data());
		return GrammarParser::make_DOT();
	}
	"?" {
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = static_cast<std::size_t>(cursor_ - input_.data());
		return GrammarParser::make_QUESTION();
	}
	":" {
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = static_cast<std::size_t>(cursor_ - input_.data());
		return GrammarParser::make_COLON();
	}
	"," {
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = static_cast<std::size_t>(cursor_ - input_.data());
		return GrammarParser::make_COMMA();
	}
	digit+ {
		const auto endOffset = static_cast<std::size_t>(cursor_ - input_.data());
		auto lexeme = makeNumber(beginOffset, endOffset, tokenLine, tokenColumn);
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = endOffset;
		return GrammarParser::make_NUMBER(std::move(lexeme));
	}
	str {
		const auto endOffset = static_cast<std::size_t>(cursor_ - input_.data());
		auto lexeme = makeString(beginOffset, endOffset, tokenLine, tokenColumn);
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = endOffset;
		return GrammarParser::make_STRING(std::move(lexeme));
	}
	hebrew {
		const auto endOffset = static_cast<std::size_t>(cursor_ - input_.data());
		auto lexeme = makeLexeme(Lexeme::Kind::HebrewWord, beginOffset, endOffset, tokenLine, tokenColumn);
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = endOffset;
		return GrammarParser::make_HEBREW_WORD(std::move(lexeme));
	}
	ascii {
		const auto endOffset = static_cast<std::size_t>(cursor_ - input_.data());
		auto lexeme = makeLexeme(Lexeme::Kind::AsciiWord, beginOffset, endOffset, tokenLine, tokenColumn);
		advancePosition(tokenStart, cursor_, line_, column_);
		offset_ = endOffset;
		return GrammarParser::make_ASCII_WORD(std::move(lexeme));
	}
	* {
		advance();
		return next();
	}
	*/
}

} // namespace hyh
