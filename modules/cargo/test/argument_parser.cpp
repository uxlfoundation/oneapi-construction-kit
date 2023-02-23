// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/argument_parser.h>
#include <gtest/gtest.h>

#include <array>

TEST(argument_parser, parse_args_bool_default) {
  cargo::argument_parser<1> parser;
  cargo::argument_parser<1, 1, 1> parser_fallthrough(
      cargo::argument_parser_option::KEEP_UNRECOGNIZED);

  bool option = false;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_FALSE(option);
  ASSERT_EQ(cargo::success,
            parser_fallthrough.add_argument({"-option", option}));
  ASSERT_FALSE(option);

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option"));
  ASSERT_FALSE(option);
  ASSERT_EQ(cargo::success, parser_fallthrough.parse_args("-not-an-option"));
  ASSERT_EQ(1, parser_fallthrough.get_unrecognized_args().size());
  ASSERT_EQ("-not-an-option", parser_fallthrough.get_unrecognized_args()[0]);
  ASSERT_FALSE(option);

  ASSERT_EQ(cargo::success, parser.parse_args("-option"));
  ASSERT_TRUE(option);
  option = false;
  ASSERT_EQ(cargo::success, parser_fallthrough.parse_args("-option"));
  ASSERT_TRUE(option);
}

TEST(argument_parser, parse_args_bool_store_true) {
  cargo::argument_parser<1> parser;

  bool option = false;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option,
                                                 cargo::argument::STORE_TRUE}));
  ASSERT_FALSE(option);

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option"));
  ASSERT_FALSE(option);

  ASSERT_EQ(cargo::success, parser.parse_args("-option"));
  ASSERT_TRUE(option);
}

TEST(argument_parser, parse_args_bool_store_false) {
  cargo::argument_parser<1> parser;

  bool option = true;
  ASSERT_EQ(
      cargo::success,
      parser.add_argument({"-option", option, cargo::argument::STORE_FALSE}));
  ASSERT_TRUE(option);

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option"));
  ASSERT_TRUE(option);

  ASSERT_EQ(cargo::success, parser.parse_args("-option"));
  ASSERT_FALSE(option);
}

TEST(argument_parser, parse_args_value_equals_default) {
  cargo::argument_parser<1> parser;

  cargo::string_view option;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option=", option}));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option=value"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("-option=value"));
  ASSERT_TRUE("value" == option) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("'-option=val ue'"));
  ASSERT_TRUE("val ue" == option) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("\"-option=val' ue\""));
  ASSERT_TRUE("val' ue" == option) << "  option: \"" << option << '"';
}

TEST(argument_parser, parse_args_choices_equals_default) {
  cargo::argument_parser<1> parser;

  cargo::string_view option;
  std::array<cargo::string_view, 1> choices = {{"true"}};
  ASSERT_EQ(cargo::success, parser.add_argument({"-option=", choices, option}));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option=true"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-option=false"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("-option=true"));
  ASSERT_TRUE("true" == option) << "  option: \"" << option << '"';
}

TEST(argument_parser, parse_args_append_equals_default) {
  cargo::argument_parser<1> parser;

  cargo::small_vector<cargo::string_view, 4> option;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option=", option}));
  ASSERT_TRUE(option.empty());

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option=one"));
  ASSERT_TRUE(option.empty());

  ASSERT_EQ(cargo::success, parser.parse_args("-option=one -option=two"));
  ASSERT_EQ(2u, option.size());
  ASSERT_TRUE("one" == option[0]) << "  option: " << option[0] << '"';
  ASSERT_TRUE("two" == option[1]) << "  option: " << option[1] << '"';
}

TEST(argument_parser, parse_args_value_space_default) {
  cargo::argument_parser<1> parser;

  cargo::string_view option;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option value"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("-option value"));
  ASSERT_TRUE("value" == option) << "  option: \"" << option << '"';
  ASSERT_EQ(cargo::success, parser.parse_args("-option 'val ue'"));
  ASSERT_TRUE("val ue" == option) << "  option: \"" << option << '"';
  ASSERT_EQ(cargo::success, parser.parse_args("-option \"val' ue\""));
  ASSERT_TRUE("val' ue" == option) << "  option: \"" << option << '"';
}

TEST(argument_parser, parse_args_choices_space_default) {
  cargo::argument_parser<1> parser;

  cargo::string_view option;
  std::array<cargo::string_view, 1> choices = {{"true"}};
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", choices, option}));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option true"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-option false"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("-option true"));
  ASSERT_TRUE("true" == option) << "  option: \"" << option << '"';
}

