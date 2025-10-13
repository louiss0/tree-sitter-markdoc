package tree_sitter_tree_sitter_markdoc_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_tree_sitter_markdoc "http://github.com/louiss0/tree-sitter-markdoc/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_tree_sitter_markdoc.Language())
	if language == nil {
		t.Errorf("Error loading Markdoc grammar")
	}
}
