#include <gtest/gtest.h>

#include "string/lang_string.h"

#include <algorithm>
#include <string>
#include <vector>

TEST(LangStringTest, GetPrefixSuffix) {
    std::string s = "hello";
    EXPECT_EQ(lang::get_prefix(s, 2), "he");
    EXPECT_EQ(lang::get_prefix(s, 10), "hello");
    EXPECT_EQ(lang::get_suffix(s, 2), "lo");
    EXPECT_EQ(lang::get_suffix(s, 10), "hello");
}

TEST(LangStringTest, GetAllPrefixes) {
    std::vector<std::string> p = lang::get_all_prefixes("ab");
    EXPECT_EQ(p.size(), 3u);
    EXPECT_EQ(p[0], "");
    EXPECT_EQ(p[1], "a");
    EXPECT_EQ(p[2], "ab");
}

TEST(LangStringTest, ConcatAndGetSuffixK) {
    EXPECT_EQ(lang::concat_and_get_suffix_k("ab", "cde", 2), "de");
    EXPECT_EQ(lang::concat_and_get_suffix_k("a", "b", 3), "ab");
}

TEST(LangStringTest, GetAllCharsAndAlphabet) {
    std::vector<std::string> chars = lang::get_all_chars("aabcc");
    EXPECT_EQ(chars.size(), 3u);
    std::vector<std::string> alpha =
        lang::get_alphabet_from_strings({"abc", "bcd"});
    EXPECT_EQ(alpha.size(), 4u);
}

TEST(LangStringTest, ConcatStringLists) {
    std::vector<std::string> s1 = {"a", "b"};
    std::vector<std::string> s2 = {"x", "y"};
    std::vector<std::string> out = lang::concat_string_lists(s1, s2);
    EXPECT_EQ(out.size(), 4u);
    EXPECT_TRUE(std::find(out.begin(), out.end(), "ax") != out.end());
    EXPECT_TRUE(std::find(out.begin(), out.end(), "by") != out.end());
}

TEST(LangStringTest, IsSubSeq) {
    EXPECT_TRUE(lang::is_sub_seq("abc", "ac"));
    EXPECT_TRUE(lang::is_sub_seq("abc", ""));
    EXPECT_TRUE(lang::is_sub_seq("abc", "abc"));
    EXPECT_FALSE(lang::is_sub_seq("ac", "abc"));
    EXPECT_FALSE(lang::is_sub_seq("abc", "ax"));
}

TEST(LangStringTest, GetAllSubseqLeqK) {
    std::vector<std::string> sub = lang::get_all_subseq_leq_k("ab", 2);
    EXPECT_GE(sub.size(), 1u);
    EXPECT_TRUE(std::find(sub.begin(), sub.end(), "") != sub.end());
    EXPECT_TRUE(std::find(sub.begin(), sub.end(), "a") != sub.end());
    EXPECT_TRUE(std::find(sub.begin(), sub.end(), "b") != sub.end());
    EXPECT_TRUE(std::find(sub.begin(), sub.end(), "ab") != sub.end());
}

TEST(LangStringTest, CatSubseqLeqQ) {
    std::vector<std::string> s1 = {"a"};
    std::vector<std::string> s2 = {"b"};
    std::vector<std::string> out = lang::cat_subseq_leq_q(s1, s2, 2);
    EXPECT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], "ab");
}

TEST(LangStringTest, SubseqRemoveShort) {
    std::vector<std::string> subs = {"a", "ab", "b", "abc"};
    std::vector<std::string> out = lang::subseq_remove_short(subs);
    EXPECT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], "abc");
}
