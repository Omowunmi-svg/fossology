/***************************************************************
 Copyright (C) 2010 Hewlett-Packard Development Company, L.P.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 ***************************************************************/

/* std library includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* local includes */
#include <radixtree.h>

/// the number of children that a node can have
#define NODE_SIZE 128
/// the base offset that indexing into the child array is done using
#define OFFSET '\0'

/* ************************************************************************** */
/* *** Private Members ****************************************************** */
/* ************************************************************************** */

/*!
 * @brief private typename to remove overuse of the struct keyword
 */
typedef struct _tree_internal node;

/*!
 * @brief internal structure of the radix tree
 *
 * the internal structure of the radix tree, essentially all these members are
 * private to anything outside of this file
 */
struct _tree_internal {
  /// the character represented by this node
  char character;
  /// true if a word terminates at this node, false otherwise
  int terminal;
  /// the list of children of this node
  node* children[NODE_SIZE];
};

/*!
 * @brief private constructor for a radix tree
 *
 * private function that is the actuall constructor for a tree, this allocates
 * space for the node and sets all the values to 0.
 *
 * @param tree the tree to be initialized
 * @param value the value that this node should have
 */
void _radix_initialize(radix_tree* tree_ptr, char value) {
  (*tree_ptr) = (node*)calloc(1, sizeof(node));
  (*tree_ptr)->character = value;
  (*tree_ptr)->terminal = 0;
  memset((*tree_ptr)->children, 0, sizeof((*tree_ptr)->children));
}

/*!
 * @brief private copy constructor for a radix tree
 *
 * private deep copy constructor for a radix tree. the tree pointed to by
 * tree_ptr will be identical to the tree pointed to by reference.
 *
 * @param tree_ptr the tree to copy into
 * @param reference the tree to copy from
 */
void _radix_copy(radix_tree* tree_ptr, radix_tree reference) {
  int i;

  (*tree_ptr) = (node*)calloc(1, sizeof(node));
  (*tree_ptr)->character = reference->character;
  (*tree_ptr)->terminal = reference->terminal;
  memset((*tree_ptr)->children, 0, sizeof((*tree_ptr)->children));

  for(i = 0; i < NODE_SIZE; i++) {
    if(reference->children[i]) {
      _radix_copy(&(*tree_ptr)->children[i], reference->children[i]);
    }
  }
}

/*!
 * @brief private worker function for radix_match
 *
 * Differs from radix_match in that dst will have the match concatenated to the
 * end instead of simply containing the match.
 *
 * @param tree the tree to be searched
 * @param dst the string that will have the match concatenated onto it
 * @param src the string to search for
 * @return the length of the string found
 */
int _radix_match(radix_tree tree, char* dst, char* src) {
  if(strlen(src) == 0) {
    dst[strlen(dst)] = tree->character;
    return 0;
  }

  dst[strlen(dst)] = tree->character;
  if(tree->children[*src - OFFSET] != NULL &&
      tree->children[*src - OFFSET]->character == *src) {
    return 1 + _radix_match(tree->children[*src - OFFSET], dst, src+1);
  }

  return 0;
}

/*!
 * @brief private recursive helper function for the radix_print function
 *
 * recursive function that does the actual file manipulation for the print
 * function. This function is employed because the word must be kept track of as
 * the function recurses and the standard print does not take a string.
 *
 * @param tree
 * @param ostr
 * @param string
 */
void _radix_recprint(radix_tree tree, FILE* ostr, char* string) {
  int i;
  string[strlen(string)] = tree->character;
  string[strlen(string)] = 0;

  if(tree->terminal) {
    fprintf(ostr,"%s\n",string);
  }

  for(i = 0; i < NODE_SIZE; i++) {
    if(tree->children[i]) {
      _radix_recprint(tree->children[i], ostr, string);
    }
  }

  string[strlen(string) - 1] = 0;
}

/* ************************************************************************** */
/* *** Constructor/Destructor *********************************************** */
/* ************************************************************************** */

/*!
 * @brief contstructor for the radix tree
 *
 * initializes the fields of a radix tree
 *
 * @param tree a reference to the tree that is being initialized
 * @param value the value that this tree contains
 */
void radix_init(radix_tree* tree_ptr) {
  _radix_initialize(tree_ptr, '\0');
}

/*!
 * @brief copy constructor for the radix tree
 *
 * @param tree the tree to copy into
 * @param reference the tree to copy from
 */
void radix_copy(radix_tree* tree, radix_tree reference) {
  _radix_copy(tree, reference);
}

