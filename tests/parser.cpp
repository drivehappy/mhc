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

TEST(ParserTest, Identifier_InvalidLarge) {
	const auto input = "Alkjlkj";

	EXPECT_FALSE(parse(input));
}

TEST(ParserTest, Identifier_InvalidReservedId) {
	const auto input = "case";

	EXPECT_FALSE(parse(input));
}

TEST(ParserTest, Identifier_InvalidReservedId_Unused) {
	const auto input = "_";

	EXPECT_FALSE(parse(input));
}

