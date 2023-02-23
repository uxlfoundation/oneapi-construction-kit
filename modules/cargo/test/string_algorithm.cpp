// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/string_algorithm.h>
#include <gtest/gtest.h>

TEST(string_algorithm, split_empty) {
  auto strings = cargo::split("", ",");
  ASSERT_EQ(0u, strings.size());
}

TEST(string_algorithm, split_delimit_ends) {
  auto strings = cargo::split(" blah\tblah\nblah\vblah\fblah\r", "blah");
  ASSERT_EQ(6u, strings.size());
  ASSERT_TRUE(cargo::string_view(" ") == strings[0]);
  ASSERT_TRUE(cargo::string_view("\t") == strings[1]);
  ASSERT_TRUE(cargo::string_view("\n") == strings[2]);
  ASSERT_TRUE(cargo::string_view("\v") == strings[3]);
  ASSERT_TRUE(cargo::string_view("\f") == strings[4]);
  ASSERT_TRUE(cargo::string_view("\r") == strings[5]);
}

TEST(string_algorithm, split_delimit_middle) {
  auto strings = cargo::split("blah blah", "blah");
  ASSERT_EQ(1u, strings.size());
  ASSERT_TRUE(cargo::string_view(" ") == strings[0]);
}

TEST(string_algorithm, split_all_delimit_ends) {
  auto strings = cargo::split_all(" blah\tblah\nblah\vblah\fblah\r", "blah");
  ASSERT_EQ(6u, strings.size());
  ASSERT_TRUE(cargo::string_view(" ") == strings[0]);
  ASSERT_TRUE(cargo::string_view("\t") == strings[1]);
  ASSERT_TRUE(cargo::string_view("\n") == strings[2]);
  ASSERT_TRUE(cargo::string_view("\v") == strings[3]);
  ASSERT_TRUE(cargo::string_view("\f") == strings[4]);
  ASSERT_TRUE(cargo::string_view("\r") == strings[5]);
}

TEST(string_algorithm, split_all_empty) {
  auto strings = cargo::split_all("", ",");
  ASSERT_EQ(0u, strings.size());
}

TEST(string_algorithm, split_all_delimit_middle) {
  auto strings = cargo::split_all("blah blahblah", "blah");
  ASSERT_EQ(4u, strings.size());
  ASSERT_TRUE(cargo::string_view("") == strings[0]);
  ASSERT_TRUE(cargo::string_view(" ") == strings[1]);
  ASSERT_TRUE(cargo::string_view("") == strings[2]);
  ASSERT_TRUE(cargo::string_view("") == strings[3]);
}

TEST(string_algorithm, split_of_empty) {
  auto strings = cargo::split_of("");
  ASSERT_EQ(0u, strings.size());
}

TEST(string_algorithm, split_of_delimit_ends) {
  auto strings = cargo::split_of(" blah\tblah\nblah\vblah\fblah\r");
  ASSERT_EQ(5u, strings.size());
  ASSERT_TRUE(cargo::string_view("blah") == strings[0]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[1]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[2]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[3]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[4]);
}

TEST(string_algorithm, split_of_delimit_middle) {
  auto strings = cargo::split_of("blah..blah", ".");
  ASSERT_EQ(2u, strings.size());
  for (auto string : strings) {
    ASSERT_TRUE(cargo::string_view("blah") == string);
  }
}

TEST(string_algorithm, split_all_of_empty) {
  auto strings = cargo::split_of("");
  ASSERT_EQ(0u, strings.size());
}

TEST(string_algorithm, split_all_of_delimit_ends) {
  auto strings = cargo::split_all_of(" blah\tblah\nblah\vblah\fblah\r");
  ASSERT_EQ(7u, strings.size());
  ASSERT_TRUE(cargo::string_view("") == strings[0]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[1]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[2]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[3]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[4]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[5]);
  ASSERT_TRUE(cargo::string_view("") == strings[6]);
}