/*!
 * @brief destructor for the radix tree
 *
 * recursively frees the radix tree
 *
 * @param tree the tree to be destructed
 */
void radix_destroy(radix_tree tree) {
  int i;

  for(i = 0; i < NODE_SIZE; i++) {
    if(tree->children[i] != NULL) {
      radix_destroy(tree->children[i]);
    }
  }

  free(tree);
}

/* ************************************************************************** */
/* *** Modifier Functions *************************************************** */
/* ************************************************************************** */

/*!
 * @brief inserts a new string into the radix tree
 *
 * This will create the nodes necessary to insert a new node into the tree, the
 * string will be represented as a set of nodes that each represent a character
 *
 * @param tree the tree that the string is to be inserted into
 * @param string the string that will be in the tree
 */
void radix_insert(radix_tree tree, const char* string) {
  /* base case for recursive insert function */
  if(strlen(string) == 0) {
    tree->terminal = 1;
    return;
  }

  /* if the subtree does not yet exist, create it */
  if(tree->children[*string - OFFSET] == NULL) {
    _radix_initialize(&tree->children[*string - OFFSET], *string);
  }

  /* recursively add the next character in the string */
  radix_insert(tree->children[*string - OFFSET], string+1);
}

/*!
 * @brief inserts an array in the radix tree
 *
 * this will insert an array into a radix tree, from the provided first to the
 * last. These should be the pointer to the start of the array and the pointer
 * just past the end of the array.
 *
 * If you wanted to insert an array of string called Dictionary:
 *      char* Dict[] = {"one", "two", ... , "n"};
 *      radix_insert_all(tree, Dict, Dict + sizeof(Dict)/sizeof(char*));
 *
 * @param tree the radix tree that should be inserted into
 * @param start the start of the array
 * @param finsih one past the end of the array
 */
void radix_insert_all(radix_tree tree, char** first, char** last) {
  /* make sure that first is less than last */
  if(first > last) {
    fprintf(stderr, "ERROR: first must be less than last");
    exit(-1);
  }

  /* insert all of the elements into the tree */
  for(;first != last; first++) {
    radix_insert(tree, *first);
  }
}

/* ************************************************************************** */
/* *** Accessor Functions *************************************************** */
/* ************************************************************************** */

/*!
 * @brief tests if a certain word in its entirtiy has been inserted into a tree
 *
 * tests if a certain string has been inserted into the tree, the exact string
 * must have been inserted into the tree. for example if a tree has had "top"
 * inserted into it but not the word "to", this function will return false for
 * the word "to" and true for the word "top"
 *
 * @param tree the tree to be searched in
 * @param string the string to search for
 * @return true if the string in the tree, false otherwise
 */
int radix_contains(radix_tree tree, char* string) {
  if(strlen(string) == 0) {
    return tree->terminal;
  }

  if(tree->children[*string - OFFSET] != NULL) {
    return radix_contains(tree->children[*string - OFFSET], string+1);
  }

  return 0;
}

/*!
 * @brief tests if a certain word in its entirtiy has been inserted into a tree
 *
 * Exactly like contains, but if the string passed in is much longer than the
 * entry in the dictionary, this will still return true where radix_contains
 * would return false
 *
 * @param tree the tree to be searched in
 * @param string the string to search for
 * @return true if the string in the tree, false otherwise
 */
int radix_contains_il(radix_tree tree, char* string) {
  if(tree == NULL) {
    return 1;
  }

  if(strlen(string) == 0) {
    return tree->terminal;
  }

  return radix_contains_il(tree->children[*string - OFFSET], string + 1);
}

/*!
 * @brief attempts to match part of a string to the radix tree
 *
 * finds the longest match starting at the root of a radix tree to the provided
 * string, for example if a radix contained "long word" and the string
 * "long sentence" was search for, this would return 5 and the string dst would
 * contain "long "
 *
 * @param tree the tree to be searched
 * @param dst the string that will contain the match
 * @param src the string to search for a match
 * @return the length of the string found
 */
int radix_match(radix_tree tree, char* dst, char* src) {
  memset(dst, '\0', sizeof(dst));
  return _radix_match(tree, dst, src);
}


/*!
 * @brief prints a radix tree to the output stream provided
 *
 * prints the contents of the radix tree out to a file as a list of words, each
 * word will be on its own line.
 *
 * @param tree the tree to print to the output stream
 * @param ostr the output stream that will be printed to
 */
void radix_print(radix_tree tree, FILE* ostr) {
  char str[256];
  memset(str, 0, sizeof(str));
  _radix_recprint(tree, ostr, str);
}
