import XCTest
import SwiftTreeSitter
import TreeSitterTreeSitterMarkdoc

final class TreeSitterTreeSitterMarkdocTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_tree_sitter_markdoc())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Markdoc grammar")
    }
}