TEST(string_algorithm, split_all_of_delimit_middle) {
  auto strings = cargo::split_all_of("blah..blah", ".");
  ASSERT_EQ(3u, strings.size());
  ASSERT_TRUE(cargo::string_view("blah") == strings[0]);
  ASSERT_TRUE(cargo::string_view("") == strings[1]);
  ASSERT_TRUE(cargo::string_view("blah") == strings[2]);
}

TEST(string_algorithm, split_with_quotes) {
  auto strings = cargo::split_with_quotes(
      " s1\t'arg ument'   $has'quote$ '' 'o\th$er '", " \t", "'$");
  ASSERT_EQ(5u, strings.size());
  ASSERT_TRUE(cargo::string_view("s1") == strings[0]);
  ASSERT_TRUE(cargo::string_view("arg ument") == strings[1]);
  ASSERT_TRUE(cargo::string_view("has'quote") == strings[2]);
  ASSERT_TRUE(cargo::string_view("") == strings[3]);
  ASSERT_TRUE(cargo::string_view("o\th$er ") == strings[4]);

  ASSERT_EQ(0u, cargo::split_with_quotes("").size());
  ASSERT_EQ(0u, cargo::split_with_quotes("'").size());

  auto st2 = cargo::split_with_quotes("''");
  ASSERT_EQ(1u, st2.size());
  ASSERT_TRUE(st2[0].empty());

  auto st3 = cargo::split_with_quotes(" \t ");
  ASSERT_EQ(0u, st3.size());
}

TEST(string_algorithm, join_cstring_array) {
  const char *strings[] = {"one", "two", "three"};
  auto joined = cargo::join(std::begin(strings), std::end(strings), " ");
  ASSERT_STREQ("one two three", joined.c_str());
}

TEST(string_algorithm, join_std_vector_std_string) {
  std::vector<std::string> strings{"one", "two", "three"};
  auto joined = cargo::join(strings.begin(), strings.end(), " | ");
  ASSERT_STREQ("one | two | three", joined.c_str());
}

TEST(string_algorithm, trim_left_default_delimiters) {
  auto output = cargo::trim_left(" \t\n\v\f\rblah");
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
}

TEST(string_algorithm, trim_left_custom_delimiters) {
  auto output = cargo::trim_left(".;:blah", ".;:");
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
}

TEST(string_algorithm, trim_left_all_delimiters) {
  auto output = cargo::trim_left(" \t\n\v\f\r");
  ASSERT_EQ(0u, output.size());
  ASSERT_TRUE(cargo::string_view("") == output);
}

TEST(string_algorithm, trim_left_not_found) {
  auto output = cargo::trim_left("blah");
  ASSERT_EQ(4u, output.size());
  ASSERT_TRUE(cargo::string_view("blah") == output);
}

TEST(string_algorithm, trim_right_default_delimiters) {
  auto output = cargo::trim_right("blah \t\n\v\f\r");
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
}

TEST(string_algorithm, trim_right_custom_delimiters) {
  auto output = cargo::trim_right("blah.;:", ".;:");
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
}

TEST(string_algorithm, trim_right_all_delimiters) {
  const char *input = " \t\n\v\f\r";
  auto output = cargo::trim_right(input);
  ASSERT_EQ(0u, output.size());
  ASSERT_EQ(input, output.data());
  ASSERT_TRUE(cargo::string_view("") == output);
}

TEST(string_algorithm, trim_right_not_found) {
  auto output = cargo::trim_right("blah");
  ASSERT_EQ(4u, output.size());
  ASSERT_TRUE(cargo::string_view("blah") == output);
}

TEST(string_algorithm, trim) {
  cargo::string_view input(" \t\n\v\f\rblah \t\n\v\f\r");
  auto output = cargo::trim(input);
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
  input = ".;:blah.;:";
  output = cargo::trim(input, ".;:");
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
  input = "blah";
  output = cargo::trim(input);
  ASSERT_EQ(4u, output.size());
  ASSERT_EQ(cargo::string_view("blah"), output);
}
