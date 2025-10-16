// Minimal test to see if {%  can be tokenized
module.exports = grammar({
  name: "test_tag",
  
  rules: {
    source_file: $ => choice(
      $.tag,
      $.text
    ),
    
    tag: $ => seq('{%', 'image', '/%}'),
    
    text: $ => /[a-z]+/
  }
});