TEST(argument_parser, parse_args_append_space_default) {
  cargo::argument_parser<1> parser;

  cargo::small_vector<cargo::string_view, 4> option;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_TRUE(option.empty());

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option one"));
  ASSERT_TRUE(option.empty());

  ASSERT_EQ(cargo::success, parser.parse_args("-option one -option two"));
  ASSERT_EQ(2u, option.size());
  ASSERT_TRUE("one" == option[0]) << "  option: " << option[0] << '"';
  ASSERT_TRUE("two" == option[1]) << "  option: " << option[1] << '"';
}

TEST(argument_parser, parse_args_value_no_space_default) {
  cargo::argument_parser<1> parser;

  cargo::string_view option;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-option value"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("-option value"));
  ASSERT_TRUE("value" == option) << "  option: \"" << option << '"';
}

TEST(argument_parser, parse_args_choices_no_space_default) {
  cargo::argument_parser<1> parser;

  cargo::string_view option;
  std::array<cargo::string_view, 1> choices = {{"true"}};
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", choices, option}));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-optiontrue"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-optionfalse"));
  ASSERT_TRUE(option.empty()) << "  option: \"" << option << '"';

  ASSERT_EQ(cargo::success, parser.parse_args("-optiontrue"));
  ASSERT_TRUE("true" == option) << "  option: \"" << option << '"';
}

TEST(argument_parser, parse_args_append_no_space_default) {
  cargo::argument_parser<1> parser;

  cargo::small_vector<cargo::string_view, 4> option;
  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_TRUE(option.empty());

  ASSERT_EQ(cargo::bad_argument, parser.parse_args("-not-an-optionone"));
  ASSERT_TRUE(option.empty());

  ASSERT_EQ(cargo::success, parser.parse_args("-optionone -optiontwo"));
  ASSERT_EQ(2u, option.size());
  ASSERT_TRUE("one" == option[0]) << "  option: \"" << option[0] << '"';
  ASSERT_TRUE("two" == option[1]) << "  option: \"" << option[1] << '"';
}

TEST(argument_parser, parse_args_positional) {
  cargo::argument_parser<1, 4, 1> parser(
      cargo::argument_parser_option::ACCEPT_POSITIONAL);

  bool option = false;

  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_FALSE(option);

  ASSERT_EQ(cargo::success,
            parser.parse_args("file1 -option - -- file2 -option"));
  ASSERT_TRUE(option);
  ASSERT_EQ(4, parser.get_positional_args().size());
  ASSERT_EQ("file1", parser.get_positional_args()[0]);
  ASSERT_EQ("-", parser.get_positional_args()[1]);
  ASSERT_EQ("file2", parser.get_positional_args()[2]);
  ASSERT_EQ("-option", parser.get_positional_args()[3]);
}

TEST(argument_parser, parse_args_custom) {
  cargo::argument_parser<4> parser;

  bool option = false;
  int counter = 0;

  ASSERT_EQ(cargo::success, parser.add_argument({"-option", option}));
  ASSERT_EQ(cargo::success,
            parser.add_argument({"-add",
                                 [&counter](cargo::string_view sv) {
                                   (void)sv;
                                   counter++;
                                   return cargo::argument::parse::COMPLETE;
                                 },
                                 [&counter](cargo::string_view sv) {
                                   counter += sv[0] - '0' - 1;
                                   return cargo::argument::parse::COMPLETE;
                                 }}));
  ASSERT_FALSE(option);
  ASSERT_EQ(0, counter);

  ASSERT_EQ(cargo::success, parser.parse_args("-option -add"));
  ASSERT_TRUE(option);
  ASSERT_EQ(1, counter);
  counter = 0;

  ASSERT_EQ(cargo::success, parser.parse_args("-option -add3"));
  ASSERT_TRUE(option);
  ASSERT_EQ(3, counter);
  counter = 0;

  ASSERT_EQ(cargo::success, parser.parse_args("-option -add -add2 -add"));
  ASSERT_TRUE(option);
  ASSERT_EQ(4, counter);
  counter = 0;
}

TEST(argument_parser, parse_args_argv) {
  cargo::argument_parser<4> parser;

  cargo::string_view in;
  ASSERT_EQ(cargo::success, parser.add_argument({"-i", in}));
  cargo::string_view out;
  ASSERT_EQ(cargo::success, parser.add_argument({"-o", out}));

  std::array<const char *, 5> args{{
    "UnitCargo",  // executable name is ignored.
    "-i",
    "input",
    "-o",
    "output",
  }};

  ASSERT_EQ(cargo::success, parser.parse_args(args.size(), args.data()));

  ASSERT_EQ("input", in);
  ASSERT_EQ("output", out);
}
