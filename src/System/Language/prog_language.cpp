
/******************************************************************************
* MODULE     : prog_language.cpp
* DESCRIPTION: Parser and syntax-highlighter for programming languages
* COPYRIGHT  : (C) 2020  Darcy Shen
*******************************************************************************
* This software falls under the GNU general public license and comes WITHOUT
* ANY WARRANTY WHATSOEVER. See the file $TEXMACS_PATH/LICENSE for more details.
* If you don't have this file, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
******************************************************************************/

#include "analyze.hpp"
#include "impl_language.hpp"
#include "scheme.hpp"
#include "tree.hpp"
#include "iterator.hpp"

prog_language_rep::prog_language_rep (string name):
  abstract_language_rep (name)
{
  debug_packrat << "Building the " * name * " language parser" << LF;

  string use_modules= "(use-modules (prog " * name * "-lang))";
  eval (use_modules);

  // Load (<name>-keywords)
  string get_the_keywords_tree= "(tm->tree (" * name * "-keywords))";
  tree keyword_tree= as_tree (eval (get_the_keywords_tree));
  customize_keyword (keyword_parser, keyword_tree);

  // Load (<name>-operators)
  string get_the_operators_tree= "(tm->tree (" * name * "-operators))";
  tree operator_tree= as_tree (eval (get_the_operators_tree));
  customize_operator (operator_tree);

  // Load (<name>-numbers)
  string get_the_numbers_tree= "(tm->tree (" * name * "-numbers))";
  tree number_tree= as_tree (eval (get_the_numbers_tree));
  customize_number (number_tree);

  // Load (<name>-inline-comment-starts)
  list<string> inline_comment_starts_list=
    as_list_string (eval ("(" * name * "-inline-comment-starts)"));
  array<string> inline_comment_starts;
  for (int i=0; i<N(inline_comment_starts_list); i++) {
    inline_comment_starts << inline_comment_starts_list[i];
  }
  inline_comment_parser.set_starts (inline_comment_starts);

  // Load (<name>-string)
  string get_the_string_tree = "(tm->tree (" * name * "-string))";
  tree string_tree= as_tree (eval (get_the_string_tree));
  customize_string (string_tree);

  // Load (<name>-preprocessors)
  string get_the_preprocessor_tree = "(tm->tree (" * name * "-preprocessors))";
  tree preprocessor_tree= as_tree (eval (get_the_preprocessor_tree));
  customize_preprocessor (preprocessor_tree);
}

void
prog_language_rep::customize_keyword (keyword_parser_rep p_keyword_parser, tree config) {
  for (int i=0; i<N(config); i++) {
    tree group_of_keywords= config[i];
    string group= get_label (group_of_keywords);
    for (int j=0; j<N(group_of_keywords); j++) {
      string word= get_label (group_of_keywords[j]);
      p_keyword_parser.put (word, group);
    }
  }
}

void
prog_language_rep::customize_operator (tree config) {
  for (int i=0; i<N(config); i++) {
    tree group_of_opers= config[i];
    string group= get_label (group_of_opers);
    for (int j=0; j<N(group_of_opers); j++) {
      string word= get_label (group_of_opers[j]);
      operator_parser.put (tm_encode (word), group);
    }
  }
}

void
prog_language_rep::customize_number (tree config) {
  for (int i=0; i<N(config); i++) {
    tree feature= config[i];
    string name= get_label (feature);
    if (name == "bool_features") {
      for (int j=0; j<N(feature); j++) {
        string key= get_label (feature[j]);
        number_parser.insert_bool_feature (key);
      }
    } else if (name == "separator" && N(feature) == 1) {
      string key= get_label (feature[0]);
      number_parser.support_separator (key);
    } else if (name == "suffix") {
      customize_keyword (number_parser.get_suffix_parser(), feature);
    }
  }
}

void
prog_language_rep::customize_string (tree config) {
  for (int i=0; i<N(config); i++) {
    tree feature= config[i];
    string name= get_label (feature);
    if (name == "bool_features") {
      for (int j=0; j<N(feature); j++) {
        string key= get_label (feature[j]);
        escaped_char_parser.insert_bool_feature (key);
      }
    } else if (name == "escape_sequences") {
      array<string> escape_seq;
      for (int j=0; j<N(feature); j++) {
        string key= get_label (feature[j]);
        escape_seq << key;
      }
      escaped_char_parser.set_sequences (escape_seq);
    }
  }

  string_parser.set_escaped_char_parser (escaped_char_parser);
  hashmap<string, string> pairs;
  pairs("\"") = "\"";
  pairs("\'")= "\'";
  string_parser.set_pairs (pairs);
  debug_packrat << string_parser.to_string();
}

