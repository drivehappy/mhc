#include <gtest/gtest.h>

#include <parser.h>

using namespace mhc;


// Comments
TEST(ParserTest, BasicBlockComment) {
	const auto input =
		"{-"
		" I'm a block comment."
		"-}";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, NestedBlockComment) {
	const auto input =
		"{-"
		" I'm a block comment."
		" {-"
		"   I'm a nested comment."
		" -}"
		"-}";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, LineCommentEOL) {
	const auto input =
		"-- I'm a comment on a single line\n"
		"-- I'm a second comment\n";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, LineCommentEOI) {
	const auto input =
		"-- I'm a comment on a single without a newline";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, GeneralDeclaration) {
	const auto input =
		"validIdentifier :: ()";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, SimpleDataType) {
	const auto input =
		"data BookInfo";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, DataType) {
	// Taken from Real World Haskell, Ch. 3
	const auto input =
		"data BookInfo = Book Int String [String]"
		"                deriving (Show)";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, DataType_Invalid) {
	// Invalid, should specificy type, not a variable
	const auto input =
		"data BookInfo = book";

	EXPECT_FALSE(parse(input));
}



/*
// Identifiers
TEST(ParserTest, Identifier_Valid) {
	const auto input = "validIdentifier";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, IdentifierPrime_Valid) {
	const auto input = "a'";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Identifier_ValidUnused) {
	const auto input = "_foo";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Identifier_ValidStartsWithReserved) {
	const auto input = "caseFoo";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Identifier_ValidLarge) {
	const auto input = "Alkjlkj";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Identifier_ValidReservedId) {
	const auto input = "case";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Identifier_ValidReservedId_Unused) {
	const auto input = "_";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, QualifiedName) {
	const auto input = "F.g";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, QualifiedName_DoubleDot) {
	const auto input = "f..";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Literal_Integer_Decimal) {
	const auto input = "42";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Literal_Integer_Octal) {
	const auto input = "0o17";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Literal_Integer_Hexadecimal) {
	const auto input = "0x17";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Literal_Integer_BadHexadecimal) {
	// This is expected to be parsed correctly, except not as hexdecimal:
	//  '0'  (decimal)
	//  'x'  (identifier)
	//  '17' (decimal)
	const auto input = "0x 17";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, String_Valid) {
	const auto input = "\"I am a string\"";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, String_Escaped) {
	const auto input = "\"I am an \"escaped\" string\"";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Char_Valid) {
	const auto input = "'I'";

	EXPECT_TRUE(parse(input));
}

TEST(ParserTest, Char_Invalid) {
	const auto input = "'am'";

	EXPECT_FALSE(parse(input));
}

TEST(ParserTest, Type_Expr) {
	const auto input = "top :: Int a -> a";

	EXPECT_TRUE(parse(input));
}
*/
