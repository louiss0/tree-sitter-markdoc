# Markdoc Table Tag Examples

## Simple Table

{% table %}
* Heading 1
* Heading 2
---
* Row 1 Cell 1
* Row 1 Cell 2
---
* Row 2 Cell 1
* Row 2 Cell 2
{% /table %}

## Table with Code in Cells

{% table %}
* Property
* Value
---
*
  ```
  puts "Some code here."
  ```
*
  Default behavior
---
* Another cell
* Data
{% /table %}

## Table with Nested Tags

{% table %}
* Foo
* Bar
* Baz
---
*
  {% list type="checkmark" %}
  * Bulleted list in table
  * Second item in bulleted list
  {% /list %}
*
  Text in a table
* More text
---
* Test 1
* Test 2
* Test 3
{% /table %}

## Table with Loose Lists

{% table %}
* Col 1
* Col 2
* Col 3
---
*
  A "loose" list with

  multiple line items
* Item 2
* Item 3
---
* Test 1
* Test 2
* Test 3
{% /table %}

## Table with Cell Annotations

{% table %}
* Name
* Description
---
* Feature A
* A cell that spans two columns {% colspan=2 %}
---
* Feature B
* Another regular cell
{% /table %}

## Complex Table Example

{% table %}
* Feature
* Description
* Status
---
*
  ```javascript
  const example = true;
  ```
*
  {% callout %}
  This is a note about the feature
  {% /callout %}
* Active
---
*
  Multiple

  Paragraphs
* Regular text here
* {% list type="checkmark" %}
  * Enabled
  * Working
  {% /list %}
{% /table %}