void
prog_language_rep::customize_preprocessor (tree config) {
  for (int i=0; i<N(config); i++) {
    tree feature= config[i];
    string name= get_label (feature);
    if (name == "directives") {
      array<string> directives;
      for (int j=0; j<N(feature); j++) {
        string key= get_label (feature[j]);
        directives << key;
      }
      preprocessor_parser.set_directives (directives);
    }
  }
  debug_packrat << preprocessor_parser.to_string();
}

text_property
prog_language_rep::advance (tree t, int& pos) {
  string s= t->label;
  if (pos>=N(s)) return &tp_normal_rep;

  if (string_parser.unfinished ()) {
    if (string_parser.escaped () && string_parser.parse_escaped (s, pos)) {
      current_parser= escaped_char_parser.get_parser_name ();
      return &tp_normal_rep;
    }
    if (string_parser.parse (s, pos)) {
      current_parser= string_parser.get_parser_name ();
      return &tp_normal_rep;
    }
  }

  if (blanks_parser.parse (s, pos)) {
    current_parser= blanks_parser.get_parser_name ();
    return &tp_space_rep;
  }
  if (preprocessor_parser.parse (s, pos)) {
    current_parser= preprocessor_parser.get_parser_name ();
    return &tp_normal_rep;
  }
  if (string_parser.parse (s, pos)) {
    current_parser= string_parser.get_parser_name ();
    return &tp_normal_rep;
  }
  if (number_parser.parse (s, pos)) {
    current_parser= number_parser.get_parser_name ();
    return &tp_normal_rep;
  }
  if (operator_parser.parse (s, pos)) {
    current_parser= operator_parser.get_parser_name ();
    return &tp_normal_rep;
  }
  if (keyword_parser.parse (s, pos)) {
    current_parser= keyword_parser.get_parser_name ();
    return &tp_normal_rep;
  }
  if (identifier_parser.parse (s, pos)) {
    current_parser= identifier_parser.get_parser_name ();
    return &tp_normal_rep;
  }

  tm_char_forwards (s, pos);
  current_parser= "";

  return &tp_normal_rep;
}

array<int>
prog_language_rep::get_hyphens (string s) {
  int i;
  array<int> penalty (N(s)+1);
  penalty[0]= HYPH_INVALID;
  for (i=1; i<N(s); i++)
    if (s[i-1] == '-' && is_alpha (s[i]))
      penalty[i]= HYPH_STD;
    else penalty[i]= HYPH_INVALID;
  penalty[i]= HYPH_INVALID;
  return penalty;
}

void
prog_language_rep::hyphenate (
  string s, int after, string& left, string& right)
{
  left = s (0, after);
  right= s (after, N(s));
}

string
prog_language_rep::get_color (tree t, int start, int end) {
  static string none= "";
  if (start >= end) return none;


  // Coloring as multi-line comment
  if (in_comment (start, t))
    return decode_color (lan_name, encode_color ("comment"));

  string type= none;
  string s= t->label;
  
  // Coloring as inline comment
  int pos= 0;
  while (pos <= start) {
    if (inline_comment_parser.can_parse (s, pos)) {
      return decode_color (lan_name, encode_color ("comment"));
    }
    pos ++;
  }

  if (current_parser == "string_parser") {
    type= "constant_string";
  } else if (current_parser == "escaped_char_parser") {
    type= "constant_char";
  } else if (current_parser == "number_parser") {
    type= "constant_number";
  } else if (current_parser == "operator_parser") {
    string oper= s(start, end);
    type= operator_parser.get (oper);
  } else if (current_parser == "keyword_parser") {
    string keyword= s(start, end);
    type= keyword_parser.get (keyword);
  } else if (current_parser == "preprocessor_parser") {
    type= "preprocessor_directive";
  } else {
    type= none;
  }

  if (type == none) return none;
  return decode_color (lan_name, encode_color (type));
}

/******************************************************************************
* Interface
******************************************************************************/

language
prog_language (string s) {
  if (language::instances -> contains (s)) return language (s);

  hashset<string> prog_v1_langs= hashset<string>();
  prog_v1_langs
    << string("cpp")        << string("dot")    << string("java")
    << string("javascript") << string("json")   << string("octave")
    << string("python")     << string("scala");

  if (prog_v1_langs->contains (s))
    return make (language, s, tm_new<prog_language_rep> (s));

  if (s == "scheme")
    return make (language, s, tm_new<scheme_language_rep> (s));
  if (s == "mathemagix" || s == "mmi" || s == "caas")
    return make (language, s, tm_new<mathemagix_language_rep> (s));
  if (s == "scilab")
    return make (language, s, tm_new<scilab_language_rep> (s));
  if (s == "r")
    return make (language, s, tm_new<r_language_rep> (s));
  if (s == "fortran")
    return make (language, s, tm_new<fortran_language_rep> (s));
  return make (language, s, tm_new<verb_language_rep> (s));
}
